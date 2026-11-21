#if WITH_DEV_AUTOMATION_TESTS
#include "HumanFishingAbilities.h"
#include "HumanFishingInventory.h"
#include "HumanFishingSave.h"
#include "HumanFishingShark.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHFEquipmentPurchase,"HumanFishing.Equipment.Purchases",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHFEquipmentPurchase::RunTest(const FString&)
{
    auto* Game=NewObject<UGameInstance>();auto* I=NewObject<UHumanFishingInventory>(Game);
    TestFalse(TEXT("Empty capsule stock cannot be consumed"),I->ConsumeJellyshock());
    I->SetSeaweed(100);
    TestFalse(TEXT("Negative quantity rejected"),I->BuyJellyshock(-1));
    TestFalse(TEXT("Overflow quantity rejected"),I->BuyJellyshock(MAX_int32));
    TestTrue(TEXT("Buy two capsules"),I->BuyJellyshock(2));TestEqual(TEXT("Exact cost"),I->GetSeaweed(),76);
    TestTrue(TEXT("Use first capsule"),I->ConsumeJellyshock());TestEqual(TEXT("One charge consumed"),I->GetJellyshockCount(),1);
    TestTrue(TEXT("Unlock fins"),I->BuyTidalFins());TestEqual(TEXT("Fins cost"),I->GetSeaweed(),26);
    TestFalse(TEXT("Cannot buy fins twice"),I->BuyTidalFins());TestEqual(TEXT("Repeat purchase does not charge"),I->GetSeaweed(),26);
    TestTrue(TEXT("Progress valid"),I->GetProgress().IsValid());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHFSprint,"HumanFishing.Equipment.Sprint",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHFSprint::RunTest(const FString&)
{
    float Energy=5,Delay=0;bool MustRelease=false;
    TestTrue(TEXT("Unlocked water sprint starts"),FHumanFishingSprintState::Advance(1,true,true,Energy,Delay,MustRelease));
    TestEqual(TEXT("Energy drains"),Energy,4.f);
    TestFalse(TEXT("Land/menu cannot sprint"),FHumanFishingSprintState::Advance(.5,true,false,Energy,Delay,MustRelease));
    TestFalse(TEXT("Energy runs out"),FHumanFishingSprintState::Advance(4,true,true,Energy,Delay,MustRelease));
    TestTrue(TEXT("Empty requires key release"),MustRelease);
    FHumanFishingSprintState::Advance(9,true,true,Energy,Delay,MustRelease);
    TestEqual(TEXT("Eight seconds restores full after delay"),Energy,5.f);
    TestFalse(TEXT("Holding the exhausted key does not restart"),FHumanFishingSprintState::Advance(0,true,true,Energy,Delay,MustRelease));
    FHumanFishingSprintState::Advance(0,false,true,Energy,Delay,MustRelease);
    TestTrue(TEXT("Release and press restarts"),FHumanFishingSprintState::Advance(.1,true,true,Energy,Delay,MustRelease));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHFV2EquipmentSave,"HumanFishing.Equipment.SaveMigration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHFV2EquipmentSave::RunTest(const FString&)
{
    auto* Save=NewObject<UHumanFishingSaveGame>();Save->Version=2;Save->Progress.Seaweed=1000;
    Save->Progress.HundredDollarBillCount=7;Save->Progress.bHasCoveKey=true;Save->Progress.bHasBackpackUpgrade=true;
    TestTrue(TEXT("Upgrade v2"),UHumanFishingSaveSubsystem::MigrateSave(Save));
    TestEqual(TEXT("Save version"),Save->Version,UHumanFishingSaveSubsystem::CurrentSaveVersion);TestEqual(TEXT("Money preserved"),Save->Progress.Seaweed,1000);
    TestEqual(TEXT("Bait preserved"),Save->Progress.HundredDollarBillCount,7);
    TestTrue(TEXT("Key and expansion kept"),Save->Progress.bHasCoveKey && Save->Progress.bHasBackpackUpgrade);
    TestEqual(TEXT("New stock defaults empty"),Save->Progress.JellyshockCount,0);
    TestFalse(TEXT("Fins start locked"),Save->Progress.bHasTidalFins);
    TestFalse(TEXT("Migration only once"),UHumanFishingSaveSubsystem::MigrateSave(Save));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHFSharkStun,"HumanFishing.Equipment.SharkStun",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHFSharkStun::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Shark=World->SpawnActor<AHumanFishingShark>();
    if(!TestNotNull(TEXT("Create shark"),Shark)){World->DestroyWorld(false);return false;}
    const FVector Start=Shark->GetActorLocation();
    Shark->ApplyStun(30);static_cast<AActor*>(Shark)->Tick(1);
    TestEqual(TEXT("Stunned state"),Shark->GetState(),EHumanFishingSharkState::Stunned);
    TestEqual(TEXT("Timer advances"),Shark->GetStunTimeRemaining(),29.f);
    TestEqual(TEXT("No swimming while stunned"),Shark->GetActorLocation(),Start);
    Shark->ApplyStun(30);TestEqual(TEXT("Hits refresh rather than stack"),Shark->GetStunTimeRemaining(),30.f);
    static_cast<AActor*>(Shark)->Tick(30);
    TestTrue(TEXT("Stun is temporary"),Shark->GetState()!=EHumanFishingSharkState::Stunned);
    TestEqual(TEXT("Timer expired"),Shark->GetStunTimeRemaining(),0.f);
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHFJellyAim,"HumanFishing.Equipment.JellyshockAim",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHFJellyAim::RunTest(const FString&)
{
    const FVector Forward(1,0,0);
    const float Center=UHumanFishingAbilities::JellyshockTargetScore(Forward,FVector(1000,0,0),1000);
    const float Edge=UHumanFishingAbilities::JellyshockTargetScore(Forward,FRotator(0,7.9f,0).Vector()*1000,1000);
    TestTrue(TEXT("Center is preferred to edge of assist cone"),Center<Edge && Edge<MAX_flt);
    TestEqual(TEXT("Outside eight degrees is rejected"),UHumanFishingAbilities::JellyshockTargetScore(Forward,FRotator(0,8.1f,0).Vector()*1000,1000),MAX_flt);
    TestEqual(TEXT("Outside thirty meters is rejected"),UHumanFishingAbilities::JellyshockTargetScore(Forward,Forward*3100,3100),MAX_flt);
    TestEqual(TEXT("Stationary target has no lead"),UHumanFishingAbilities::PredictJellyshockPoint(FVector::ZeroVector,FVector(900,0,0),FVector::ZeroVector),FVector(900,0,0));
    TestEqual(TEXT("Moving target lead is capped at half a second"),UHumanFishingAbilities::PredictJellyshockPoint(FVector::ZeroVector,FVector(3000,0,0),FVector(0,300,0)),FVector(3000,150,0));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHFViolet,"HumanFishing.Equipment.VioletJellyshock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHFViolet::RunTest(const FString&)
{
    auto* Game=NewObject<UGameInstance>();auto* I=NewObject<UHumanFishingInventory>(Game);
    TestFalse(TEXT("No free lethal shots"),I->ConsumeVioletJellyshock());I->SetSeaweed(100);
    TestFalse(TEXT("Invalid quantity rejected"),I->BuyVioletJellyshock(-1));
    TestFalse(TEXT("Overflow quantity rejected"),I->BuyVioletJellyshock(MAX_int32));
    TestTrue(TEXT("Buy two violet capsules"),I->BuyVioletJellyshock(2));
    TestEqual(TEXT("Premium price is 35 each"),I->GetSeaweed(),30);
    TestFalse(TEXT("Insufficient funds rejected"),I->BuyVioletJellyshock());
    TestTrue(TEXT("Consume one violet capsule"),I->ConsumeVioletJellyshock());
    TestEqual(TEXT("Separate violet inventory"),I->GetVioletJellyshockCount(),1);
    TestEqual(TEXT("Normal inventory unaffected"),I->GetJellyshockCount(),0);
    auto* Save=NewObject<UHumanFishingSaveGame>();Save->Version=3;Save->Progress.Seaweed=123;
    Save->Progress.JellyshockCount=7;Save->Progress.bHasTidalFins=true;
    TestTrue(TEXT("Migrate v3"),UHumanFishingSaveSubsystem::MigrateSave(Save));
    TestEqual(TEXT("New stock defaults zero"),Save->Progress.VioletJellyshockCount,0);
    TestEqual(TEXT("Existing stock preserved"),Save->Progress.JellyshockCount,7);
    TestEqual(TEXT("Balance preserved"),Save->Progress.Seaweed,123);
    TestTrue(TEXT("Fins preserved"),Save->Progress.bHasTidalFins);
    TestFalse(TEXT("Migration is idempotent"),UHumanFishingSaveSubsystem::MigrateSave(Save));return true;
}
#endif
