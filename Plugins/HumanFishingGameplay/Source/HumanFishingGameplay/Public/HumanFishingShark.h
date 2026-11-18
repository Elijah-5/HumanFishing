#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanFishingShark.generated.h"

class APawn;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UHumanFishingPlayerUI;

UENUM(BlueprintType)
enum class EHumanFishingSharkState : uint8
{
    // Swims between random points around its home.
    Patrol,
    // Swims straight at the fish.
    Chase,
    // Just stole from the fish; patrols and ignores the fish until the cooldown ends.
    Retreat
};

/** What the shark knows about the fish this frame. */
struct FHumanFishingSharkSenses
{
    bool bTargetAvailable = false;
    float TargetDistance = TNumericLimits<float>::Max();
    bool bCoolingDown = false;
};

/** Detection ranges, in Unreal units. LoseRadius should be larger than DetectionRadius. */
struct FHumanFishingSharkRanges
{
    float DetectionRadius = 1200.f;
    float LoseRadius = 1800.f;
    float ContactRadius = 160.f;
};

/**
 * A shark that patrols the deep ocean, chases the fish when it swims within DetectionRadius,
 * steals everything in the backpack on contact, and gives up once the fish is beyond LoseRadius.
 *
 * Place it (or a Blueprint subclass with the final mesh) in L_DeepOcean. Its placed location is
 * the center of its patrol area.
 */
UCLASS(Blueprintable)
class HUMANFISHINGGAMEPLAY_API AHumanFishingShark : public AActor
{
    GENERATED_BODY()

public:
    AHumanFishingShark();

    /** Pure state rule, kept separate from movement so it can be tested. Sets bOutSteal on contact. */
    static EHumanFishingSharkState ChooseState(EHumanFishingSharkState Current, const FHumanFishingSharkSenses& Senses,
        const FHumanFishingSharkRanges& Ranges, bool& bOutSteal);

    /** Base * (1 + Load * FullLoadBonus), with Load clamped to 0..1. */
    static float ScaleForLoad(float Base, float Load, float FullLoadBonus);

    UFUNCTION(BlueprintPure, Category="Shark")
    EHumanFishingSharkState GetState() const { return State; }

    /** Radius around the placed location that the shark patrols. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Patrol", meta=(ClampMin="100"))
    float PatrolRadius = 2500.f;

    /** How far above or below its placed height the shark may swim, while patrolling or chasing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Patrol", meta=(ClampMin="0"))
    float VerticalRange = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Patrol", meta=(ClampMin="1"))
    float PatrolSpeed = 157.5f;

    /** Degrees per second. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Patrol", meta=(ClampMin="1"))
    float PatrolTurnRate = 70.f;

    /** The fish must come this close for the chase to start. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Chase", meta=(ClampMin="0"))
    float DetectionRadius = 1200.f;

    /** The chase ends once the fish gets this far away. Kept at least DetectionRadius. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Chase", meta=(ClampMin="0"))
    float LoseRadius = 1800.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Chase", meta=(ClampMin="1"))
    float ChaseSpeed = 315.f;

    /** Degrees per second. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Chase", meta=(ClampMin="1"))
    float ChaseTurnRate = 160.f;

    /** Distance between the shark and fish centers that counts as contact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Chase", meta=(ClampMin="0"))
    float ContactRadius = 160.f;

    /** If true, the shark ignores a fish whose backpack is empty. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Chase")
    bool bOnlyChaseWhenCarrying = false;

    /**
     * Extra detection and give-up range with a full backpack, as a fraction of the base ranges.
     * The bonus grows with the share of backpack cells in use. 0.5 means 50% farther when full.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Carried Catch", meta=(ClampMin="0"))
    float FullLoadDetectionBonus = .5f;

    /** Extra chase speed with a full backpack, as a fraction of ChaseSpeed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Carried Catch", meta=(ClampMin="0"))
    float FullLoadChaseSpeedBonus = .25f;

    /** Seconds the shark ignores the fish after a bite. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Theft", meta=(ClampMin="0"))
    float StealCooldown = 6.f;

    /** Health the fish loses per bite. The fish has 100 HP. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Theft", meta=(ClampMin="0"))
    float BiteDamage = 25.f;

    /** Seconds after a bite before the deep ocean heals the fish again. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shark|Theft", meta=(ClampMin="0"))
    float HealingDelayAfterBite = 4.f;

    /** Called when the state changes, for sounds, animation, or effects in a Blueprint subclass. */
    UFUNCTION(BlueprintImplementableEvent, Category="Shark")
    void OnSharkStateChanged(EHumanFishingSharkState NewState);

    /** Called after each bite. Count is the number of catches taken: 1, or 0 for an empty backpack. */
    UFUNCTION(BlueprintImplementableEvent, Category="Shark")
    void OnCatchesStolen(int32 Count);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shark")
    TObjectPtr<USphereComponent> Collision;

    /** Placeholder body. Replace the meshes in a Blueprint subclass, or hide them and add a skeletal mesh. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shark")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shark")
    TObjectPtr<UStaticMeshComponent> Fin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shark")
    TObjectPtr<UStaticMeshComponent> Tail;

private:
    FHumanFishingSharkRanges GetRanges(float Load) const;
    APawn* FindTarget(UHumanFishingPlayerUI*& OutUI) const;
    void SetState(EHumanFishingSharkState NewState, UHumanFishingPlayerUI* UI);
    void StealFrom(UHumanFishingPlayerUI* UI);
    void PickPatrolPoint(const FVector& AvoidLocation);
    FVector ClampToSwimArea(FVector Location) const;
    void SwimToward(const FVector& Goal, float Speed, float TurnRate, float DeltaSeconds);

    EHumanFishingSharkState State = EHumanFishingSharkState::Patrol;
    FVector Home = FVector::ZeroVector;
    FVector PatrolPoint = FVector::ZeroVector;
    float CurrentSpeed = 0.f;
    float PatrolPointTimeLeft = 0.f;
    float CooldownLeft = 0.f;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<USphereComponent> DetectionPreview;

    UPROPERTY()
    TObjectPtr<USphereComponent> LosePreview;

    UPROPERTY()
    TObjectPtr<USphereComponent> PatrolPreview;
#endif
};
