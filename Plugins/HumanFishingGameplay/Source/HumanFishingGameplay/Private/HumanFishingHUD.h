#pragma once

#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

class UHumanFishingPlayerUI;

class SHumanFishingHUD : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHumanFishingHUD) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& Arguments, UHumanFishingPlayerUI* InPlayerUI);
    TSharedPtr<SWidget> GetPausePanel() const { return PausePanel; }
    void ResetPausePage() { ResetPage.ExecuteIfBound(); }

private:
    FSlateBrush DollarBaitPortrait;
    FSlateBrush FlashBaitPortrait;
    TSharedPtr<SWidget> PausePanel;
    FSimpleDelegate ResetPage;
};
