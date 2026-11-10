#pragma once

#include "GameFramework/Actor.h"
#include "HumanFishingBeachCrowd.generated.h"

class ACharacter;
class AAIController;
class FAutoConsoleCommandWithWorldArgsAndOutputDevice;
class UAnimMontage;
class UHumanFishingCatalog;
struct FHumanFishingCatchType;

UCLASS()
class HUMANFISHINGGAMEPLAY_API AHumanFishingBeachCrowd : public AActor
{
    GENERATED_BODY()

public:
    AHumanFishingBeachCrowd();

    UPROPERTY(EditInstanceOnly, Category="Beach")
    TObjectPtr<AActor> Beach;

    UPROPERTY(EditAnywhere, Category="Beach")
    TSubclassOf<ACharacter> PedestrianClass;

    UPROPERTY(EditAnywhere, Category="Beach")
    TObjectPtr<UHumanFishingCatalog> Catalog;

    UPROPERTY(EditAnywhere, Category="Beach", meta=(ClampMin="1"))
    int32 MaxPedestrians = 6;

    UPROPERTY(EditAnywhere, Category="Beach", meta=(ClampMin="0.25"))
    float SpawnIntervalMin = 10.f;

    UPROPERTY(EditAnywhere, Category="Beach", meta=(ClampMin="0.25"))
    float SpawnIntervalMax = 16.f;

    UFUNCTION(BlueprintPure, Category="Beach")
    bool IsOnBeach(FVector Position) const;

    UFUNCTION(BlueprintCallable, Category="Beach")
    bool ObserveBait(int32 CastId, FVector Bait, FVector Goal, double Appeal,
        ACharacter*& Candidate, double& Interest);

    UFUNCTION(BlueprintCallable, Category="Beach")
    bool ReleaseTarget(ACharacter* Pedestrian);

    UFUNCTION(BlueprintCallable, Category="Beach")
    void IgnoreCast(ACharacter* Pedestrian, int32 CastId);

    UFUNCTION(BlueprintPure, Category="Beach")
    FName GetCatchType(const ACharacter* Pedestrian) const;

    UFUNCTION(BlueprintPure, Category="Beach")
    double GetInterestDecay(const ACharacter* Pedestrian) const;

    UFUNCTION(BlueprintPure, Category="Beach")
    double GetLineWear(const ACharacter* Pedestrian) const;

    UFUNCTION(BlueprintPure, Category="Beach")
    bool NeedsWaterContact(const ACharacter* Pedestrian) const;

    UFUNCTION(BlueprintPure, Category="Beach")
    bool IsWaterReel(const ACharacter* Pedestrian) const;

    UFUNCTION(BlueprintPure, Category="Beach")
    FString GetInterestCaption(const ACharacter* Pedestrian) const;

    UFUNCTION(BlueprintPure, Category="Beach")
    FString GetReelPrompt(const ACharacter* Pedestrian, const FString& DefaultText) const;

    UFUNCTION(BlueprintPure, Category="Beach")
    UAnimMontage* GetInspectAnimation(const ACharacter* Pedestrian, UAnimMontage* DefaultAnimation) const;

    UFUNCTION(BlueprintCallable, Category="Beach")
    void BeginReelSession(ACharacter* Pedestrian, const ACharacter* Fish, FVector ShoreGoal);

    UFUNCTION(BlueprintCallable, Category="Beach")
    void SetReelInput(ACharacter* Pedestrian, bool bPulling);

    UFUNCTION(BlueprintCallable, Category="Beach")
    double SampleLineWear(ACharacter* Pedestrian);

    // -1 for a blocked route, 0 while pulling, 1 at contact.
    UFUNCTION(BlueprintCallable, Category="Beach")
    int32 AdvanceContactReel(ACharacter* Pedestrian, const ACharacter* Fish, double& Progress);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    enum class EVisitPhase : uint8
    {
        Entering,
        Staying,
        Leaving
    };

    enum class EWaterReelPhase : uint8
    {
        Dry,
        Crossing,
        InWater,
        Returning
    };

    struct FVisitor
    {
        TWeakObjectPtr<ACharacter> Pawn;
        FName Type;
        EVisitPhase Phase = EVisitPhase::Entering;
        TOptional<FVector> Destination;
        double RouteStartedAt = 0;
        double RouteAfter = 0;
        double StayUntil = 0;
        double CheckPathAfter = 0;
        double Interest = 0;
        double ReelStartedAt = 0;
        double LastWearSample = 0;
        double ExtraWear = 0;
        double TotalReelDistance = 1;
        FVector ShoreGoal = FVector::ZeroVector;
        FVector DryAnchor = FVector::ZeroVector;
        FVector WaterAnchor = FVector::ZeroVector;
        FVector FishAnchor = FVector::ZeroVector;
        EWaterReelPhase WaterPhase = EWaterReelPhase::Dry;
        int32 IgnoredCast = INDEX_NONE;
        bool bReelActive = false;
        bool bPulling = false;
        bool bReachedWaterAnchor = false;
        bool bCanReachBait = false;
        bool bExitNorth = false;
    };

    void QueueSpawn(double Now);
    bool SpawnVisitor(double Now, bool bNorth, FName Type, FVector Point, bool bStartInland = false);
#if !UE_BUILD_SHIPPING
    static FAutoConsoleCommandWithWorldArgsAndOutputDevice DeveloperCommand;
    void RunDeveloperCommand(const TArray<FString>& Args, FOutputDevice& Output);
#endif
    void WalkVisitors(double Now);
    bool AdvanceVisitor(FVisitor& Visitor, ACharacter& Pawn, double Now);
    bool StartRoute(FVisitor& Visitor, AAIController& AI, FVector Goal, double Now);
    void BeginStay(FVisitor& Visitor, double Now);
    void AdvanceReturn(FVisitor& Visitor, ACharacter& Pawn, float DeltaSeconds);
    bool FindInlandPoint(ACharacter& Pawn, bool bRoaming, FVector& Destination) const;
    bool ProjectToBeach(FVector Point, FVector& Projected) const;
    bool CanReach(ACharacter& Pawn, FVector Goal) const;
    FVector SidePoint(bool bNorth) const;
    FName ChooseType() const;
    FVisitor* FindVisitor(const ACharacter* Pedestrian);
    const FVisitor* FindVisitor(const ACharacter* Pedestrian) const;
    const FHumanFishingCatchType* FindProfile(const FVisitor* Visitor) const;

    TArray<FVisitor> Visitors;
    TWeakObjectPtr<ACharacter> HeldVisitor;
    FBox BeachBounds = FBox(ForceInit);
    double SpawnAt = 0;
    double NextCrowdUpdate = 0;
    double HeldSince = 0;
    int32 ObservedCast = INDEX_NONE;
    int32 RepeatedSide = 0;
    bool bLastSpawnNorth = false;
};
