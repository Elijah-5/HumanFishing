#pragma once

#include "Containers/Ticker.h"
#include "GameFramework/SaveGame.h"
#include "HumanFishingInventory.h"
#include "HumanFishingSave.generated.h"

UCLASS()
class HUMANFISHINGGAMEPLAY_API UHumanFishingSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 Version = 0;

    UPROPERTY()
    int64 Revision = 0;

    UPROPERTY()
    FHumanFishingProgress Progress;

    UPROPERTY()
    FName Level;

    UPROPERTY()
    FDateTime SavedAt;
};

struct FHumanFishingSlotSummary
{
    bool bExists = false;
    bool bReadable = false;
    int32 Captures = 0;
    int32 Discovered = 0;
    FName Level;
    FDateTime SavedAt;
};

UCLASS()
class HUMANFISHINGGAMEPLAY_API UHumanFishingSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static constexpr int32 SlotCount = 3;
    static const FName ShoreLevel;
    static const FName CoveLevel;
    static const FName OceanLevel;
    static const FName MenuLevel;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    FHumanFishingSlotSummary GetSlotSummary(int32 Slot, const UHumanFishingCatalog* Catalog) const;
    bool StartSlot(UWorld* World, int32 Slot, bool bNewGame);
    bool ReturnToMenu(UWorld* World);
    void LevelReady(FName Level);
    void ProgressChanged(bool bImmediate);
    bool Flush();
    bool HasSaveError() const { return bSaveError; }

    bool EnterDeveloperMode(UWorld* World);
    bool LeaveDeveloperMode(UWorld* World);
    bool IsDeveloperMode() const { return DeveloperSnapshot.IsSet(); }

private:
    struct FDeveloperSnapshot
    {
        FHumanFishingProgress Progress;
        FName Level;
    };

    UHumanFishingSaveGame* ReadLatest(int32 Slot, int32& Generation, bool& bExists) const;
    UHumanFishingSaveGame* WriteSlot(int32 Slot, int32 Generation, int64 Revision,
        const FHumanFishingProgress& Progress, FName Level) const;
    bool Tick(float DeltaTime);
    void HandleExit();

    UPROPERTY()
    TObjectPtr<UHumanFishingInventory> Inventory;

    TOptional<FDeveloperSnapshot> DeveloperSnapshot;
    FTSTicker::FDelegateHandle TickHandle;
    FDelegateHandle ExitHandle;
    FName ResumeLevel;
    int32 ActiveSlot = INDEX_NONE;
    int32 ActiveGeneration = INDEX_NONE;
    int64 ActiveRevision = 0;
    double LastSaveAttempt = 0.;
    bool bDirty = false;
    bool bSaveError = false;
};
