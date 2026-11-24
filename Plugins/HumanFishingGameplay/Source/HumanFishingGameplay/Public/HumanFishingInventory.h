#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HumanFishingInventory.generated.h"

class UTexture2D;
class USkeletalMesh;
class UStaticMesh;
class UAnimSequence;
class UAnimMontage;

UENUM(BlueprintType)
enum class EHumanFishingBait : uint8
{
    Dollar,
    Flash,
    HundredDollarBill,
    Count UMETA(Hidden)
};

struct FHumanFishingBaitDefinition
{
    FText Name;
    UStaticMesh* Mesh;
    UTexture2D* Portrait;
    double InterestMultiplier = 1.;
    double InspectBonusSeconds = 0.;
    bool bPrioritizeRarity = false;
};

UENUM(BlueprintType)
enum class EHumanFishingStruggle : uint8
{
    Steady,
    Pulse,
    CapFeint
};

UENUM(BlueprintType)
enum class EHumanFishingCatchFinish : uint8
{
    Shore,
    WaterContact
};

UENUM(BlueprintType)
enum class EHumanFishingRarity : uint8
{
    White,
    Green,
    Blue,
    Purple,
    Gold,
    Red,
    Count UMETA(Hidden)
};

struct FHumanFishingRarityRules
{
    int32 SpawnWeight;
    FVector2D StaySeconds;
    int32 SeaweedPerCell;
    FLinearColor BackpackColor;
};

USTRUCT(BlueprintType)
struct FHumanFishingAccessory
{
    GENERATED_BODY()

    // The prop is authored in the visitor's skeletal mesh reference space.
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName Bone;
};

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    FText FieldNotes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    FText CaptureTips;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach")
    TObjectPtr<USkeletalMesh> BeachMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="0"))
    float MoneyPreference = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="0"))
    float InterestRadius = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="0"))
    float InterestGainMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach")
    EHumanFishingStruggle Struggle = EHumanFishingStruggle::Steady;

    UPROPERTY(EditAnywhere, Category="Beach")
    bool bRequiresHundredDollarBill = false;

    bool AcceptsBait(EHumanFishingBait Bait) const
    {
        return !bRequiresHundredDollarBill || Bait == EHumanFishingBait::HundredDollarBill || Bait == EHumanFishingBait::Flash;
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach")
    EHumanFishingCatchFinish CatchFinish = EHumanFishingCatchFinish::Shore;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="0"))
    float InterestDecayPerSecond = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="0"))
    float LineWearPerSecond = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="1"))
    float WalkSpeed = 160.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EHumanFishingRarity Rarity = EHumanFishingRarity::White;

    // A zero range uses the rarity's default duration.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="0"))
    FVector2D StaySecondsOverride = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach", meta=(ClampMin="0"))
    int32 RequiredCatches = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach")
    bool bUniquePerSession = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beach")
    TArray<FHumanFishingAccessory> Accessories;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimSequence> IdleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> InspectAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimSequence> StruggleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimSequence> WaterEntryAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimSequence> WaterReelAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimSequence> WaterReturnAnimation;
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

USTRUCT()
struct FHumanFishingProgress
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FHumanFishingCatch> Catches;

    UPROPERTY()
    TSet<FName> CaughtTypes;

    // IDs are never reused, so this is also the total number of successful captures.
    UPROPERTY()
    int32 NextId = 0;

    UPROPERTY()
    int32 Seaweed = 0;

    UPROPERTY()
    float Health = 100.f;

    UPROPERTY()
    bool bHasMudskipper = false;

    UPROPERTY()
    bool bHasBraidedLine = false;

    UPROPERTY()
    bool bHasHundredDollarBill = false;

    UPROPERTY()
    int32 FlashBaitCount = 0;

    UPROPERTY()
    int32 HundredDollarBillCount = 0;

    UPROPERTY()
    bool bHasCoveKey = false;

    UPROPERTY()
    bool bHasBackpackUpgrade = false;

    UPROPERTY()
    int32 JellyshockCount = 0;
    UPROPERTY()
    int32 VioletJellyshockCount = 0;

    UPROPERTY()
    bool bHasTidalFins = false;

    bool IsValid() const;
    int32 GetColumns() const;
};

