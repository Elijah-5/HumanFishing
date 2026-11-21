#pragma once
#include "GameFramework/Actor.h"
#include "HumanFishingJellyshock.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;

UCLASS()
class HUMANFISHINGGAMEPLAY_API AHumanFishingJellyshockBurst : public AActor
{
    GENERATED_BODY()
public:
    AHumanFishingJellyshockBurst();
    void Configure(bool bHitShark,const FVector& Normal,bool bViolet=false);
    static void Spawn(UWorld* World,const FVector& Position,const FVector& Normal,bool bHitShark,bool bViolet=false);
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Rings;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Flash;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Arcs;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bubbles;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    float Age=0.f;
    bool bStrong=true;
    bool bViolet=false;
};

UCLASS()
class HUMANFISHINGGAMEPLAY_API AHumanFishingJellyshock : public AActor
{
    GENERATED_BODY()
public:
    AHumanFishingJellyshock();
    void ConfigureVariant(bool bViolet);
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    float DistanceTravelled=0.f;
    bool bLethal=false;
    float Age=0.f,TrailClock=0.f;
    UPROPERTY() TObjectPtr<USceneComponent> Visual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Bell;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Tentacles;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Trail;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Streaks;
    TArray<FVector> TrailPositions;
};
