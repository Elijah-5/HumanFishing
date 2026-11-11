#include "HumanFishingPlayerUI.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HumanFishingHUD.h"
#include "HumanFishingInventory.h"
#include "HumanFishingSave.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

UHumanFishingPlayerUI::UHumanFishingPlayerUI()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHumanFishingPlayerUI::BeginPlay()
{
    Super::BeginPlay();
    const FName Level(*UGameplayStatics::GetCurrentLevelName(this));
    bDeepOcean = Level == UHumanFishingSaveSubsystem::OceanLevel;
    if (auto* Save = GetSaveSubsystem())
    {
        Save->LevelReady(Level);
    }
}

APlayerController* UHumanFishingPlayerUI::GetLocalController() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    return Controller && Controller->IsLocalController() ? Controller : nullptr;
}

UHumanFishingInventory* UHumanFishingPlayerUI::GetInventory() const
{
    UGameInstance* Instance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    return Instance ? Instance->GetSubsystem<UHumanFishingInventory>() : nullptr;
}

UHumanFishingSaveSubsystem* UHumanFishingPlayerUI::GetSaveSubsystem() const
{
    UGameInstance* Instance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    return Instance ? Instance->GetSubsystem<UHumanFishingSaveSubsystem>() : nullptr;
}

void UHumanFishingPlayerUI::EnsureOverlay(APlayerController* Controller)
{
    ULocalPlayer* Player = Controller->GetLocalPlayer();
    if (!Overlay && Player && Player->ViewportClient)
    {
        Overlay = SNew(SHumanFishingHUD, this);
        OverlayPlayer = Player;
        Player->ViewportClient->AddViewportWidgetForPlayer(Player, Overlay.ToSharedRef(), 20);
    }
}

void UHumanFishingPlayerUI::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    APlayerController* Controller = GetLocalController();
    if (!Controller)
    {
        return;
    }

    EnsureOverlay(Controller);
    if (bDeepOcean)
    {
        if (UHumanFishingInventory* Inventory = GetInventory())
        {
            Inventory->ChangeHealth(OceanHealingPerSecond * DeltaTime);
        }
    }
    if (Controller->WasInputKeyJustPressed(EKeys::Tab))
    {
        SetBackpackOpen(true);
    }
}

bool UHumanFishingPlayerUI::AdvanceBeachHealth(bool bInWater, bool bGrounded)
{
    if (bDeepOcean || UGameplayStatics::IsGamePaused(this))
    {
        return false;
    }

    bOnBeach = !bInWater && (bOnBeach || bGrounded);
    UHumanFishingInventory* Inventory = GetInventory();
    if (!bOnBeach || !Inventory || bEvacuating)
    {
        return false;
    }

    const float DamageMultiplier = Inventory->HasMudskipper() ? .5f : 1.f;
    Inventory->ChangeHealth(-BeachDamagePerSecond * DamageMultiplier * GetWorld()->GetDeltaSeconds());
    bEvacuating = Inventory->GetHealth() <= 0.f;
    return bEvacuating;
}

bool UHumanFishingPlayerUI::TryStoreCatch(FName Type)
{
    UHumanFishingInventory* Inventory = GetInventory();
    const FHumanFishingCatchType* Definition = Catalog ? Catalog->Types.Find(Type) : nullptr;
    if (!Inventory || !Definition)
    {
        return false;
    }
    const bool bAlreadyCaught = Inventory->HasCaughtType(Type);
    if (!Inventory->TryAddCatch(Type, Definition->Size))
    {
        return false;
    }
    NewGuideType = bAlreadyCaught ? NAME_None : Type;
    return true;
}

void UHumanFishingPlayerUI::ShowCaptureToast(FName Type)
{
    ToastType = Type;
    ToastUntil = GetWorld()->GetTimeSeconds() + 2.5;
}

FName UHumanFishingPlayerUI::GetToastType() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() < ToastUntil ? ToastType : NAME_None;
}

bool UHumanFishingPlayerUI::IsNewGuideToast() const
{
    const auto* Save = GetSaveSubsystem();
    return !NewGuideType.IsNone() && NewGuideType == GetToastType() && (!Save || !Save->IsDeveloperMode());
}

void UHumanFishingPlayerUI::SetBackpackOpen(bool bOpen)
{
    APlayerController* Controller = GetLocalController();
    if ((bOpen && bShopOpen) || !Controller || !Overlay || bBackpackOpen == bOpen || !UGameplayStatics::SetGamePaused(this, bOpen))
    {
        return;
    }

    bBackpackOpen = bOpen;
    Controller->FlushPressedKeys();
    Controller->SetShowMouseCursor(bOpen);
    if (bOpen)
    {
        Overlay->ResetPausePage();
        AHUD* HUD = Controller->GetHUD();
        if (HUD && HUD->Implements<UHumanFishingBackpackListener>())
        {
            IHumanFishingBackpackListener::Execute_OnBackpackOpening(HUD);
        }
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(Overlay->GetPausePanel());
        Controller->SetInputMode(InputMode);
    }
    else
    {
        Controller->SetInputMode(FInputModeGameOnly());
    }
}