UCLASS(BlueprintType)
class HUMANFISHINGGAMEPLAY_API UHumanFishingCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TMap<FName, FHumanFishingCatchType> Types;

    UPROPERTY(EditAnywhere, Category="Bait")
    TObjectPtr<UStaticMesh> DollarBaitMesh;

    UPROPERTY(EditAnywhere, Category="Bait")
    TObjectPtr<UStaticMesh> FlashBaitMesh;

    UPROPERTY(EditAnywhere, Category="Bait")
    TObjectPtr<UStaticMesh> HundredDollarBillMesh;

    UPROPERTY(EditAnywhere, Category="Bait")
    TObjectPtr<UTexture2D> DollarBaitPortrait;

    UPROPERTY(EditAnywhere, Category="Bait")
    TObjectPtr<UTexture2D> FlashBaitPortrait;

    UPROPERTY(EditAnywhere, Category="Bait")
    TObjectPtr<UTexture2D> HundredDollarBillPortrait;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UTexture2D> BackpackFrame;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    TObjectPtr<UTexture2D> GuideCover;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    TObjectPtr<UTexture2D> GuidePaper;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    TObjectPtr<UTexture2D> GuideUnknown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    TObjectPtr<UTexture2D> GuideBookIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    TObjectPtr<UTexture2D> GuideNotesIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    TObjectPtr<UTexture2D> GuideTipsIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field Guide")
    TObjectPtr<UTexture2D> GuideLockIcon;

    FHumanFishingBaitDefinition GetBaitDefinition(EHumanFishingBait Bait) const;
    static const FHumanFishingRarityRules& GetRarityRules(EHumanFishingRarity Rarity);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    int32 GetCatchSeaweedValue(const FHumanFishingCatch& Item) const;
};

