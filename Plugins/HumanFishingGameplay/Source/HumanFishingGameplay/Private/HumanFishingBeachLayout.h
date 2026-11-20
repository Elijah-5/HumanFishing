#pragma once

#include "Components/PrimitiveComponent.h"
#include "HumanFishingBeachLayout.generated.h"

/**
 * Editor-only lines that show the beach areas of the owning AHumanFishingBeachCrowd.
 * Hidden in game, without collision or navigation impact.
 */
UCLASS()
class UHumanFishingBeachLayoutComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UHumanFishingBeachLayoutComponent();

#if WITH_EDITOR
    struct FLine
    {
        FVector Start;
        FVector End;
        FLinearColor Color;
    };

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TArray<FLine> Lines;
    FBox DrawnBounds = FBox(ForceInit);
#endif
};
