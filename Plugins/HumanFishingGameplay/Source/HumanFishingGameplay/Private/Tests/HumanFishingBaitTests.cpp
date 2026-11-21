#if WITH_DEV_AUTOMATION_TESTS
#include "HumanFishingInventory.h"
#include "HumanFishingSave.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingBaitTransaction, "HumanFishing.Bait.Transactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHumanFishingBaitTransaction::RunTest(const FString& Parameters)
{
    auto* Game=NewObject<UGameInstance>(); auto* I=NewObject<UHumanFishingInventory>(Game);
    I->ToggleBait(); TestEqual(TEXT("Cycle starts with bill"), I->GetSelectedBait(), EHumanFishingBait::HundredDollarBill);
    TestFalse(TEXT("Zero stock cannot cast"),I->CanCastSelectedBait());
    TestFalse(TEXT("Zero stock cannot reserve"),I->ReserveSelectedBait());
    I->SetSeaweed(100);
    TestFalse(TEXT("No negative purchases"),I->BuyBait(EHumanFishingBait::Flash,-1));
    TestFalse(TEXT("No free ordinary bait purchases"),I->BuyBait(EHumanFishingBait::Dollar,10));
    TestFalse(TEXT("No oversized purchases"),I->BuyBait(EHumanFishingBait::Flash,MAX_int32));
    TestTrue(TEXT("Buy five bills"),I->BuyBait(EHumanFishingBait::HundredDollarBill,5));
    TestEqual(TEXT("Exact purchase cost"),I->GetSeaweed(),75);
    TestTrue(TEXT("Reserve"),I->ReserveSelectedBait());
    TestEqual(TEXT("Available count excludes reservation"),I->GetBaitCount(EHumanFishingBait::HundredDollarBill),4);
    TestEqual(TEXT("Unhooked reservation survives save as owned stock"),I->GetProgress().HundredDollarBillCount,5);
    TestFalse(TEXT("Cannot reserve twice"),I->ReserveSelectedBait());
    I->FinishBaitUse(true); I->FinishBaitUse(true);
    TestEqual(TEXT("Retrieve refunds exactly once"),I->GetBaitCount(EHumanFishingBait::HundredDollarBill),5);
    I->ReserveSelectedBait(); I->CommitHookedBait(); I->CommitHookedBait(); I->FinishBaitUse(true);
    TestEqual(TEXT("Hook commits once; retrieval cannot refund hooked bait"),I->GetBaitCount(EHumanFishingBait::HundredDollarBill),4);
    I->ReserveSelectedBait(); I->FinishBaitUse(false); I->FinishBaitUse(false);
    TestEqual(TEXT("Failed cast consumes once"),I->GetBaitCount(EHumanFishingBait::HundredDollarBill),3);
    I->ToggleBait(); TestEqual(TEXT("Flash is top tier in cycle"),I->GetSelectedBait(),EHumanFishingBait::Flash);
    TestTrue(TEXT("Empty Flash can remain selected"),I->GetBaitCount(I->GetSelectedBait())==0);
    TestTrue(TEXT("Buy Flash"),I->BuyBait(EHumanFishingBait::Flash,5));
    TestEqual(TEXT("Flash cost"),I->GetSeaweed(),25);
    I->ToggleBait(); TestEqual(TEXT("Cycle returns to Dollar"),I->GetSelectedBait(),EHumanFishingBait::Dollar);
    TestTrue(TEXT("Ordinary bait unlimited"),I->ReserveSelectedBait()); I->FinishBaitUse(false);
    TestEqual(TEXT("Unlimited sentinel"),I->GetBaitCount(EHumanFishingBait::Dollar),INDEX_NONE);
    TestTrue(TEXT("Progress valid"),I->GetProgress().IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingBaitMerchant, "HumanFishing.Bait.MerchantAndEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHumanFishingBaitMerchant::RunTest(const FString& Parameters)
{
    FHumanFishingCatchType Merchant; Merchant.bRequiresHundredDollarBill=true;
    TestFalse(TEXT("Merchant rejects Dollar"),Merchant.AcceptsBait(EHumanFishingBait::Dollar));
    TestTrue(TEXT("Merchant accepts bill"),Merchant.AcceptsBait(EHumanFishingBait::HundredDollarBill));
    TestTrue(TEXT("Merchant accepts Flash"),Merchant.AcceptsBait(EHumanFishingBait::Flash));
    const auto Flash=GetDefault<UHumanFishingCatalog>()->GetBaitDefinition(EHumanFishingBait::Flash);
    TestEqual(TEXT("Flash interest doubled"),Flash.InterestMultiplier,2.);
    TestEqual(TEXT("Flash bonus window"),Flash.InspectBonusSeconds,4.);
    TestTrue(TEXT("Flash prioritizes rarities"),Flash.bPrioritizeRarity);
    TestTrue(TEXT("Flash costs more"),UHumanFishingInventory::FlashBaitCost>UHumanFishingInventory::HundredDollarBillCost);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingCovePurchase,"HumanFishing.Shop.CoveKey",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHumanFishingCovePurchase::RunTest(const FString& Parameters)
{
    auto* Game=NewObject<UGameInstance>();auto* I=NewObject<UHumanFishingInventory>(Game);
    I->SetSeaweed(59);TestFalse(TEXT("Key needs 60"),I->BuyCoveKey());
    I->SetSeaweed(60);TestTrue(TEXT("Buy key"),I->BuyCoveKey());TestEqual(TEXT("Cost deducted"),I->GetSeaweed(),0);
    I->SetSeaweed(100);TestFalse(TEXT("No repeat key purchase"),I->BuyCoveKey());
    TestEqual(TEXT("Repeat purchase preserves balance"),I->GetSeaweed(),100);TestTrue(TEXT("Permanently owned"),I->HasCoveKey());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingSaveMigration,"HumanFishing.Save.V1Migration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHumanFishingSaveMigration::RunTest(const FString& Parameters)
{
    auto* Save=NewObject<UHumanFishingSaveGame>();Save->Version=1;Save->Level=UHumanFishingSaveSubsystem::CoveLevel;
    Save->Progress.Seaweed=42;Save->Progress.bHasHundredDollarBill=true;Save->Progress.bHasMudskipper=true;
    TestTrue(TEXT("Migration applied"),UHumanFishingSaveSubsystem::MigrateSave(Save));
    TestEqual(TEXT("Legacy bill refunded"),Save->Progress.Seaweed,72);
    TestTrue(TEXT("Existing Cove save grandfathered"),Save->Progress.bHasCoveKey);
    TestTrue(TEXT("Existing upgrade preserved"),Save->Progress.bHasMudskipper);
    TestEqual(TEXT("Advanced bait starts empty"),Save->Progress.FlashBaitCount,0);
    TestFalse(TEXT("Migration idempotent"),UHumanFishingSaveSubsystem::MigrateSave(Save));
    TestEqual(TEXT("No duplicate compensation"),Save->Progress.Seaweed,72);
    auto* Shore=NewObject<UHumanFishingSaveGame>();Shore->Version=1;Shore->Level=UHumanFishingSaveSubsystem::ShoreLevel;
    UHumanFishingSaveSubsystem::MigrateSave(Shore);TestFalse(TEXT("Shore save still needs key"),Shore->Progress.bHasCoveKey);
    auto* Full=NewObject<UHumanFishingSaveGame>();Full->Version=1;Full->Progress.bHasHundredDollarBill=true;Full->Progress.Seaweed=MAX_int32;
    UHumanFishingSaveSubsystem::MigrateSave(Full);TestEqual(TEXT("Compensation cannot overflow"),Full->Progress.Seaweed,MAX_int32);
    return true;
}
#endif
