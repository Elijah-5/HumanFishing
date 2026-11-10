#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HumanFishingInventory.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FHumanFishingCatchType
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FIntPoint Size = FIntPoint(1, 2);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UTexture2D> Portrait;
};

UCLASS(BlueprintType)
class HUMANFISHINGGAMEPLAY_API UHumanFishingCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TMap<FName, FHumanFishingCatchType> Types;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UTexture2D> BackpackFrame;
};

USTRUCT(BlueprintType)
struct FHumanFishingCatch
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 Id = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly)
    FName Type;

    UPROPERTY(BlueprintReadOnly)
    FIntPoint Cell = FIntPoint::ZeroValue;

    UPROPERTY(BlueprintReadOnly)
    FIntPoint Size = FIntPoint::ZeroValue;
};

/** Session data owned by the GameInstance and shared across shore and ocean. */
UCLASS()
class HUMANFISHINGGAMEPLAY_API UHumanFishingInventory : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static constexpr int32 Columns = 4;
    static constexpr int32 Rows = 4;
    static constexpr float MaxHealth = 100.f;

    const TArray<FHumanFishingCatch>& GetCatches() const { return Catches; }
    const FHumanFishingCatch* FindCatch(int32 Id) const;
    bool CanPlace(FIntPoint Cell, FIntPoint Size, int32 IgnoreId = INDEX_NONE) const;

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    bool TryAddCatch(FName Type, FIntPoint Size);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    bool TryMoveCatch(int32 Id, FIntPoint Cell);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    TArray<FHumanFishingCatch> GetBackpack() const { return Catches; }

    UFUNCTION(BlueprintPure, Category="Human Fishing|Health")
    float GetHealth() const { return Health; }

    void ChangeHealth(float Amount);

private:
    UPROPERTY()
    TArray<FHumanFishingCatch> Catches;

    int32 NextId = 0;
    float Health = MaxHealth;
};
