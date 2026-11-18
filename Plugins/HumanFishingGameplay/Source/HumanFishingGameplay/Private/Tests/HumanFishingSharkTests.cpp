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
    auto* Catalog = NewObject<UHumanFishingCatalog>();
    Catalog->Types.Add(TEXT("Tourist")).Rarity = EHumanFishingRarity::White;
    Catalog->Types.Add(TEXT("Farmer")).Rarity = EHumanFishingRarity::Green;
    Catalog->Types.Add(TEXT("Photographer")).Rarity = EHumanFishingRarity::Gold;
    FHumanFishingCatch Stolen;

    TestFalse(TEXT("Empty backpack loses nothing"), Inventory->StealMostValuableCatch(Catalog, Stolen));
    TestEqual(TEXT("Empty backpack has no load"), Inventory->GetLoadFraction(), 0.f);
    Inventory->SetSeaweed(30);
    TestTrue(TEXT("Store tourist"), Inventory->TryAddCatch(TEXT("Tourist"), FIntPoint(1, 2)));
    TestTrue(TEXT("Store photographer"), Inventory->TryAddCatch(TEXT("Photographer"), FIntPoint(3, 2)));
    TestTrue(TEXT("Store farmer"), Inventory->TryAddCatch(TEXT("Farmer"), FIntPoint(2, 2)));
    TestEqual(TEXT("Load counts occupied cells"), Inventory->GetLoadFraction(), 12.f / 16.f);

    TestTrue(TEXT("First bite steals"), Inventory->StealMostValuableCatch(Catalog, Stolen));
    TestEqual(TEXT("First bite takes the most valuable catch"), Stolen.Type, FName(TEXT("Photographer")));
    TestEqual(TEXT("Other catches kept"), Inventory->GetCatches().Num(), 2);
    TestEqual(TEXT("Load drops after a bite"), Inventory->GetLoadFraction(), 6.f / 16.f);
    TestTrue(TEXT("Second bite steals"), Inventory->StealMostValuableCatch(Catalog, Stolen));
    TestEqual(TEXT("Second bite takes the next most valuable"), Stolen.Type, FName(TEXT("Farmer")));
    TestTrue(TEXT("Third bite steals"), Inventory->StealMostValuableCatch(Catalog, Stolen));
    TestEqual(TEXT("Third bite takes the last catch"), Stolen.Type, FName(TEXT("Tourist")));
    TestFalse(TEXT("Nothing left to steal"), Inventory->StealMostValuableCatch(Catalog, Stolen));

    TestEqual(TEXT("Seaweed untouched"), Inventory->GetSeaweed(), 30);
    TestEqual(TEXT("Lifetime captures preserved"), Inventory->GetLifetimeCatchCount(), 3);
    TestTrue(TEXT("Field Guide preserved"), Inventory->HasCaughtType(TEXT("Photographer")));

    TestTrue(TEXT("Store small catch"), Inventory->TryAddCatch(TEXT("Tourist"), FIntPoint(1, 2)));
    TestTrue(TEXT("Store large catch"), Inventory->TryAddCatch(TEXT("Farmer"), FIntPoint(2, 2)));
    TestTrue(TEXT("Steal without a catalog"), Inventory->StealMostValuableCatch(nullptr, Stolen));
    TestEqual(TEXT("Without a catalog the largest catch goes"), Stolen.Type, FName(TEXT("Farmer")));
    TestTrue(TEXT("Progress remains valid"), Inventory->GetProgress().IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanFishingSharkLoad,
    "HumanFishing.Shark.Load", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHumanFishingSharkLoad::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Empty backpack keeps the base value"), AHumanFishingShark::ScaleForLoad(1000.f, 0.f, .5f), 1000.f);
    TestEqual(TEXT("Half load gives half the bonus"), AHumanFishingShark::ScaleForLoad(1000.f, .5f, .5f), 1250.f);
    TestEqual(TEXT("Full load gives the full bonus"), AHumanFishingShark::ScaleForLoad(1000.f, 1.f, .5f), 1500.f);
    TestEqual(TEXT("Load above full is clamped"), AHumanFishingShark::ScaleForLoad(1000.f, 3.f, .5f), 1500.f);
    TestEqual(TEXT("Negative bonus is ignored"), AHumanFishingShark::ScaleForLoad(1000.f, 1.f, -1.f), 1000.f);
    return true;
}

#endif
