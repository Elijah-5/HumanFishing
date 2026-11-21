#include "HumanFishingTravelZone.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "HumanFishingSave.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHumanFishingTravelZone::AHumanFishingTravelZone()
{
    PrimaryActorTick.bCanEverTick = true;
    InputPriority = 100;
    TravelVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TravelVolume"));
    RootComponent = TravelVolume;
    TravelVolume->SetBoxExtent(FVector(180, 280, 230));
    TravelVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TravelVolume->SetCanEverAffectNavigation(false);
    Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DestinationSign"));
    Sign->SetupAttachment(RootComponent);
    Sign->SetRelativeLocation(FVector(0, 0, 210));
    Sign->SetWorldSize(38);
    Sign->SetHorizontalAlignment(EHTA_Center);
    Sign->SetTextRenderColor(FColor(95, 220, 205));
    Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EntryBeacon"));
    Beacon->SetupAttachment(RootComponent);
    Beacon->SetStaticMesh(nullptr);
    Beacon->SetVisibility(false);Beacon->SetHiddenInGame(true);
    Beacon->SetRelativeScale3D(FVector(.38f));
    Beacon->SetRelativeLocation(FVector(0, 0, 140));
    Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Beacon->SetCanEverAffectNavigation(false);
    RangeHighlight=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteractionRangeHighlight"));
    RangeHighlight->SetupAttachment(RootComponent);
    RangeHighlight->SetStaticMesh(nullptr);RangeHighlight->SetHiddenInGame(true);
    RangeHighlight->SetCollisionEnabled(ECollisionEnabled::NoCollision);RangeHighlight->SetCanEverAffectNavigation(false);
    RangeHighlight->SetVisibility(false);
}

void AHumanFishingTravelZone::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    // Clear old serialized components too: the map may predate this version.
    Beacon->SetStaticMesh(nullptr);Beacon->SetVisibility(false);Beacon->SetHiddenInGame(true);
    RangeHighlight->SetStaticMesh(nullptr);RangeHighlight->SetVisibility(false);RangeHighlight->SetHiddenInGame(true);
    Sign->SetText(DestinationLabel);
}

void AHumanFishingTravelZone::BeginPlay()
{
    Super::BeginPlay();
    Beacon->SetStaticMesh(nullptr);Beacon->SetVisibility(false);Beacon->SetHiddenInGame(true);
    RangeHighlight->SetStaticMesh(nullptr);RangeHighlight->SetVisibility(false);RangeHighlight->SetHiddenInGame(true);
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        EnableInput(PC);
        InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AHumanFishingTravelZone::Interact).bConsumeInput = true;
        DisableInput(PC);
    }
}

bool AHumanFishingTravelZone::IsInRange(const APawn* Fish) const
{
    if (!Fish || DestinationMap.IsNull()) return false;
    const FVector P = TravelVolume->GetComponentTransform().InverseTransformPosition(Fish->GetActorLocation()).GetAbs();
    const FVector E = TravelVolume->GetUnscaledBoxExtent();
    return P.X <= E.X && P.Y <= E.Y && P.Z <= E.Z;
}

bool AHumanFishingTravelZone::IsLocked(const APawn* Fish) const
{
    const auto* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
    const auto* Inventory = UI ? UI->GetInventory() : nullptr;
    return DestinationMap.GetAssetName() == TEXT("L_ScenicCove") && (!Inventory || !Inventory->HasCoveKey());
}

FText AHumanFishingTravelZone::GetPrompt(const APawn* Fish) const
{
    return IsLocked(Fish)
        ? NSLOCTEXT("HumanFishingTravel", "CoveLockedV3", "Scenic Cove — Key required\nVisit the Seaweed Exchange")
        : FText::Format(NSLOCTEXT("HumanFishingTravel", "TravelPrompt", "E  Travel to {0}"), DestinationLabel);
}

AHumanFishingTravelZone* AHumanFishingTravelZone::FindNearby(const APawn* Fish)
{
    if (!Fish) return nullptr;
    AHumanFishingTravelZone* Best = nullptr;
    double BestDistance = TNumericLimits<double>::Max();
    for (TActorIterator<AHumanFishingTravelZone> It(Fish->GetWorld()); It; ++It)
    {
        const double Distance = FVector::DistSquared(Fish->GetActorLocation(), It->GetActorLocation());
        if (It->IsInRange(Fish) && Distance < BestDistance) { Best = *It; BestDistance = Distance; }
    }
    return Best;
}

void AHumanFishingTravelZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Fish = UGameplayStatics::GetPlayerPawn(this, 0);
    auto* PC = UGameplayStatics::GetPlayerController(this, 0);
    const auto* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
    const bool bNearby = PC && FindNearby(Fish) == this && UI && !UI->IsBackpackOpen() && !UI->IsShopOpen();
    if (bNearby != bInputEnabled && PC)
    {
        if (bNearby) EnableInput(PC); else DisableInput(PC);
        bInputEnabled = bNearby;
    }
    Sign->SetText(FText::Format(NSLOCTEXT("HumanFishingTravel", "DestinationSign", "{0}{1}\n{2} m"),
        DestinationLabel, IsLocked(Fish) ? FText::FromString(TEXT(" [LOCKED]")) : FText::GetEmpty(),
        FText::AsNumber(Fish ? FMath::RoundToInt(FVector::Distance(Fish->GetActorLocation(), GetActorLocation()) / 100.) : 0)));
    Sign->SetTextRenderColor(IsLocked(Fish) ? FColor(250, 184, 84) : FColor(95, 220, 205));
    if (PC && PC->PlayerCameraManager)
        Sign->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation() - Sign->GetComponentLocation()).Rotation());
    Sign->SetWorldSize(bNearby?42.f:34.f);
}

void AHumanFishingTravelZone::Interact()
{
    auto* Fish = UGameplayStatics::GetPlayerPawn(this, 0);
    auto* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
    if (bTravelling || !IsInRange(Fish) || !UI || UI->IsShopOpen() || UI->IsBackpackOpen()) return;
    if (IsLocked(Fish)) { UI->ShowAlert(GetPrompt(Fish)); return; }
    if (auto* Inventory = UI->GetInventory()) Inventory->FinishBaitUse(true);
    if (auto* Save = UI->GetSaveSubsystem())
    {
        if (!Save->Flush()) { UI->ShowAlert(NSLOCTEXT("HumanFishingTravel", "SaveError", "Could not save. Please try again.")); return; }
    }
    bTravelling = true;
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
        if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0, 1, .25f, FLinearColor::Black, false, true);
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, this, &AHumanFishingTravelZone::CompleteTravel, .25f, false);
}

void AHumanFishingTravelZone::CompleteTravel()
{
    const FString Options = ArrivalTag.IsNone() ? FString() : TEXT("Arrival=") + ArrivalTag.ToString();
    UGameplayStatics::OpenLevel(this, FName(*DestinationMap.GetLongPackageName()), true, Options);
}
