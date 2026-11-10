#pragma once

#include "Styling/SlateBrush.h"
#include "Widgets/SWidget.h"

class UHumanFishingPlayerUI;
class UTexture2D;

void SetGuideBrush(FSlateBrush& Brush, UTexture2D* Texture, FLinearColor Fallback = FLinearColor::Transparent);
TSharedRef<SWidget> MakeHumanFishingGuide(UHumanFishingPlayerUI* UI);
