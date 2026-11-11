#include "HumanFishingShopMenu.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "HumanFishingShop"

UHumanFishingPlayerUI* UHumanFishingShopMenu::GetPlayerUI() const
{
    const APawn* Pawn = GetOwningPlayerPawn();
    return Pawn ? Pawn->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
}

void UHumanFishingShopMenu::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SellAllButton->OnClicked.AddDynamic(this, &UHumanFishingShopMenu::RedeemAll);
    CloseButton->OnClicked.AddDynamic(this, &UHumanFishingShopMenu::CloseShop);
    MudskipperButton->OnClicked.AddDynamic(this, &UHumanFishingShopMenu::BuyMudskipper);
}

void UHumanFishingShopMenu::NativeConstruct()
{
    Super::NativeConstruct();
    if (auto* UI = GetPlayerUI())
    {
        UI->SetShopOpen(true);
    }
    Refresh();
}

void UHumanFishingShopMenu::NativeDestruct()
{
    if (auto* UI = GetPlayerUI())
    {
        UI->SetShopOpen(false);
    }
    Super::NativeDestruct();
}

void UHumanFishingShopMenu::CloseShop()
{
    if (!IsInViewport())
    {
        return;
    }
    if (auto* UI = GetPlayerUI())
    {
        UI->SetShopOpen(false);
    }
    if (auto* Controller = GetOwningPlayer())
    {
        Controller->SetIgnoreMoveInput(false);
        Controller->SetIgnoreLookInput(false);
        Controller->SetShowMouseCursor(false);
        Controller->FlushPressedKeys();
        Controller->SetInputMode(FInputModeGameOnly());
    }
    RemoveFromParent();
}

FReply UHumanFishingShopMenu::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::E || Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Tab)
    {
        CloseShop();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UHumanFishingShopMenu::RedeemAll()
{
    if (const auto* UI = GetPlayerUI())
    {
        if (auto* Inventory = UI->GetInventory())
        {
            Inventory->RedeemCatches(UI->GetCatalog());
        }
    }
    Refresh();
}

void UHumanFishingShopMenu::BuyMudskipper()
{
    if (const auto* UI = GetPlayerUI())
    {
        if (auto* Inventory = UI->GetInventory())
        {
            Inventory->BuyMudskipper();
        }
    }
    Refresh();
}

void UHumanFishingShopMenu::Refresh()
{
    const auto* UI = GetPlayerUI();
    const auto* Inventory = UI ? UI->GetInventory() : nullptr;
    const int32 Count = Inventory ? Inventory->GetCatches().Num() : 0;
    const int32 Seaweed = Inventory ? Inventory->GetSeaweed() : 0;
    const int32 Payout = Inventory ? Inventory->GetRedemptionValue(UI->GetCatalog()) : 0;

    CapturesText->SetText(FText::Format(LOCTEXT("InBag", "In bag: {0}"), Count));
    SeaweedText->SetText(FText::Format(LOCTEXT("Seaweed", "Seaweed: {0}"), Seaweed));
    Rate->SetText(Count > 0
        ? FText::Format(LOCTEXT("TotalQuote", "Redeem all catches for {0} seaweed."), Payout)
        : LOCTEXT("Empty", "Bring catches from your backpack to trade for seaweed."));
    SellAllButton->SetIsEnabled(Payout > 0);

    const bool bOwned = Inventory && Inventory->HasMudskipper();
    MudskipperButton->SetIsEnabled(!bOwned && Seaweed >= UHumanFishingInventory::MudskipperCost);
    MudskipperPrice->SetText(bOwned
        ? LOCTEXT("Owned", "Owned")
        : FText::Format(LOCTEXT("UpgradePrice", "{0} seaweed"), UHumanFishingInventory::MudskipperCost));
}

#undef LOCTEXT_NAMESPACE
