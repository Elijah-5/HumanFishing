#pragma once

#include "GameFramework/GameModeBase.h"
#include "HumanFishingSaveMenu.generated.h"

class SWidget;
class UHumanFishingCatalog;

UCLASS()
class HUMANFISHINGGAMEPLAY_API AHumanFishingSaveMenu : public AGameModeBase
{
    GENERATED_BODY()

public:
    AHumanFishingSaveMenu();

protected:
    virtual void StartPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY()
    TObjectPtr<UHumanFishingCatalog> Catalog;

    TSharedPtr<SWidget> Menu;
};
