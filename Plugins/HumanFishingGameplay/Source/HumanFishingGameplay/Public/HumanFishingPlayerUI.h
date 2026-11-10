#pragma once

#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "HumanFishingPlayerUI.generated.h"

class APlayerController;
class SHumanFishingHUD;
class UHumanFishingCatalog;
class UHumanFishingInventory;
class ULocalPlayer;

UINTERFACE(BlueprintType)
class HUMANFISHINGGAMEPLAY_API UHumanFishingBackpackListener : public UInterface
{
    GENERATED_BODY()
};

class HUMANFISHINGGAMEPLAY_API IHumanFishingBackpackListener
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category="Human Fishing|Backpack")
    void OnBackpackOpening();
};

UCLASS(ClassGroup=(HumanFishing), meta=(BlueprintSpawnableComponent))
class HUMANFISHINGGAMEPLAY_API UHumanFishingPlayerUI : public UActorComponent
{
    GENERATED_BODY()

public:
    UHumanFishingPlayerUI();

    UPROPERTY(EditDefaultsOnly, Category="Human Fishing|Backpack")
    TObjectPtr<UHumanFishingCatalog> Catalog;

    UPROPERTY(EditDefaultsOnly, Category="Human Fishing|Health", meta=(ClampMin="0.0"))
    float BeachDamagePerSecond = 10.f;

    UPROPERTY(EditDefaultsOnly, Category="Human Fishing|Health", meta=(ClampMin="0.0"))
    float OceanHealingPerSecond = 20.f;

    bool AdvanceBeachHealth(bool bInWater, bool bGrounded);
    bool TryStoreCatch(FName Type);
    void ShowCaptureToast(FName Type);
    void SetBackpackOpen(bool bOpen);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    bool IsBackpackOpen() const { return bBackpackOpen; }

    const UHumanFishingCatalog* GetCatalog() const { return Catalog.Get(); }
    UHumanFishingInventory* GetInventory() const;
    FName GetToastType() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void EnsureOverlay(APlayerController* Controller);
    APlayerController* GetLocalController() const;

    TSharedPtr<SHumanFishingHUD> Overlay;
    TWeakObjectPtr<ULocalPlayer> OverlayPlayer;
    bool bDeepOcean = false;
    bool bOnBeach = false;
    bool bEvacuating = false;
    bool bBackpackOpen = false;
    FName ToastType;
    double ToastUntil = 0;
};

UCLASS()
class HUMANFISHINGGAMEPLAY_API UHumanFishingBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Health")
    static bool UpdateBeachHealth(APawn* Fish, bool bInWater, bool bGrounded);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    static bool TryStoreCatch(APawn* Fish, FName Type);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    static void ShowCaptureToast(APawn* Fish, FName Type);
};
