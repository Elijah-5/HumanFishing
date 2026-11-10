#include "HumanFishingVisitorAnimation.h"

#include "Animation/AnimSequence.h"
#include "GameFramework/Pawn.h"
#include "HumanFishingInventory.h"

void UHumanFishingVisitorAnimation::InitializeLocomotion(UAnimSequence* Idle)
{
    StandingIdle = Idle;
    ActiveIdle = ProfileIdle ? ProfileIdle.Get() : Idle;
}

void UHumanFishingVisitorAnimation::Configure(const FHumanFishingCatchType* Profile)
{
    ProfileIdle = Profile ? Profile->IdleAnimation.Get() : nullptr;
    ActiveIdle = ProfileIdle ? ProfileIdle.Get() : StandingIdle.Get();
    Struggle = Profile ? Profile->StruggleAnimation.Get() : nullptr;
    WaterEntry = Profile ? Profile->WaterEntryAnimation.Get() : nullptr;
    WaterReel = Profile ? Profile->WaterReelAnimation.Get() : nullptr;
    WaterReturn = Profile ? Profile->WaterReturnAnimation.Get() : nullptr;
}

void UHumanFishingVisitorAnimation::ShowStruggle(double Time)
{
    if (Struggle)
    {
        FishingAction = Struggle;
        bShowFishingAction = Time >= 0.;
        FishingActionTime = FMath::Max(0., Time);
    }
}

void UHumanFishingVisitorAnimation::BeginWaterEntry()
{
    FishingAction = WaterEntry ? WaterEntry.Get() : WaterReel.Get();
    FishingActionTime = 0.f;
    bAnimateWater = bShowFishingAction = FishingAction != nullptr;
}

void UHumanFishingVisitorAnimation::BeginWaterReturn()
{
    FishingAction = WaterReturn;
    FishingActionTime = 0.f;
    bAnimateWater = bShowFishingAction = FishingAction != nullptr;
}

void UHumanFishingVisitorAnimation::ClearFishingAction()
{
    bAnimateWater = bShowFishingAction = false;
}

void UHumanFishingVisitorAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    if (!ActiveIdle)
    {
        ActiveIdle = StandingIdle;
    }
    if (bAnimateWater && FishingAction)
    {
        FishingActionTime += DeltaSeconds;
        const float Length = FishingAction->GetPlayLength();
        if (Length > UE_SMALL_NUMBER && FishingActionTime >= Length)
        {
            FishingActionTime = FMath::Fmod(FishingActionTime, Length);
            if (FishingAction == WaterEntry && WaterReel)
            {
                FishingAction = WaterReel;
            }
        }
    }

    const APawn* Pawn = TryGetPawnOwner();
    if (!Pawn)
    {
        return;
    }
    const float Speed = Pawn->GetVelocity().Size2D();
    const float Target = FMath::GetMappedRangeValueClamped(FVector2f(8.f, 110.f), FVector2f(0.f, 1.f), Speed);
    const float BlendTime = Target > WalkBlendProgress ? .30f : .42f;
    // Use the last gait phase at a standstill and ease its weight out. Reversing direction keeps the current blend.
    WalkRate = Speed > 8.f ? FMath::Clamp(Speed / 160.f, 0.f, 1.35f) : 0.f;
    WalkBlendProgress = FMath::FInterpConstantTo(WalkBlendProgress, Target, FMath::Min(DeltaSeconds, .05f), 1.f / BlendTime);
    WalkBlendWeight = FMath::SmoothStep(0.f, 1.f, WalkBlendProgress);
}
