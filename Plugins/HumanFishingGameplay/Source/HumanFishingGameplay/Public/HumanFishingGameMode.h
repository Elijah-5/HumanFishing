#pragma once

#include "GameFramework/GameModeBase.h"
#include "HumanFishingGameMode.generated.h"

UCLASS()
class HUMANFISHINGGAMEPLAY_API AHumanFishingGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
