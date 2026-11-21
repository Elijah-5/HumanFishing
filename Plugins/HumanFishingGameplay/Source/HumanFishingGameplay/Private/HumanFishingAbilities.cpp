#include "HumanFishingAbilities.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "HumanFishingJellyshock.h"
#include "HumanFishingShark.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

namespace
{
UHumanFishingInventory* AbilityInventory(const UActorComponent* C)
{ return C && C->GetWorld() && C->GetWorld()->GetGameInstance() ? C->GetWorld()->GetGameInstance()->GetSubsystem<UHumanFishingInventory>() : nullptr; }
bool FirstJellyBlock(UWorld* World,const FVector& Start,const FVector& End,const AActor* Owner,
    FHitResult& Out,const AActor* Ignore=nullptr,float Radius=0.f,bool bIgnoreSharks=false)
{
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(JellyshockAim),false,Owner);
    if(Ignore)Params.AddIgnoredActor(Ignore);
    TArray<FHitResult> Hits;
    if(Radius>0)World->SweepMultiByObjectType(Hits,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(Radius),Params);
    else World->LineTraceMultiByObjectType(Hits,Start,End,Objects,Params);
    Hits.Sort([](const FHitResult& A,const FHitResult& B){return A.Time<B.Time;});
    for(const auto& Hit:Hits)if((!bIgnoreSharks || !Cast<AHumanFishingShark>(Hit.GetActor())) &&
        Hit.GetComponent() && Hit.GetComponent()->GetCollisionResponseToChannel(ECC_WorldDynamic)==ECR_Block)
    {Out=Hit;return true;}
    return false;
}
}
bool FHumanFishingSprintState::Advance(float Delta, bool bHeld, bool bAllowed,
    float& Energy, float& Delay, bool& bMustRelease)
{
    Delta=FMath::Max(0.f,Delta);
    Energy=FMath::Clamp(Energy,0.f,Capacity);
    if(!bHeld)bMustRelease=false;
    if(bHeld && bAllowed && !bMustRelease && Energy>0.f)
    {
        Energy=FMath::Max(0.f,Energy-Delta);Delay=1.f;
        if(Energy<=0.f)bMustRelease=true;
        return Energy>0.f;
    }
    const float RechargeTime=FMath::Max(0.f,Delta-Delay);
    Delay=FMath::Max(0.f,Delay-Delta);
    Energy=FMath::Min(Capacity,Energy+RechargeTime*(Capacity/8.f));
    return false;
}
UHumanFishingAbilities::UHumanFishingAbilities()
{ PrimaryComponentTick.bCanEverTick=true; }
bool UHumanFishingAbilities::IsUnderwater() const
{
    const auto* Fish=Cast<ACharacter>(GetOwner());
    const auto* M=Fish?Fish->GetCharacterMovement():nullptr;
    return M && (M->IsFlying() || M->IsSwimming());
}
bool UHumanFishingAbilities::CanUseAbilities() const
{
    const auto* Fish=Cast<APawn>(GetOwner());
    const auto* PC=Fish?Cast<APlayerController>(Fish->GetController()):nullptr;
    const auto* UI=Fish?Fish->FindComponentByClass<UHumanFishingPlayerUI>():nullptr;
    const auto* I=AbilityInventory(this);
    return PC && PC->IsLocalController() && !PC->IsMoveInputIgnored() && UI &&
        !UI->IsShopOpen() && !UI->IsBackpackOpen() && !UGameplayStatics::IsGamePaused(this) &&
        I && I->GetHealth()>0.f && !I->HasActiveBaitUse() && IsUnderwater();
}
float UHumanFishingAbilities::GetSprintFraction() const
{ const auto* I=AbilityInventory(this);return I?I->SprintEnergy/FHumanFishingSprintState::Capacity:1.f; }
bool UHumanFishingAbilities::ShowSprintMeter() const
{const auto* I=AbilityInventory(this);return I && I->HasTidalFins() && IsUnderwater() && (bSprinting || I->SprintEnergy<4.99f);}
void UHumanFishingAbilities::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    auto* Fish=Cast<ACharacter>(GetOwner());
    auto* PC=Fish?Cast<APlayerController>(Fish->GetController()):nullptr;
    auto* I=AbilityInventory(this);
    if(!PC || !PC->IsLocalController() || !I)return;
    auto* M=Fish->GetCharacterMovement();
    if(!bCapturedSpeed)
    {BaseFlySpeed=M->MaxFlySpeed;BaseSwimSpeed=M->MaxSwimSpeed;bCapturedSpeed=true;}
    const bool Held=PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
    bSprinting=FHumanFishingSprintState::Advance(Delta,Held,CanUseAbilities() && I->HasTidalFins(),
        I->SprintEnergy,I->SprintRecoveryDelay,bMustRelease);
    M->MaxFlySpeed=BaseFlySpeed*GetWaterSpeedMultiplier();
    M->MaxSwimSpeed=BaseSwimSpeed*GetWaterSpeedMultiplier();
    if(PC->WasInputKeyJustPressed(EKeys::F))FireJellyshock();
    if(PC->WasInputKeyJustPressed(EKeys::G))FireVioletJellyshock();
}
void UHumanFishingAbilities::EndPlay(const EEndPlayReason::Type Reason)
{
    if(auto* Fish=Cast<ACharacter>(GetOwner()))if(bCapturedSpeed)
    {Fish->GetCharacterMovement()->MaxFlySpeed=BaseFlySpeed;Fish->GetCharacterMovement()->MaxSwimSpeed=BaseSwimSpeed;}
    bSprinting=false;Super::EndPlay(Reason);
}
void UHumanFishingAbilities::CancelSprint()
{
    bSprinting=false;bMustRelease=true;
    if(auto* Fish=Cast<ACharacter>(GetOwner()))if(bCapturedSpeed)
    {Fish->GetCharacterMovement()->MaxFlySpeed=BaseFlySpeed;Fish->GetCharacterMovement()->MaxSwimSpeed=BaseSwimSpeed;}
}
float UHumanFishingAbilities::JellyshockTargetScore(const FVector& Forward,const FVector& CameraDelta,float MuzzleDistance)
{
    if(MuzzleDistance>3000.f || MuzzleDistance<=0.f || CameraDelta.IsNearlyZero())return MAX_flt;
    const float Dot=FVector::DotProduct(Forward.GetSafeNormal(),CameraDelta.GetSafeNormal());
    if(Dot<FMath::Cos(FMath::DegreesToRadians(8.f)))return MAX_flt;
    return 1.f-Dot+MuzzleDistance*1.e-9f;
}
FVector UHumanFishingAbilities::PredictJellyshockPoint(const FVector& Origin,const FVector& Position,const FVector& Velocity)
{return Position+Velocity*FMath::Clamp(FVector::Distance(Origin,Position)/1500.f,0.f,.5f);}
bool UHumanFishingAbilities::ShowJellyshockReticle() const
{return CanUseAbilities();}
bool UHumanFishingAbilities::GetJellyshockAimPoint(FVector& WorldPoint,bool& bAssisted) const
{const auto Aim=QueryJellyshockAim();WorldPoint=Aim.Point;bAssisted=Aim.bAssisted;return ShowJellyshockReticle();}
FHumanFishingJellyshockAim UHumanFishingAbilities::QueryJellyshockAim(bool bForceRefresh) const
{
    const auto* Fish=Cast<APawn>(GetOwner());
    auto* PC=Fish?Cast<APlayerController>(Fish->GetController()):nullptr;
    if(!PC || !GetWorld())return {};
    const double Now=GetWorld()->GetTimeSeconds();
    if(!bForceRefresh && AimUpdatedAt>=0 && Now-AimUpdatedAt<.1)return CachedAim;
    AimUpdatedAt=Now;CachedAim={};
    FVector View;FRotator Facing;PC->GetPlayerViewPoint(View,Facing);
    const FVector Forward=Facing.Vector();
    CachedAim.Origin=Fish->GetActorLocation()+Forward*100.f;
    CachedAim.Point=View+Forward*3000.f;
    FHitResult Block;
    if(FirstJellyBlock(GetWorld(),View,CachedAim.Point,Fish,Block))CachedAim.Point=Block.ImpactPoint;
    CachedAim.bBlocked=FirstJellyBlock(GetWorld(),Fish->GetActorLocation(),CachedAim.Origin,Fish,Block,nullptr,50.f,true);
    const auto* Inventory=AbilityInventory(this);
    if(!CachedAim.bBlocked && CanUseAbilities() && Inventory && (Inventory->GetJellyshockCount()>0 || Inventory->GetVioletJellyshockCount()>0))
    {
        float Best=MAX_flt;
        for(TActorIterator<AHumanFishingShark> It(GetWorld());It;++It)
        {
            const FVector Position=It->GetActorLocation();
            const float Score=JellyshockTargetScore(Forward,Position-View,FVector::Distance(Position,CachedAim.Origin));
            if(Score>=Best || FirstJellyBlock(GetWorld(),View,Position,Fish,Block,*It) ||
                FirstJellyBlock(GetWorld(),CachedAim.Origin,Position,Fish,Block,*It))continue;
            Best=Score;CachedAim.Target=*It;CachedAim.bAssisted=true;
            CachedAim.Point=PredictJellyshockPoint(CachedAim.Origin,Position,It->GetVelocity());
        }
    }
    CachedAim.Direction=(CachedAim.Point-CachedAim.Origin).GetSafeNormal();
    if(CachedAim.Direction.IsNearlyZero())CachedAim.Direction=Forward;
    return CachedAim;
}
bool UHumanFishingAbilities::FireJellyshock()
{return FireJellyshockVariant(false);}
bool UHumanFishingAbilities::FireVioletJellyshock()
{return FireJellyshockVariant(true);}
bool UHumanFishingAbilities::FireJellyshockVariant(bool bViolet)
{
    auto* I=AbilityInventory(this);
    auto* UI=GetOwner()->FindComponentByClass<UHumanFishingPlayerUI>();
    if(!CanUseAbilities() || GetWorld()->GetTimeSeconds()<NextShotTime)return false;
    if((bViolet?I->GetVioletJellyshockCount():I->GetJellyshockCount())<=0)
    {
        if(UI)
        {
            UI->ShowAlert(bViolet?NSLOCTEXT("HFEquipment","RestockViolet","No Violet Jellyshock. Restock at the Seaweed Exchange."):
                NSLOCTEXT("HFEquipment","RestockShock","No Jellyshock Capsules. Restock at the Seaweed Exchange."),2.f);
        }
        return false;
    }
    auto* Fish=Cast<APawn>(GetOwner());
    const auto Aim=QueryJellyshockAim(true);
    if(Aim.bBlocked)
    {if(UI)UI->ShowAlert(NSLOCTEXT("HFEquipment","ShockBlocked","Move clear of the obstacle to launch Jellyshock."),1.5f);return false;}
    FActorSpawnParameters Params;Params.Owner=Fish;Params.Instigator=Fish;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Shot=GetWorld()->SpawnActor<AHumanFishingJellyshock>(Aim.Origin,Aim.Direction.Rotation(),Params);
    if(!Shot)return false;
    Shot->ConfigureVariant(bViolet);
    if(!(bViolet?I->ConsumeVioletJellyshock():I->ConsumeJellyshock())){Shot->Destroy();return false;}
    NextShotTime=GetWorld()->GetTimeSeconds()+.75;
    return true;
}