namespace
{
UHumanFishingInventory* ProgressInventory(const UObject* WorldContextObject)
{
    UGameInstance* Game = UGameplayStatics::GetGameInstance(WorldContextObject);
    return Game ? Game->GetSubsystem<UHumanFishingInventory>() : nullptr;
}
}

int32 UHumanFishingBlueprintLibrary::GetCaptureCount(const UObject* WorldContextObject)
{
    const auto* Inventory = ProgressInventory(WorldContextObject);
    return Inventory ? Inventory->GetLifetimeCatchCount() : 0;
}

int32 UHumanFishingBlueprintLibrary::GetSeaweedBalance(const UObject* WorldContextObject)
{
    const auto* Inventory = ProgressInventory(WorldContextObject);
    return Inventory ? Inventory->GetSeaweed() : 0;
}

void UHumanFishingPlayerUI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetFishingCamera();
    if (ULocalPlayer* Player = OverlayPlayer.Get())
    {
        if (Player->ViewportClient && Overlay)
        {
            Player->ViewportClient->RemoveViewportWidgetForPlayer(Player, Overlay.ToSharedRef());
        }
    }
    Overlay.Reset();
    Super::EndPlay(EndPlayReason);
}

void UHumanFishingPlayerUI::UpdateFishingCamera(bool bFishing)
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn)
    {
        return;
    }
    USpringArmComponent* Boom = FishingCameraBoom.Get();
    if (!Boom)
    {
        if (!bFishing)
        {
            return;
        }
        Boom = Pawn->FindComponentByClass<USpringArmComponent>();
        if (!Boom || !Boom->bUsePawnControlRotation || !Boom->bInheritPitch || Boom->IsUsingAbsoluteRotation())
        {
            return;
        }
        DefaultCameraRotation = Boom->GetRelativeRotation();
        FishingCameraBoom = Boom;
        FishingCameraBlend = 0.f;
        Boom->bInheritPitch = false;
    }

    constexpr float BlendSeconds = .4f;
    FishingCameraBlend = FMath::FInterpConstantTo(
        FishingCameraBlend, bFishing ? 1.f : 0.f, GetWorld()->GetDeltaSeconds(), 1.f / BlendSeconds);
    if (!bFishing && FishingCameraBlend <= 0.f)
    {
        ResetFishingCamera();
        return;
    }

    const float ViewPitch = FRotator::NormalizeAxis(Pawn->GetViewRotation().Pitch);
    const float FishingPitch = FMath::Min(ViewPitch,
        FMath::GetMappedRangeValueClamped(FVector2f(-55.f, 25.f), FVector2f(-55.f, -20.f), ViewPitch));
    FRotator Rotation = DefaultCameraRotation;
    Rotation.Pitch = FMath::Lerp(ViewPitch, FishingPitch, FMath::SmoothStep(0.f, 1.f, FishingCameraBlend));
    Boom->SetRelativeRotation(Rotation);
}

void UHumanFishingPlayerUI::ResetFishingCamera()
{
    if (USpringArmComponent* Boom = FishingCameraBoom.Get())
    {
        Boom->SetRelativeRotation(DefaultCameraRotation);
        Boom->bInheritPitch = true;
    }
    FishingCameraBoom.Reset();
    FishingCameraBlend = 0.f;
}

bool UHumanFishingBlueprintLibrary::UpdateBeachHealth(APawn* Fish, bool bInWater, bool bGrounded)
{
    UHumanFishingPlayerUI* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
    return UI && UI->AdvanceBeachHealth(bInWater, bGrounded);
}

bool UHumanFishingBlueprintLibrary::TryStoreCatch(APawn* Fish, FName Type)
{
    UHumanFishingPlayerUI* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
    return UI && UI->TryStoreCatch(Type);
}

void UHumanFishingBlueprintLibrary::ShowCaptureToast(APawn* Fish, FName Type)
{
    if (UHumanFishingPlayerUI* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr)
    {
        UI->ShowCaptureToast(Type);
    }
}

void UHumanFishingBlueprintLibrary::UpdateFishingCamera(APawn* Fish, bool bFishing)
{
    if (UHumanFishingPlayerUI* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr)
    {
        UI->UpdateFishingCamera(bFishing);
    }
}

void UHumanFishingBlueprintLibrary::ResetFishingCamera(APawn* Fish)
{
    if (UHumanFishingPlayerUI* UI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr)
    {
        UI->ResetFishingCamera();
    }
}
