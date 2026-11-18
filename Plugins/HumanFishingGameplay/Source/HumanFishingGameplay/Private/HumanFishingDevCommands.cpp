// Console commands for testing the backpack, shop, and sharks without fishing first.
// Changes save to the current slot unless developer mode (hf.dev on) is active.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "HumanFishingSave.h"
#include "Kismet/GameplayStatics.h"

namespace
{
UHumanFishingPlayerUI* DevFindPlayerUI(UWorld* World)
{
    APawn* Fish = World && World->IsGameWorld() ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
    return Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
}

void DevNoteSaving(UWorld* World, FOutputDevice& Output)
{
    const UGameInstance* Game = World->GetGameInstance();
    const auto* Save = Game ? Game->GetSubsystem<UHumanFishingSaveSubsystem>() : nullptr;
    if (!Save || !Save->IsDeveloperMode())
    {
        Output.Log(TEXT("Saved to the current slot. Use hf.dev on first to test without keeping changes."));
    }
}

FAutoConsoleCommandWithWorldArgsAndOutputDevice GiveCommand(
    TEXT("hf.give"), TEXT("Add catches to the backpack: hf.give <visitor ID> [count]. Run without arguments to list IDs."),
    FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda(
        [](const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)
        {
            UHumanFishingPlayerUI* UI = DevFindPlayerUI(World);
            const UHumanFishingCatalog* Catalog = UI ? UI->GetCatalog() : nullptr;
            if (!Catalog)
            {
                Output.Log(TEXT("Start a game with the fish first."));
                return;
            }

            TArray<FString> Ids;
            for (const auto& Pair : Catalog->Types)
            {
                Ids.Add(Pair.Key.ToString());
            }
            Ids.Sort();
            const FName Type = Args.Num() > 0 ? FName(*Args[0]) : NAME_None;
            if (Type.IsNone() || !Catalog->Types.Contains(Type))
            {
                Output.Logf(TEXT("Usage: hf.give <visitor ID> [count]. Visitor IDs: %s"), *FString::Join(Ids, TEXT(", ")));
                return;
            }

            const int32 Requested = Args.Num() > 1 ? FMath::Max(FCString::Atoi(*Args[1]), 1) : 1;
            int32 Added = 0;
            while (Added < Requested && UI->TryStoreCatch(Type))
            {
                ++Added;
            }
            Output.Logf(TEXT("Added %d %s to the backpack.%s"), Added, *Type.ToString(),
                Added < Requested ? TEXT(" The backpack is full.") : TEXT(""));
            if (Added > 0)
            {
                DevNoteSaving(World, Output);
            }
        }), ECVF_Cheat);

FAutoConsoleCommandWithWorldArgsAndOutputDevice SeaweedCommand(
    TEXT("hf.seaweed"), TEXT("Set the seaweed balance: hf.seaweed <amount>"),
    FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda(
        [](const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)
        {
            UHumanFishingPlayerUI* UI = DevFindPlayerUI(World);
            UHumanFishingInventory* Inventory = UI ? UI->GetInventory() : nullptr;
            if (!Inventory)
            {
                Output.Log(TEXT("Start a game with the fish first."));
                return;
            }
            if (Args.Num() != 1 || !Args[0].IsNumeric())
            {
                Output.Logf(TEXT("Usage: hf.seaweed <amount>. Current balance: %d"), Inventory->GetSeaweed());
                return;
            }
            Inventory->SetSeaweed(FMath::Max(FCString::Atoi(*Args[0]), 0));
            Output.Logf(TEXT("Seaweed balance set to %d."), Inventory->GetSeaweed());
            DevNoteSaving(World, Output);
        }), ECVF_Cheat);
}

#endif
