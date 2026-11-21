#include "HumanFishingDisplay.h"
#include "HumanFishingMacDisplayBridge.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#include "Slate/SceneViewport.h"
#include "Misc/ConfigCacheIni.h"

void UHumanFishingDisplay::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if PLATFORM_MAC
    if(!GIsEditor)
    {ApplyMacPreset();TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UHumanFishingDisplay::Tick),.1f);}
#endif
}
void UHumanFishingDisplay::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
#if PLATFORM_MAC
    if(ObservedWindow)HFStopObservingMacWindow();
#endif
    Super::Deinitialize();
}
void UHumanFishingDisplay::ApplyMacPreset()
{
    int32 Version=0;GConfig->GetInt(TEXT("HumanFishing.MacDisplay"),TEXT("PresetVersion"),Version,GGameUserSettingsIni);
    auto* Settings=GEngine?GEngine->GetGameUserSettings():nullptr;
    if(Version>=1 || !Settings)return;
    Settings->SetOverallScalabilityLevel(1);
    Settings->SetResolutionScaleValueEx(75.f);Settings->SetTextureQuality(2);Settings->SetAntiAliasingQuality(2);
    Settings->SetGlobalIlluminationQuality(0);Settings->SetReflectionQuality(0);Settings->SetViewDistanceQuality(2);
    Settings->SetFrameRateLimit(60.f);Settings->SetVSyncEnabled(true);
    Settings->ApplyNonResolutionSettings();Settings->SaveSettings();
    GConfig->SetInt(TEXT("HumanFishing.MacDisplay"),TEXT("PresetVersion"),1,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
bool UHumanFishingDisplay::Tick(float)
{
#if PLATFORM_MAC
    if(!GEngine || !GEngine->GameViewport || !FSlateApplication::IsInitialized())return true;
    auto Window=GEngine->GameViewport->GetWindow();
    auto* Scene=static_cast<FSceneViewport*>(GEngine->GameViewport->Viewport);
    if(!Window.IsValid() || !Window->GetNativeWindow().IsValid() || !Scene)return true;
    void* Handle=Window->GetNativeWindow()->GetOSWindowHandle();
    if(!Handle)return true;
    if(ObservedWindow!=Handle)
    {ObservedWindow=Handle;HFObserveMacWindow(Handle);bPending=true;StableSamples=0;}
    if(HFMacWindowChanged()){bPending=true;StableSamples=0;}
    if(!bPending)return true;
    FHumanFishingMacView View;
    if(!HFReadMacGameView(Handle,View))return true;
    if(!View.Size.Equals(LastSize,.5) || !View.Origin.Equals(LastOrigin,.5))
    {LastSize=View.Size;LastOrigin=View.Origin;StableSamples=0;return true;}
    if(++StableSamples<2)return true;
    bPending=false;
    Window->SetViewportSizeDrivenByWindow(true);
    if(View.bFullscreen)
    {
        // Cocoa fullscreen content differs from NSScreen.visibleFrame on notched Retina displays.
        Window->SetCachedScreenPosition(View.Origin);Window->SetCachedSize(View.Size);
    }
    FSlateApplication::Get().InvalidateAllWidgets(false);
    Window->SlatePrepass(FSlateApplication::Get().GetApplicationScale()*Window->GetNativeWindow()->GetDPIScaleFactor());
    Scene->ResizeViewport(FMath::RoundToInt(View.Size.X),FMath::RoundToInt(View.Size.Y),Window->GetWindowMode());
    UE_LOG(LogTemp,Display,TEXT("HF_MAC_VIEW synchronized native=%dx%d origin=%.0f,%.0f fullscreen=%d"),
        FMath::RoundToInt(View.Size.X),FMath::RoundToInt(View.Size.Y),View.Origin.X,View.Origin.Y,View.bFullscreen);
#endif
    return true;
}
