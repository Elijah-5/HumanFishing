#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "HumanFishingDisplay.generated.h"

UCLASS()
class HUMANFISHINGGAMEPLAY_API UHumanFishingDisplay : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
private:
    bool Tick(float DeltaSeconds);
    void ApplyMacPreset();
    FTSTicker::FDelegateHandle TickHandle;
    void* ObservedWindow=nullptr;
    FVector2D LastSize=FVector2D::ZeroVector,LastOrigin=FVector2D::ZeroVector;
    int32 StableSamples=0;
    bool bPending=true;
};
