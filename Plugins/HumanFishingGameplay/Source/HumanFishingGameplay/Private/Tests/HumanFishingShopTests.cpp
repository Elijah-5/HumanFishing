#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "HumanFishingInventory.h"
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
#endif
