#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "HumanFishingInventory.h"
#include "HumanFishingShark.h"
#include "Misc/AutomationTest.h"

namespace
{
using EState = EHumanFishingSharkState;

FHumanFishingSharkSenses See(float Distance, bool bAvailable = true, bool bCoolingDown = false)
{
    FHumanFishingSharkSenses Senses;
    Senses.bTargetAvailable = bAvailable;
    Senses.TargetDistance = Distance;
    Senses.bCoolingDown = bCoolingDown;
    return Senses;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingSharkStates,
    "HumanFishing.Shark.States", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHumanFishingSharkStates::RunTest(const FString& Parameters)
{
    FHumanFishingSharkRanges Ranges;
    Ranges.DetectionRadius = 1000.f;
    Ranges.LoseRadius = 1500.f;
    Ranges.ContactRadius = 150.f;
    bool bSteal = false;

    TestEqual(TEXT("Patrols while the fish is far"), AHumanFishingShark::ChooseState(EState::Patrol, See(2000.f), Ranges, bSteal), EState::Patrol);
    TestEqual(TEXT("Chases inside detection radius"), AHumanFishingShark::ChooseState(EState::Patrol, See(1000.f), Ranges, bSteal), EState::Chase);
    TestFalse(TEXT("Spotting the fish steals nothing"), bSteal);
    TestEqual(TEXT("Patrol between radii does not start a chase"), AHumanFishingShark::ChooseState(EState::Patrol, See(1200.f), Ranges, bSteal), EState::Patrol);
    TestEqual(TEXT("Chase continues between radii"), AHumanFishingShark::ChooseState(EState::Chase, See(1200.f), Ranges, bSteal), EState::Chase);
    TestEqual(TEXT("Chase continues at lose radius"), AHumanFishingShark::ChooseState(EState::Chase, See(1500.f), Ranges, bSteal), EState::Chase);
    TestEqual(TEXT("Gives up beyond lose radius"), AHumanFishingShark::ChooseState(EState::Chase, See(1501.f), Ranges, bSteal), EState::Patrol);
    TestFalse(TEXT("Giving up steals nothing"), bSteal);

    TestEqual(TEXT("Contact during chase retreats"), AHumanFishingShark::ChooseState(EState::Chase, See(150.f), Ranges, bSteal), EState::Retreat);
    TestTrue(TEXT("Contact during chase steals"), bSteal);
    TestEqual(TEXT("Contact while patrolling starts a chase first"), AHumanFishingShark::ChooseState(EState::Patrol, See(50.f), Ranges, bSteal), EState::Chase);
    TestFalse(TEXT("Theft only happens during a chase"), bSteal);

    TestEqual(TEXT("Cooldown ignores a nearby fish"), AHumanFishingShark::ChooseState(EState::Retreat, See(50.f, true, true), Ranges, bSteal), EState::Retreat);
    TestFalse(TEXT("Cooldown steals nothing"), bSteal);
    TestEqual(TEXT("Patrols again after cooldown"), AHumanFishingShark::ChooseState(EState::Retreat, See(2000.f), Ranges, bSteal), EState::Patrol);
    TestEqual(TEXT("Chases again after cooldown if close"), AHumanFishingShark::ChooseState(EState::Retreat, See(500.f), Ranges, bSteal), EState::Chase);

    TestEqual(TEXT("Drops the chase when the fish is unavailable"), AHumanFishingShark::ChooseState(EState::Chase, See(100.f, false), Ranges, bSteal), EState::Patrol);
    TestFalse(TEXT("Unavailable fish cannot be robbed"), bSteal);

    Ranges.LoseRadius = 500.f;
    TestEqual(TEXT("Lose radius never shrinks below detection"), AHumanFishingShark::ChooseState(EState::Chase, See(900.f), Ranges, bSteal), EState::Chase);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingSharkTheft,
    "HumanFishing.Shark.Theft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHumanFishingSharkTheft::RunTest(const FString& Parameters)
{
    auto* Game = NewObject<UGameInstance>();
    auto* Inventory = NewObject<UHumanFishingInventory>(Game);

    TestEqual(TEXT("Empty backpack loses nothing"), Inventory->StealCatches(), 0);
    Inventory->SetSeaweed(30);
    TestTrue(TEXT("Store first catch"), Inventory->TryAddCatch(TEXT("Tourist"), FIntPoint(1, 2)));
    TestTrue(TEXT("Store second catch"), Inventory->TryAddCatch(TEXT("Farmer"), FIntPoint(2, 2)));

    TestEqual(TEXT("Steals every catch"), Inventory->StealCatches(), 2);
    TestEqual(TEXT("Backpack emptied"), Inventory->GetCatches().Num(), 0);
    TestEqual(TEXT("Seaweed untouched"), Inventory->GetSeaweed(), 30);
    TestEqual(TEXT("Lifetime captures preserved"), Inventory->GetLifetimeCatchCount(), 2);
    TestTrue(TEXT("Field Guide preserved"), Inventory->HasCaughtType(TEXT("Farmer")));
    TestEqual(TEXT("Second theft finds nothing"), Inventory->StealCatches(), 0);
    TestTrue(TEXT("Backpack usable after theft"), Inventory->TryAddCatch(TEXT("Tourist"), FIntPoint(1, 2)));
    TestTrue(TEXT("Progress remains valid"), Inventory->GetProgress().IsValid());
    return true;
}

#endif
