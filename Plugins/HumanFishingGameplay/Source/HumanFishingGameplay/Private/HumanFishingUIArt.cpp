#include "HumanFishingUIArt.h"
#include "HumanFishingUIStyle.h"
#include "Engine/Texture2D.h"
#include "UObject/GCObject.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
class FUIArt:public FGCObject
{
public:
    TArray<FSlateBrush> Brushes;
    FUIArt()
    {
        const TCHAR* Paths[]={
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_Seaweed.T_UI_Seaweed"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_CoveKey.T_UI_CoveKey"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_Purchase.T_UI_Purchase"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_SellOne.T_UI_SellOne"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_SellAll.T_UI_SellAll"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_Upgrade.T_UI_Upgrade"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_Locked.T_UI_Locked"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_Backpack.T_UI_Backpack"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_Slot.T_UI_Slot"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_ShopPanel.T_UI_ShopPanel"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_Jellyshock.T_UI_Jellyshock"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_TidalFins.T_UI_TidalFins"),
            TEXT("/Game/HumanFishing/UI/Theme/T_UI_VioletJellyshock.T_UI_VioletJellyshock")};
        static_assert(UE_ARRAY_COUNT(Paths)==static_cast<int32>(EHumanFishingUIArt::Count));
        Brushes.SetNum(UE_ARRAY_COUNT(Paths));
        for(int32 I=0;I<Brushes.Num();++I)
        {
            auto* Texture=LoadObject<UTexture2D>(nullptr,Paths[I]);
            Brushes[I].SetResourceObject(Texture);
            Brushes[I].SetImageSize(Texture?FVector2D(Texture->GetSizeX(),Texture->GetSizeY()):FVector2D(32));
            Brushes[I].DrawAs=ESlateBrushDrawType::Image;
            if(I==static_cast<int32>(EHumanFishingUIArt::Slot))
                Brushes[I].SetUVRegion(FBox2f(FVector2f(.12f,.12f),FVector2f(.88f,.88f)));
        }
    }
    virtual FString GetReferencerName() const override {return TEXT("HumanFishingUIArt");}
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override
    {for(const auto& Brush:Brushes){UObject* Object=Brush.GetResourceObject();Collector.AddReferencedObject(Object);}}
};
}
const FSlateBrush* HumanFishingUI::Art(EHumanFishingUIArt Kind)
{static FUIArt Resources;return &Resources.Brushes[static_cast<int32>(Kind)];}
TSharedRef<SWidget> HumanFishingUI::Icon(EHumanFishingUIArt Kind,float Size)
{return SNew(SBox).WidthOverride(Size).HeightOverride(Size)[FitImage(Art(Kind))];}
TSharedRef<SWidget> HumanFishingUI::FitImage(const FSlateBrush* Brush)
{return SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Brush)];}
TSharedRef<SWidget> HumanFishingUI::Currency(TAttribute<FText> Amount,int32 FontSize,bool bDark)
{
    return SNew(SHorizontalBox).ToolTipText(NSLOCTEXT("HFUI","SeaweedCurrency","Seaweed"))
        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,4,0)[Icon(EHumanFishingUIArt::Seaweed,FontSize+9)]
        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [SNew(STextBlock).Text(Amount).Font(Font(FontSize,true)).ColorAndOpacity(bDark?Ink():Text())];
}
