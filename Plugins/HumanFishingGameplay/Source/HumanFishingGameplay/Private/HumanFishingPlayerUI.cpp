#include "HumanFishingPlayerUI.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HumanFishingHUD.h"
#include "HumanFishingInventory.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

UHumanFishingPlayerUI::UHumanFishingPlayerUI()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHumanFishingPlayerUI::BeginPlay()
{
    Super::BeginPlay();
    bDeepOcean = UGameplayStatics::GetCurrentLevelName(this) == TEXT("L_DeepOcean");
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

    Inventory->ChangeHealth(-BeachDamagePerSecond * GetWorld()->GetDeltaSeconds());
    bEvacuating = Inventory->GetHealth() <= 0.f;
    return bEvacuating;
}

bool UHumanFishingPlayerUI::TryStoreCatch(FName Type)
{
    UHumanFishingInventory* Inventory = GetInventory();
    const FHumanFishingCatchType* Definition = Catalog ? Catalog->Types.Find(Type) : nullptr;
    return Inventory && Definition && Inventory->TryAddCatch(Type, Definition->Size);
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

void UHumanFishingPlayerUI::SetBackpackOpen(bool bOpen)
{
    APlayerController* Controller = GetLocalController();
    if (!Controller || !Overlay || bBackpackOpen == bOpen || !UGameplayStatics::SetGamePaused(this, bOpen))
    {
        return;
    }

    bBackpackOpen = bOpen;
    Controller->FlushPressedKeys();
    Controller->SetShowMouseCursor(bOpen);
    if (bOpen)
    {
        AHUD* HUD = Controller->GetHUD();
        if (HUD && HUD->Implements<UHumanFishingBackpackListener>())
        {
            IHumanFishingBackpackListener::Execute_OnBackpackOpening(HUD);
        }
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(Overlay->GetBackpack());
        Controller->SetInputMode(InputMode);
    }
    else
    {
        Controller->SetInputMode(FInputModeGameOnly());
    }
}

void UHumanFishingPlayerUI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
