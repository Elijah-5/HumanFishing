#include "HumanFishingJellyshock.h"
#include "HumanFishingShark.h"
#include "HumanFishingPlayerUI.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr int32 RingCount=3,ArcMaterial=3,BubbleMaterial=4,FlashMaterial=5;
void Decoration(UStaticMeshComponent* Part,USceneComponent* Parent,UStaticMesh* Mesh)
{
    Part->SetupAttachment(Parent);Part->SetStaticMesh(Mesh);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(false);Part->SetCastShadow(false);
}
}

AHumanFishingJellyshock::AHumanFishingJellyshock()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("CapsuleRoot"));
    Visual=CreateDefaultSubobject<USceneComponent>(TEXT("SwimmingJelly"));Visual->SetupAttachment(RootComponent);
    Visual->SetRelativeRotation(FRotator(0,90,0));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BellAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyBell.SM_JellyBell"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> InnerAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyInner.SM_JellyInner"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CoreAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyCore.SM_JellyCore"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> TentacleAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyTentacle.SM_JellyTentacle"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BubbleAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyBubble.SM_JellyBubble"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ArcAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyArc.SM_JellyArc"));
    Bell=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JellyBell"));Decoration(Bell,Visual,BellAsset.Object);Bell->SetTranslucentSortPriority(1);
    auto* Inner=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LuminousInner"));Decoration(Inner,Visual,InnerAsset.Object);
    for(int32 I=0;I<2;++I)
    {
        auto* Core=CreateDefaultSubobject<UStaticMeshComponent>(FName(*FString::Printf(TEXT("LightningCore%d"),I)));
        Decoration(Core,Visual,CoreAsset.Object);Core->SetRelativeRotation(FRotator(0,I*90,0));
    }
    for(int32 I=0;I<3;++I)
    {
        auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(FName(*FString::Printf(TEXT("CurledTentacle%d"),I)));
        Decoration(Part,Visual,TentacleAsset.Object);Part->SetRelativeLocation(FVector((I-1)*16,0,5));
        Part->SetRelativeScale3D(FVector(1,1,I==1?1.12f:.88f));Tentacles.Add(Part);
    }
    Trail=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("IonBubbleTrail"));Decoration(Trail,RootComponent,BubbleAsset.Object);
    Streaks=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ElectricWake"));Decoration(Streaks,RootComponent,ArcAsset.Object);
}
void AHumanFishingJellyshock::BeginPlay()
{
    Super::BeginPlay();SetLifeSpan(2.05f);
    for(int32 I=0;I<10;++I)
    {TrailPositions.Add(GetActorLocation());Trail->AddInstance(FTransform(FQuat::Identity,GetActorLocation(),FVector(.012f)),true);}
    for(int32 I=0;I<4;++I)Streaks->AddInstance(FTransform::Identity);
}
void AHumanFishingJellyshock::ConfigureVariant(bool bViolet)
{
    bLethal=bViolet;if(!bViolet)return;
    Visual->SetRelativeScale3D(FVector(1.2f));
    TArray<UStaticMeshComponent*> Parts;GetComponents(Parts);
    for(auto* Part:Parts)if(auto* Material=Part->CreateDynamicMaterialInstance(0))
    {
        const FString Name=Part->GetName();
        const FLinearColor Tint=Name.Contains(TEXT("LightningCore"))?FLinearColor(2.9f,1.5f,3.6f):
            Name.Contains(TEXT("Tentacle"))?FLinearColor(1.3f,.4f,2.4f):
            Name.Contains(TEXT("Inner"))?FLinearColor(.4f,.04f,.7f):FLinearColor(.65f,.12f,1.6f);
        Material->SetVectorParameterValue(TEXT("Tint"),Tint);
    }
}
void AHumanFishingJellyshock::Tick(float Delta)
{
    Super::Tick(Delta);Age+=Delta;
    const float Pulse=FMath::Sin(Age*11.f);
    Bell->SetRelativeScale3D(FVector(1+Pulse*.08f,1+Pulse*.08f,1-Pulse*.07f));
    Visual->SetRelativeRotation(FRotator(-12.f+Pulse*4.f,90,FMath::Sin(Age*4.f)*4.f));
    for(int32 I=0;I<Tentacles.Num();++I)
        Tentacles[I]->SetRelativeRotation(FRotator(FMath::Sin(Age*7.f+I*1.8f)*10.f,(I-1)*32.f,FMath::Cos(Age*5.f+I)*7.f));
    const float Step=FMath::Min(1500.f*Delta,3000.f-DistanceTravelled);
    const FVector Start=GetActorLocation(),End=Start+GetActorForwardVector()*Step;
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(JellyshockSweep),false,GetOwner());Query.AddIgnoredActor(this);
    TArray<FHitResult> Hits;
    GetWorld()->SweepMultiByObjectType(Hits,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(50.f),Query);
    Hits.Sort([](const FHitResult& A,const FHitResult& B){return A.Time<B.Time;});
    for(const auto& Hit:Hits)
    {
        if(auto* Shark=Cast<AHumanFishingShark>(Hit.GetActor()))
        {
            if(bLethal)
            {
                Shark->Destroy();
                if(auto* UI=GetOwner()?GetOwner()->FindComponentByClass<UHumanFishingPlayerUI>():nullptr)
                    UI->ShowAlert(NSLOCTEXT("HFEquipment","VioletDefeat","Shark defeated!"),2.f);
            }
            else Shark->ApplyStun(30.f);
            AHumanFishingJellyshockBurst::Spawn(GetWorld(),Hit.ImpactPoint,-GetActorForwardVector(),true,bLethal);
            Destroy();return;
        }
        if(Hit.GetComponent() && Hit.GetComponent()->GetCollisionResponseToChannel(ECC_WorldDynamic)==ECR_Block)
        {AHumanFishingJellyshockBurst::Spawn(GetWorld(),Hit.ImpactPoint,Hit.ImpactNormal,false,bLethal);Destroy();return;}
    }
    SetActorLocation(End);DistanceTravelled+=Step;TrailClock+=Delta;
    if(TrailClock>=.035f){TrailPositions.Insert(Start,0);TrailPositions.SetNum(10);TrailClock=0;}
    for(int32 I=0;I<10;++I)
    {
        const FVector Point=TrailPositions[I]+FVector(FMath::Sin(Age*8+I)*3,FMath::Cos(Age*9+I)*3,I*1.5f);
        Trail->UpdateInstanceTransform(I,FTransform(FQuat::Identity,Point,FVector(.012f+(9-I)*.002f)),true,false,true);
    }
    for(int32 I=0;I<4;++I)
    {
        const FVector From=TrailPositions[I*2],To=TrailPositions[I*2+2],Segment=To-From;
        Streaks->UpdateInstanceTransform(I,FTransform(Segment.Rotation(),From,FVector(Segment.Size()/100.f,.4f,.4f)),true,false,true);
    }
    Trail->MarkRenderStateDirty();Streaks->MarkRenderStateDirty();if(DistanceTravelled>=3000.f)Destroy();
}

