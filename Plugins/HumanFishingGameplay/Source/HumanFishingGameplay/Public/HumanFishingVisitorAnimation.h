#pragma once

#include "Animation/AnimInstance.h"
#include "HumanFishingVisitorAnimation.generated.h"

class UAnimSequence;
struct FHumanFishingCatchType;

UCLASS(Blueprintable)
class HUMANFISHINGGAMEPLAY_API UHumanFishingVisitorAnimation : public UAnimInstance
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Locomotion")
    void InitializeLocomotion(UAnimSequence* Idle);

    void Configure(const FHumanFishingCatchType* Profile);
    void ShowStruggle(double Time);
    void BeginWaterEntry();
    void BeginWaterReturn();
    void ClearFishingAction();
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> StandingIdle;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion")
    TObjectPtr<UAnimSequence> ActiveIdle;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion")
    float WalkRate = 1.f;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion")
    float WalkBlendWeight = 0.f;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<UAnimSequence> FishingAction;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Fishing")
    float FishingActionTime = 0.f;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Fishing")
    bool bShowFishingAction = false;

private:
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> ProfileIdle;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> Struggle;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> WaterEntry;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> WaterReel;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> WaterReturn;

    float WalkBlendProgress = 0.f;
    bool bAnimateWater = false;
};
