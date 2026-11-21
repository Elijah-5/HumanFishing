#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

/** Shared by Slate HUD/menu and the native presentation inside the UMG shop. */
namespace HumanFishingUI
{
inline FLinearColor Ink() { return FLinearColor(.016f, .033f, .048f, .97f); }
inline FLinearColor Panel() { return FLinearColor(.028f, .075f, .088f, .99f); }
inline FLinearColor Teal() { return FLinearColor(.31f, .86f, .77f); }
inline FLinearColor Gold() { return FLinearColor(.99f, .72f, .33f); }
inline FLinearColor Cream() { return FLinearColor(.93f, .87f, .72f); }
inline FLinearColor Text() { return FLinearColor(.92f, .97f, .98f); }
inline FLinearColor Muted() { return FLinearColor(.54f, .66f, .68f); }
inline FSlateFontInfo Font(int32 Size = 14, bool bBold = false)
{ return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size); }
inline const FSlateBrush* White() { return FCoreStyle::Get().GetBrush("WhiteBrush"); }
inline const FButtonStyle& Button()
{
    static const FButtonStyle Style = []
    {
        FButtonStyle S = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
        FSlateBrush Normal = *White(); Normal.TintColor = Panel();
        FSlateBrush Hover = Normal; Hover.TintColor = FLinearColor(.08f, .30f, .31f);
        FSlateBrush Press = Normal; Press.TintColor = FLinearColor(.05f, .22f, .24f);
        FSlateBrush Disabled = Normal; Disabled.TintColor = FLinearColor(.06f, .10f, .12f);
        S.SetNormal(Normal).SetHovered(Hover).SetPressed(Press).SetDisabled(Disabled);
        S.SetNormalPadding(FMargin(14, 9)).SetPressedPadding(FMargin(14, 10, 14, 8));
        return S;
    }();
    return Style;
}
inline const FButtonStyle& PrimaryButton()
{
    static const FButtonStyle Style=[]
    {
        FButtonStyle S=Button();FSlateBrush Normal=*White();Normal.TintColor=Gold();
        FSlateBrush Hover=Normal;Hover.TintColor=FLinearColor(1,.83f,.53f);
        FSlateBrush Pressed=Normal;Pressed.TintColor=FLinearColor(.73f,.49f,.22f);
        S.SetNormal(Normal).SetHovered(Hover).SetPressed(Pressed);return S;
    }();return Style;
}
inline const FButtonStyle& CompactButton()
{
    static const FButtonStyle Style=[]
    {FButtonStyle S=Button();S.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0,1,0,0));return S;}();
    return Style;
}
}
