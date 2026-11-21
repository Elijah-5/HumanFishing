#pragma once
#include "CoreMinimal.h"
#if PLATFORM_MAC
struct FHumanFishingMacView
{
    FVector2D Size=FVector2D::ZeroVector;
    FVector2D Origin=FVector2D::ZeroVector;
    bool bFullscreen=false;
};
void HFObserveMacWindow(void* Handle);
void HFStopObservingMacWindow();
bool HFMacWindowChanged();
bool HFReadMacGameView(void* Handle,FHumanFishingMacView& View);
#endif
