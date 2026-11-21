#pragma once

#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

class UHumanFishingPlayerUI;
/** Shared live inventory, including purchased expansion, selection and drag behavior. */
TSharedRef<SWidget> MakeHumanFishingBackpack(UHumanFishingPlayerUI* UI,TFunction<void(int32)> OnSelected);

class SHumanFishingHUD : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHumanFishingHUD) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& Arguments, UHumanFishingPlayerUI* InPlayerUI);
    TSharedPtr<SWidget> GetPausePanel() const { return PausePanel; }
    void ResetPausePage() { ResetPage.ExecuteIfBound(); }

private:
    struct FBaitReadout
    {
        FSlateBrush Portrait;
        FText Text;
    };

    TArray<FBaitReadout> BaitReadouts;
    TSharedPtr<SWidget> PausePanel;
    FSimpleDelegate ResetPage;
};