AHumanFishingJellyshockBurst::AHumanFishingJellyshockBurst()
{
    PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("DischargeRoot"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RingAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyRing.SM_JellyRing"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ArcAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyArc.SM_JellyArc"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BubbleAsset(TEXT("/Game/HumanFishing/Equipment/Jellyshock/SM_JellyBubble.SM_JellyBubble"));
    for(int32 I=0;I<RingCount;++I)
    {
        auto* Ring=CreateDefaultSubobject<UStaticMeshComponent>(FName(*FString::Printf(TEXT("PressureRing%d"),I)));
        Decoration(Ring,RootComponent,RingAsset.Object);Rings.Add(Ring);
    }
    Arcs=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RadialLightning"));Decoration(Arcs,RootComponent,ArcAsset.Object);
    Bubbles=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DischargeBubbles"));Decoration(Bubbles,RootComponent,BubbleAsset.Object);
    Flash=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DischargeCore"));Decoration(Flash,RootComponent,BubbleAsset.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Gold(TEXT("/Game/HumanFishing/Equipment/Jellyshock/M_JellyGold.M_JellyGold"));
    Flash->SetMaterial(0,Gold.Object);
}
void AHumanFishingJellyshockBurst::Spawn(UWorld* World,const FVector& Position,const FVector& Normal,bool bHitShark,bool bViolet)
{
    int32 Count=0;AHumanFishingJellyshockBurst* Oldest=nullptr;
    for(TActorIterator<AHumanFishingJellyshockBurst> It(World);It;++It)
    {++Count;if(!Oldest || It->GetGameTimeSinceCreation()>Oldest->GetGameTimeSinceCreation())Oldest=*It;}
    if(Count>=4 && Oldest)Oldest->Destroy();
    if(auto* Effect=World->SpawnActor<AHumanFishingJellyshockBurst>(Position+Normal*4,FRotator::ZeroRotator))Effect->Configure(bHitShark,Normal,bViolet);
}
void AHumanFishingJellyshockBurst::Configure(bool bHitShark,const FVector& Normal,bool bInViolet)
{
    bStrong=bHitShark;bViolet=bInViolet;SetActorRotation(FRotationMatrix::MakeFromZ(Normal).Rotator());
    Rings[2]->SetVisibility(bViolet && bStrong);
    SetLifeSpan(bStrong?(bViolet?1.35f:1.1f):.55f);
    if(bViolet)
    {
        const FLinearColor Tints[]={FLinearColor(1.4f,.12f,3),FLinearColor(2.5f,1.5f,3.5f),FLinearColor(2.4f,.1f,1.6f),
            FLinearColor(1.5f,.35f,3.2f),FLinearColor(.9f,.3f,1.8f),FLinearColor(3.5f,2.5f,4)};
        for(int32 I=0;I<Materials.Num();++I)if(Materials[I])Materials[I]->SetVectorParameterValue(TEXT("Tint"),Tints[I]);
    }
}
void AHumanFishingJellyshockBurst::BeginPlay()
{
    Super::BeginPlay();SetLifeSpan(1.1f);
    for(auto* Part:{static_cast<UStaticMeshComponent*>(Rings[0].Get()),Rings[1].Get(),Rings[2].Get(),
        static_cast<UStaticMeshComponent*>(Arcs.Get()),static_cast<UStaticMeshComponent*>(Bubbles.Get()),Flash.Get()})
        Materials.Add(Part->CreateDynamicMaterialInstance(0));
    if(Materials[1])Materials[1]->SetVectorParameterValue(TEXT("Tint"),FLinearColor(2.5f,1.1f,.14f));
    Rings[2]->SetVisibility(false);
    for(int32 I=0;I<16;++I)Arcs->AddInstance(FTransform::Identity);
    for(int32 I=0;I<28;++I)Bubbles->AddInstance(FTransform::Identity);
    Tick(0.f);
}
void AHumanFishingJellyshockBurst::Tick(float Delta)
{
    Super::Tick(Delta);Age+=Delta;
    const bool bNova=bViolet && bStrong;
    const float Duration=bStrong?(bNova?1.35f:1.1f):.55f,T=FMath::Clamp(Age/Duration,0.f,1.f);
    const float Radius=(bStrong?(bNova?550.f:420.f):125.f)*FMath::InterpEaseOut(0.f,1.f,T,2.f)+20.f;
    const float Fade=FMath::Square(1.f-T);
    for(int32 I=0;I<Materials.Num();++I)if(Materials[I])
        Materials[I]->SetScalarParameterValue(TEXT("Strength"),I==FlashMaterial?FMath::Max(0.f,1.f-Age/(bNova?.22f:.18f)):Fade*(I==ArcMaterial?(bNova?2.6f:1.8f):1.f));
    for(int32 I=0;I<RingCount;++I)
    {
        Rings[I]->SetRelativeScale3D(FVector(Radius/100.f*(1.f-I*.15f)));
        Rings[I]->SetRelativeRotation(FRotator(I*65.f,Age*(I?-35:25),I*25.f));
    }
    Flash->SetRelativeScale3D(FVector((bNova?.35f:bStrong?.22f:.09f)+FMath::Min(Age,.18f)*2.f));
    Flash->SetVisibility(Age<(bNova?.20f:.16f));
    for(int32 I=0;I<16;++I)
    {
        const FRotator Angle(FMath::Sin(I*2.1f)*45.f,I*(bNova?22.5f:45.f)+FMath::Sin(Age*40+I)*7.f,Age*80+I*31.f);
        const float Length=Radius*.65f*(.85f+FMath::Sin(Age*35+I)*.15f);
        const FVector Scale=I<(bNova?16:8)?FVector(Length/100.f,1.f+Fade*(bNova?3.f:2.f),1.f+Fade*(bNova?3.f:2.f)):FVector::ZeroVector;
        Arcs->UpdateInstanceTransform(I,FTransform(Angle,FVector::ZeroVector,Scale),false,false,true);
    }
    for(int32 I=0;I<28;++I)
    {
        const FVector Direction=FRotator(-65.f+(I%6)*26.f,I*137.5f,0).Vector();
        const FVector Point=Direction*(20.f+Radius*(.25f+(I%5)*.1f))+FVector(0,0,Age*35);
        const FVector Scale=I<(bNova?28:18)?FVector((.045f+(I%4)*.015f)*(1.f-T*.5f)):FVector::ZeroVector;
        Bubbles->UpdateInstanceTransform(I,FTransform(FQuat::Identity,Point,Scale),false,false,true);
    }
    Arcs->MarkRenderStateDirty();Bubbles->MarkRenderStateDirty();
}