/** Session data owned by the GameInstance and shared across shore and ocean. */
UCLASS()
class HUMANFISHINGGAMEPLAY_API UHumanFishingInventory : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static constexpr int32 Columns = 4;
    static constexpr int32 UpgradedColumns = 5;
    static constexpr int32 Rows = 4;
    static constexpr float MaxHealth = 100.f;
    static constexpr int32 MudskipperCost = 12;
    static constexpr int32 BraidedLineCost = 12;
    static constexpr int32 HundredDollarBillCost = 5;
    static constexpr int32 FlashBaitCost = 10;
    static constexpr int32 CoveKeyCost = 60;
    static constexpr int32 BackpackUpgradeCost = 50;
    static constexpr int32 JellyshockCost = 12;
    static constexpr int32 VioletJellyshockCost = 35;
    static constexpr int32 TidalFinsCost = 50;

    const TArray<FHumanFishingCatch>& GetCatches() const { return Progress.Catches; }
    int32 GetLifetimeCatchCount() const { return Progress.NextId; }
    bool HasCaughtType(FName Type) const { return Progress.CaughtTypes.Contains(Type); }
    const FHumanFishingProgress& GetProgress() const { return Progress; }
    EHumanFishingBait GetSelectedBait() const { return SelectedBait; }
    void ToggleBait();
    const FHumanFishingCatch* FindCatch(int32 Id) const;
    bool CanPlace(FIntPoint Cell, FIntPoint Size, int32 IgnoreId = INDEX_NONE) const;

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    bool TryAddCatch(FName Type, FIntPoint Size);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    bool TryMoveCatch(int32 Id, FIntPoint Cell);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    bool TryDiscardCatch(int32 Id);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    TArray<FHumanFishingCatch> GetBackpack() const { return Progress.Catches; }

    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    int32 GetColumns() const { return Progress.GetColumns(); }

    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    bool HasBackpackUpgrade() const { return Progress.bHasBackpackUpgrade; }

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    bool BuyBackpackUpgrade();

    UFUNCTION(BlueprintPure, Category="Human Fishing|Equipment")
    int32 GetJellyshockCount() const { return Progress.JellyshockCount; }

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Equipment")
    bool BuyJellyshock(int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Equipment")
    bool ConsumeJellyshock();
    UFUNCTION(BlueprintPure, Category="Human Fishing|Equipment")
    int32 GetVioletJellyshockCount() const { return Progress.VioletJellyshockCount; }
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Equipment")
    bool BuyVioletJellyshock(int32 Quantity=1);
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Equipment")
    bool ConsumeVioletJellyshock();

    UFUNCTION(BlueprintPure, Category="Human Fishing|Equipment")
    bool HasTidalFins() const { return Progress.bHasTidalFins; }

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Equipment")
    bool BuyTidalFins();

    // Session energy survives travel but is not serialized into a save.
    float SprintEnergy = 5.f;
    float SprintRecoveryDelay = 0.f;

    UFUNCTION(BlueprintPure, Category="Human Fishing|Health")
    float GetHealth() const { return Progress.Health; }

    UFUNCTION(BlueprintPure, Category="Human Fishing|Progress")
    int32 GetSeaweed() const { return Progress.Seaweed; }

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Progress")
    void SetSeaweed(int32 Amount);

    /** Share of backpack cells in use, from 0 (empty) to 1 (full). */
    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    float GetLoadFraction() const;

    /**
     * Removes the catch worth the most seaweed (the largest one without a catalog) without paying anything.
     * Lifetime captures and the Field Guide are kept. Returns false for an empty backpack.
     */
    bool StealMostValuableCatch(const UHumanFishingCatalog* Catalog, FHumanFishingCatch& OutStolen);

    int32 GetRedemptionValue(const UHumanFishingCatalog* Catalog) const;
    int32 RedeemCatches(const UHumanFishingCatalog* Catalog);
    /** Redeem exactly one catch; preserves lifetime captures and discoveries. */
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Shop")
    int32 RedeemCatch(const UHumanFishingCatalog* Catalog, int32 CatchId);

    bool HasMudskipper() const { return Progress.bHasMudskipper; }
    bool BuyMudskipper();

    bool HasBraidedLine() const { return Progress.bHasBraidedLine; }
    bool BuyBraidedLine();
    bool HasHundredDollarBill() const { return GetBaitCount(EHumanFishingBait::HundredDollarBill) > 0; }
    bool BuyHundredDollarBill();

    UFUNCTION(BlueprintPure, Category="Human Fishing|Bait")
    int32 GetBaitCount(EHumanFishingBait Bait) const;

    UFUNCTION(BlueprintPure, Category="Human Fishing|Bait")
    bool CanCastSelectedBait() const;

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait")
    bool BuyBait(EHumanFishingBait Bait, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait")
    bool ReserveSelectedBait();

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait")
    void CommitHookedBait();

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait")
    void FinishBaitUse(bool bRetrieveBeforeHook);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Travel")
    bool HasCoveKey() const { return Progress.bHasCoveKey; }

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Travel")
    bool BuyCoveKey();

    bool HasActiveBaitUse() const { return bBaitReserved; }
    static int32 GetBaitPrice(EHumanFishingBait Bait);

    void ChangeHealth(float Amount);

private:
    friend class UHumanFishingSaveSubsystem;

    bool BuyUpgrade(int32 Cost, bool& bOwned);
    void NotifyChange(bool bImmediate = true);

    UPROPERTY()
    FHumanFishingProgress Progress;

    EHumanFishingBait SelectedBait = EHumanFishingBait::Dollar;
    EHumanFishingBait ReservedBait = EHumanFishingBait::Dollar;
    bool bBaitReserved = false;
    bool bBaitCommitted = false;
};
