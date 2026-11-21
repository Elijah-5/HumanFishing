#include "HumanFishingMacDisplayBridge.h"
#if PLATFORM_MAC
#include "Mac/CocoaThread.h"
#include "Mac/MacApplication.h"
#import <Cocoa/Cocoa.h>

namespace
{
TAtomic<bool> Dirty(true);
id Observers[6]={nil,nil,nil,nil,nil,nil};
}
void HFStopObservingMacWindow()
{
    MainThreadCall(^{for(id& Observer:Observers)if(Observer)
        {[[NSNotificationCenter defaultCenter] removeObserver:Observer];Observer=nil;}});
}
void HFObserveMacWindow(void* Handle)
{
    HFStopObservingMacWindow();
    MainThreadCall(^{
        NSNotificationName Names[]={NSWindowDidResizeNotification,NSWindowDidMoveNotification,
            NSWindowDidEnterFullScreenNotification,NSWindowDidExitFullScreenNotification,
            NSWindowDidChangeBackingPropertiesNotification,NSWindowDidChangeScreenNotification};
        NSWindow* Window=(NSWindow*)Handle;
        for(int32 I=0;I<6;++I)Observers[I]=[[NSNotificationCenter defaultCenter]
            addObserverForName:Names[I] object:Window queue:nil usingBlock:^(NSNotification*){Dirty.Store(true);}];
        Dirty.Store(true);
    });
}
bool HFMacWindowChanged(){return Dirty.Exchange(false);}
bool HFReadMacGameView(void* Handle,FHumanFishingMacView& Result)
{
    __block NSRect ScreenRect=NSZeroRect,PixelRect=NSZeroRect;
    __block bool Valid=false,Fullscreen=false;
    MainThreadCall(^{
        NSWindow* Window=(NSWindow*)Handle;
        NSView* View=[Window contentView];
        if(Window && View && [Window isVisible])
        {
            ScreenRect=[Window convertRectToScreen:[View convertRect:[View bounds] toView:nil]];
            PixelRect=[View convertRectToBacking:[View bounds]];
            Fullscreen=([Window styleMask]&NSWindowStyleMaskFullScreen)!=0;
            Valid=PixelRect.size.width>0 && PixelRect.size.height>0;
        }
    });
    if(!Valid)return false;
    Result.Size=FVector2D(FMath::RoundToInt(PixelRect.size.width),FMath::RoundToInt(PixelRect.size.height));
    Result.Origin=FMacApplication::ConvertCocoaPositionToSlate(NSMinX(ScreenRect),NSMaxY(ScreenRect));
    Result.bFullscreen=Fullscreen;
    return true;
}
#endif
