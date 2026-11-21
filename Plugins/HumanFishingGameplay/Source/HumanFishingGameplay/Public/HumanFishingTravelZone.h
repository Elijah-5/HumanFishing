#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanFishingTravelZone.generated.h"

class UBoxComponent;
class UTextRenderComponent;
class UStaticMeshComponent;
class APawn;

/** A visible, explicit interaction at each map exit. Never travels on overlap. */
UCLASS()
class HUMANFISHINGGAMEPLAY_API AHumanFishingTravelZone : public AActor
{
    GENERATED_BODY()
public:
    AHumanFishingTravelZone();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Travel")
    TSoftObjectPtr<UWorld> DestinationMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Travel")
    FName ArrivalTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Travel")
    FText DestinationLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Travel")
    TObjectPtr<UBoxComponent> TravelVolume;

    bool IsInRange(const APawn* Fish) const;
    bool IsLocked(const APawn* Fish) const;
    FText GetPrompt(const APawn* Fish) const;
    static AHumanFishingTravelZone* FindNearby(const APawn* Fish);
    void Interact();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    void CompleteTravel();
    UPROPERTY() TObjectPtr<UTextRenderComponent> Sign;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Beacon;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RangeHighlight;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> HighlightMaterial;
    bool bInputEnabled = false;
    bool bTravelling = false;
};
