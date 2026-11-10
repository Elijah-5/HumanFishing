#pragma once

#include "Widgets/SCompoundWidget.h"

class UHumanFishingPlayerUI;

class SHumanFishingHUD : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHumanFishingHUD) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& Arguments, UHumanFishingPlayerUI* InPlayerUI);
    TSharedPtr<SWidget> GetBackpack() const { return Backpack; }

private:
    TSharedPtr<SWidget> Backpack;
};
