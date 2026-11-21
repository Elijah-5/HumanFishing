#pragma once
#include "Components/ActorComponent.h"
#include "HumanFishingAbilities.generated.h"

class AHumanFishingShark;

struct HUMANFISHINGGAMEPLAY_API FHumanFishingJellyshockAim
{
    FVector Origin=FVector::ZeroVector;
    FVector Direction=FVector::ForwardVector;
    FVector Point=FVector::ZeroVector;
    TWeakObjectPtr<AHumanFishingShark> Target;
    bool bAssisted=false;
    bool bBlocked=false;
};

struct HUMANFISHINGGAMEPLAY_API FHumanFishingSprintState
{
    static constexpr float Capacity = 5.f;
    static constexpr float Multiplier = 1.6f;
    static bool Advance(float DeltaSeconds, bool bHeld, bool bAllowed,
        float& Energy, float& RecoveryDelay, bool& bMustRelease);
};

UCLASS(ClassGroup=(HumanFishing), meta=(BlueprintSpawnableComponent))
class HUMANFISHINGGAMEPLAY_API UHumanFishingAbilities : public UActorComponent
{
    GENERATED_BODY()
public:
    UHumanFishingAbilities();
    UFUNCTION(BlueprintPure, Category="Human Fishing|Equipment")
    bool IsSprinting() const { return bSprinting; }
    UFUNCTION(BlueprintPure, Category="Human Fishing|Equipment")
    bool IsUnderwater() const;
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Equipment")
    bool FireJellyshock();
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Equipment")
    bool FireVioletJellyshock();
    UFUNCTION(BlueprintPure, Category="Human Fishing|Equipment")
    bool GetJellyshockAimPoint(FVector& WorldPoint, bool& bAssisted) const;
    FHumanFishingJellyshockAim QueryJellyshockAim(bool bForceRefresh=false) const;
    bool ShowJellyshockReticle() const;
    static float JellyshockTargetScore(const FVector& Forward,const FVector& CameraDelta,float MuzzleDistance);
    static FVector PredictJellyshockPoint(const FVector& Origin,const FVector& Position,const FVector& Velocity);
    void CancelSprint();
    float GetSprintFraction() const;
    bool ShowSprintMeter() const;
    float GetWaterSpeedMultiplier() const { return bSprinting ? FHumanFishingSprintState::Multiplier : 1.f; }
protected:
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool CanUseAbilities() const;
    bool FireJellyshockVariant(bool bViolet);
    bool bSprinting = false;
    bool bMustRelease = false;
    bool bCapturedSpeed = false;
    float BaseFlySpeed = 300.f;
    float BaseSwimSpeed = 300.f;
    double NextShotTime = 0.;
    mutable FHumanFishingJellyshockAim CachedAim;
    mutable double AimUpdatedAt=-1.;
};
