#include "HumanFishingGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "HumanFishingSave.h"
#include "Kismet/GameplayStatics.h"

AActor* AHumanFishingGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (FName(*UGameplayStatics::GetCurrentLevelName(this)) == UHumanFishingSaveSubsystem::OceanLevel)
    {
        const FName ShoreSide(TEXT("ShoreSide"));
        const FString Arrival = UGameplayStatics::ParseOption(OptionsString, TEXT("Arrival"));
        const FName Requested = Arrival.IsEmpty() ? ShoreSide : FName(*Arrival);
        APlayerStart* DefaultStart = nullptr;
        for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        {
            if (It->PlayerStartTag == Requested)
            {
                return *It;
            }
            if (It->PlayerStartTag == ShoreSide)
            {
                DefaultStart = *It;
            }
        }
        if (DefaultStart)
        {
            return DefaultStart;
        }
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}
