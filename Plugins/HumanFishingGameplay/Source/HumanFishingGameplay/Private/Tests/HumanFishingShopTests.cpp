#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "HumanFishingInventory.h"
#include "HumanFishingSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingShopRedemption,
    "HumanFishing.Shop.Redemption", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHumanFishingShopRedemption::RunTest(const FString& Parameters)
{
    auto* Game = NewObject<UGameInstance>();
    auto* Inventory = NewObject<UHumanFishingInventory>(Game);
    auto* Catalog = NewObject<UHumanFishingCatalog>();
    Catalog->Types.Add(TEXT("Tourist")).Rarity = EHumanFishingRarity::White;
    Catalog->Types.Add(TEXT("Photographer")).Rarity = EHumanFishingRarity::Gold;

    TestEqual(TEXT("Empty redemption"), Inventory->RedeemCatches(Catalog), 0);
    TestTrue(TEXT("Store first catch"), Inventory->TryAddCatch(TEXT("Tourist"), FIntPoint(1, 2)));
    TestTrue(TEXT("Store rare catch"), Inventory->TryAddCatch(TEXT("Photographer"), FIntPoint(3, 2)));
    TestEqual(TEXT("Quote uses catch sizes and rarities"), Inventory->GetRedemptionValue(Catalog), 50);
    TestEqual(TEXT("Quote does not change backpack"), Inventory->GetCatches().Num(), 2);
    TestEqual(TEXT("Sell all catches"), Inventory->RedeemCatches(Catalog), 50);
    TestEqual(TEXT("Balance"), Inventory->GetSeaweed(), 50);
    TestEqual(TEXT("Backpack emptied"), Inventory->GetCatches().Num(), 0);
    TestEqual(TEXT("Lifetime captures preserved"), Inventory->GetLifetimeCatchCount(), 2);
    TestTrue(TEXT("Field Guide preserved"), Inventory->HasCaughtType(TEXT("Photographer")));
    TestEqual(TEXT("Repeated redemption pays nothing"), Inventory->RedeemCatches(Catalog), 0);
    TestEqual(TEXT("Balance unchanged"), Inventory->GetSeaweed(), 50);

    Inventory->TryAddCatch(TEXT("Tourist"), FIntPoint(1, 2));
    Inventory->TryAddCatch(TEXT("Unknown"), FIntPoint(1, 1));
    TestEqual(TEXT("Unknown profile rejects entire bulk sale"), Inventory->RedeemCatches(Catalog), 0);
    TestEqual(TEXT("Failed bulk sale keeps all catches"), Inventory->GetCatches().Num(), 2);
    TestEqual(TEXT("Missing catalog rejected"), Inventory->RedeemCatches(nullptr), 0);
    TestTrue(TEXT("Progress remains valid"), Inventory->GetProgress().IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingMudskipperPurchase,
    "HumanFishing.Shop.Mudskipper", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHumanFishingMudskipperPurchase::RunTest(const FString& Parameters)
{
    auto* Game = NewObject<UGameInstance>();
    auto* Inventory = NewObject<UHumanFishingInventory>(Game);
    TestFalse(TEXT("New progress has no Mudskipper"), Inventory->HasMudskipper());
    Inventory->SetSeaweed(UHumanFishingInventory::MudskipperCost - 1);
    TestFalse(TEXT("Insufficient funds cannot buy upgrade"), Inventory->BuyMudskipper());
    TestEqual(TEXT("Failed purchase keeps balance"), Inventory->GetSeaweed(), 24);
    TestFalse(TEXT("Failed purchase keeps upgrade locked"), Inventory->HasMudskipper());
    Inventory->SetSeaweed(UHumanFishingInventory::MudskipperCost);
    TestTrue(TEXT("Exact funds buy upgrade"), Inventory->BuyMudskipper());
    TestEqual(TEXT("Purchase deducts price"), Inventory->GetSeaweed(), 0);
    TestTrue(TEXT("Upgrade owned"), Inventory->HasMudskipper());
    Inventory->SetSeaweed(200);
    TestFalse(TEXT("Upgrade cannot be bought twice"), Inventory->BuyMudskipper());
    TestEqual(TEXT("Repeat purchase keeps balance"), Inventory->GetSeaweed(), 200);
    TestTrue(TEXT("Progress remains valid"), Inventory->GetProgress().IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingSingleSale,
    "HumanFishing.Shop.SingleSale",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHumanFishingSingleSale::RunTest(const FString& Parameters)
{
    auto* Game=NewObject<UGameInstance>();auto* I=NewObject<UHumanFishingInventory>(Game);
    auto* Catalog=NewObject<UHumanFishingCatalog>();Catalog->Types.Add(TEXT("Tourist")).Rarity=EHumanFishingRarity::White;
    I->TryAddCatch(TEXT("Tourist"),FIntPoint(1,2));I->TryAddCatch(TEXT("Tourist"),FIntPoint(1,2));
    const int32 Id=I->GetCatches()[0].Id;
    TestEqual(TEXT("One selected catch pays its exact value"),I->RedeemCatch(Catalog,Id),2);
    TestEqual(TEXT("Other catch remains"),I->GetCatches().Num(),1);
    TestEqual(TEXT("No duplicate payout"),I->RedeemCatch(Catalog,Id),0);
    TestEqual(TEXT("No unknown catch payout"),I->RedeemCatch(Catalog,MAX_int32),0);
    TestEqual(TEXT("Lifetime count preserved"),I->GetLifetimeCatchCount(),2);
    TestTrue(TEXT("Guide discovery preserved"),I->HasCaughtType(TEXT("Tourist")));
    const int32 Remaining=I->GetCatches()[0].Id;I->SetSeaweed(MAX_int32);
    TestEqual(TEXT("Overflow rejects sale"),I->RedeemCatch(Catalog,Remaining),0);
    TestEqual(TEXT("Overflow keeps catch"),I->GetCatches().Num(),1);
    TestEqual(TEXT("Missing catalog keeps catch"),I->RedeemCatch(nullptr,Remaining),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingBackpackUpgrade,
    "HumanFishing.Shop.BackpackUpgrade", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHumanFishingBackpackUpgrade::RunTest(const FString& Parameters)
{
    auto* Game = NewObject<UGameInstance>();
    auto* I = NewObject<UHumanFishingInventory>(Game);
    TestEqual(TEXT("Default width"), I->GetColumns(), 4);
    TestFalse(TEXT("No free expansion"), I->BuyBackpackUpgrade());
    TestTrue(TEXT("Fill original backpack"), I->TryAddCatch(TEXT("Tourist"), FIntPoint(4,4)));
    TestFalse(TEXT("Original backpack full"), I->TryAddCatch(TEXT("Tourist"), FIntPoint(1,1)));
    I->SetSeaweed(UHumanFishingInventory::BackpackUpgradeCost);
    TestTrue(TEXT("Buy expansion"), I->BuyBackpackUpgrade());
    TestEqual(TEXT("Price deducted"), I->GetSeaweed(), 0);
    TestEqual(TEXT("Expanded width"), I->GetColumns(), 5);
    TestEqual(TEXT("Existing catch stays in place"), I->GetCatches()[0].Cell, FIntPoint::ZeroValue);
    TestEqual(TEXT("Load uses expanded capacity"), I->GetLoadFraction(), .8f);
    TestTrue(TEXT("New column usable"), I->TryAddCatch(TEXT("Tourist"), FIntPoint(1,4)));
    TestEqual(TEXT("New catch fills fifth column"), I->GetCatches()[1].Cell, FIntPoint(4,0));
    TestFalse(TEXT("Twenty cells is full"), I->TryAddCatch(TEXT("Tourist"), FIntPoint(1,1)));
    I->SetSeaweed(100);
    TestFalse(TEXT("Expansion cannot be bought twice"), I->BuyBackpackUpgrade());
    TestEqual(TEXT("Repeat purchase preserves balance"), I->GetSeaweed(), 100);
    auto* Save = NewObject<UHumanFishingSaveGame>();
    Save->Version = UHumanFishingSaveSubsystem::CurrentSaveVersion;
    Save->Progress = I->GetProgress();
    TArray<uint8> Bytes;
    TestTrue(TEXT("Serialize expanded progress"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Restored = Cast<UHumanFishingSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Deserialize expanded progress"), Restored))
    {
        TestTrue(TEXT("Expansion persists"), Restored->Progress.bHasBackpackUpgrade);
        TestEqual(TEXT("Width persists"), Restored->Progress.GetColumns(), 5);
        TestTrue(TEXT("Expanded saved catches valid"), Restored->Progress.IsValid());
        Restored->Progress.bHasBackpackUpgrade = false;
        TestFalse(TEXT("Fifth column requires purchased expansion"), Restored->Progress.IsValid());
    }
    return true;
}
#endif
