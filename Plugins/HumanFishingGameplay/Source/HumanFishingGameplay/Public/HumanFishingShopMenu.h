#pragma once

#include "Blueprint/UserWidget.h"
#include "HumanFishingShopMenu.generated.h"

class UButton;
class UHumanFishingPlayerUI;
class UTextBlock;

UCLASS(Abstract)
class HUMANFISHINGGAMEPLAY_API UHumanFishingShopMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Shop")
    void CloseShop();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    TSharedPtr<SWidget> ShopContent;
    UHumanFishingPlayerUI* GetPlayerUI() const;

    UFUNCTION()
    void RedeemAll();

    UFUNCTION()
    void BuyMudskipper();

    UFUNCTION()
    void BuyBraidedLine();

    UFUNCTION()
    void BuyHundredDollarBill();

    void Refresh();

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> SellAllButton;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> MudskipperButton;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> MudskipperPrice;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> BraidedLineButton;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> BraidedLinePrice;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> HundredDollarBillButton;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> HundredDollarBillPrice;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> CloseButton;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> CapturesText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> SeaweedText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> Rate;
};
