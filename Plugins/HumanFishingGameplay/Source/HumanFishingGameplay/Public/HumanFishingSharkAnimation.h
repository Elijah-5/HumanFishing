#pragma once
#include "Animation/AnimInstance.h"
#include "HumanFishingSharkAnimation.generated.h"
class UAnimSequence;

/** A small native animation graph: continuous swim plus an additive jaw control. */
UCLASS(Transient)
class HUMANFISHINGGAMEPLAY_API UHumanFishingSharkAnimation : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> SwimAnimation;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
