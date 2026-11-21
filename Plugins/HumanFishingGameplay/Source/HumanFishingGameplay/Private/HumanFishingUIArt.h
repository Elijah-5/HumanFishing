#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SWidget.h"

enum class EHumanFishingUIArt:uint8 {Seaweed,CoveKey,Purchase,SellOne,SellAll,Upgrade,Locked,Backpack,Slot,ShopPanel,Jellyshock,TidalFins,VioletJellyshock,Count};
namespace HumanFishingUI
{
const FSlateBrush* Art(EHumanFishingUIArt Kind);
TSharedRef<SWidget> FitImage(const FSlateBrush* Brush);
TSharedRef<SWidget> Icon(EHumanFishingUIArt Kind,float Size=24.f);
TSharedRef<SWidget> Currency(TAttribute<FText> Amount,int32 FontSize=17,bool bDark=false);
}
