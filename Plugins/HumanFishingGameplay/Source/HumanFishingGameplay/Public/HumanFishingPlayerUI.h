#pragma once

#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "HumanFishingPlayerUI.generated.h"

class APlayerController;
class SHumanFishingHUD;
class USpringArmComponent;
class UHumanFishingCatalog;
class UHumanFishingInventory;
class UHumanFishingSaveSubsystem;
class ULocalPlayer;
class UHumanFishingAbilities;

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
    /**
     * Applies damage from an attack and pauses deep-ocean healing for HealingDelay seconds.
     * At zero HP in the deep ocean, the fish returns to the level's spawn point. Returns true in that case.
     */
    bool ApplyAttackDamage(float Damage, float HealingDelay);
    bool TryStoreCatch(FName Type);
    void ShowCaptureToast(FName Type);
    /** Shows a short warning at the top of the screen, such as a shark alert. */
    void ShowAlert(const FText& Text, float Seconds = 3.f);
    FText GetAlert() const;
    void SetBackpackOpen(bool bOpen);
    void UpdateFishingCamera(bool bFishing);
    void ResetFishingCamera();
    FText GetTravelPrompt() const;
    int32 GetTravelUnlockCost() const;

    UFUNCTION(BlueprintPure, Category="Human Fishing|Backpack")
    bool IsBackpackOpen() const { return bBackpackOpen; }
    void SetShopOpen(bool bOpen);
    bool IsShopOpen() const { return bShopOpen; }
    bool IsDeepOcean() const { return bDeepOcean; }
    UHumanFishingAbilities* GetAbilities() const { return Abilities; }

    const UHumanFishingCatalog* GetCatalog() const { return Catalog.Get(); }
    UHumanFishingInventory* GetInventory() const;
    UHumanFishingSaveSubsystem* GetSaveSubsystem() const;
    FName GetToastType() const;
    bool IsNewGuideToast() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UHumanFishingAbilities> Abilities;
    void EnsureOverlay(APlayerController* Controller);
    APlayerController* GetLocalController() const;

    TSharedPtr<SHumanFishingHUD> Overlay;
    TWeakObjectPtr<ULocalPlayer> OverlayPlayer;
    TWeakObjectPtr<USpringArmComponent> FishingCameraBoom;
    FRotator DefaultCameraRotation = FRotator::ZeroRotator;
    float FishingCameraBlend = 0.f;
    bool bDeepOcean = false;
    bool bOnBeach = false;
    bool bEvacuating = false;
    bool bBackpackOpen = false;
    bool bShopOpen = false;
    FName ToastType;
    FName NewGuideType;
    double ToastUntil = 0;
    FText AlertText;
    double AlertUntil = 0;
    double HealingPausedUntil = 0;
};

UCLASS()
class HUMANFISHINGGAMEPLAY_API UHumanFishingBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait", meta=(WorldContext="WorldContextObject"))
    static bool ReserveFishingBait(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait", meta=(WorldContext="WorldContextObject"))
    static void HookFishingBait(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait", meta=(WorldContext="WorldContextObject"))
    static void FinishFishingBait(const UObject* WorldContextObject, bool bRetrieveBeforeHook);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Bait", meta=(WorldContext="WorldContextObject"))
    static bool CanUseFishingInput(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Bait", meta=(WorldContext="WorldContextObject"))
    static void PrepareFishingPause(UObject* WorldContextObject);

    /** Number of catches currently in the backpack. */
    UFUNCTION(BlueprintPure, Category="Human Fishing|Progress", meta=(WorldContext="WorldContextObject"))
    static int32 GetCaptureCount(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Progress", meta=(WorldContext="WorldContextObject"))
    static int32 GetSeaweedBalance(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category="Human Fishing|Fishing", meta=(WorldContext="WorldContextObject"))
    static double GetLineCapacity(const UObject* WorldContextObject, double BaseCapacity);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Health")
    static bool UpdateBeachHealth(APawn* Fish, bool bInWater, bool bGrounded);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    static bool TryStoreCatch(APawn* Fish, FName Type);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Backpack")
    static void ShowCaptureToast(APawn* Fish, FName Type);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Camera")
    static void UpdateFishingCamera(APawn* Fish, bool bFishing);

    UFUNCTION(BlueprintCallable, Category="Human Fishing|Camera")
    static void ResetFishingCamera(APawn* Fish);
};
