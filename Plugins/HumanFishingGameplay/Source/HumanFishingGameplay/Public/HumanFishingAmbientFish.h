#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanFishingAmbientFish.generated.h"
class USkeletalMeshComponent;
class UAnimSequence;
class USkeletalMesh;

UCLASS()
class HUMANFISHINGGAMEPLAY_API AHumanFishingAmbientFish : public AActor
{
    GENERATED_BODY()
public:
    AHumanFishingAmbientFish();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ocean") TObjectPtr<USkeletalMeshComponent> Visual;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean") TObjectPtr<USkeletalMesh> FishMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean") TObjectPtr<UAnimSequence> SwimAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean") float SwimRadius=280.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean") float SwimSpeed=65.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ocean") float FishLength=55.f;
protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    FVector Home;
    float Phase=0;
};
