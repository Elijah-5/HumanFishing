#include "HumanFishingShopMenu.h"
#include "HumanFishingHUD.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "HumanFishingUIArt.h"
#include "HumanFishingUIStyle.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "UObject/GCObject.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
enum class EOffer {Bill,Flash,Mudskipper,BraidedLine,Backpack,CoveKey,Jellyshock,TidalFins,VioletJellyshock};
struct FOffer {EOffer Id;const TCHAR* Name;const TCHAR* Description;const TCHAR* Group;int32 Price;};
const FOffer Offers[]={
    {EOffer::Bill,TEXT("$100 Bill"),TEXT("Attracts every visitor, including the Businessman. Normal interest gain."),TEXT("BAIT"),UHumanFishingInventory::HundredDollarBillCost},
    {EOffer::Flash,TEXT("Flash Bait"),TEXT("Attracts everyone. Prioritizes rare visitors, doubles interest gain and adds 4 seconds to hook."),TEXT("BAIT"),UHumanFishingInventory::FlashBaitCost},
    {EOffer::Jellyshock,TEXT("Jellyshock Capsule"),TEXT("A bottled electric jellyfish. Press F underwater to launch it. Stuns the first shark hit for 30 seconds; walls block the shot."),TEXT("DEFENSE"),UHumanFishingInventory::JellyshockCost},
    {EOffer::VioletJellyshock,TEXT("Violet Jellyshock"),TEXT("A volatile violet jellyfish. Press G underwater to launch it. Defeats the first shark hit in a violet nova. Walls block it; sharks return when the area reloads."),TEXT("DEFENSE"),UHumanFishingInventory::VioletJellyshockCost},
    {EOffer::Mudskipper,TEXT("Mudskipper"),TEXT("Permanent: take 50% less damage on land."),TEXT("UPGRADES"),UHumanFishingInventory::MudskipperCost},
    {EOffer::BraidedLine,TEXT("Braided Line"),TEXT("Permanent: 25% more line durability."),TEXT("UPGRADES"),UHumanFishingInventory::BraidedLineCost},
    {EOffer::Backpack,TEXT("Backpack Expansion"),TEXT("Permanent: expands your backpack from 4 x 4 to 5 x 4. Existing catches stay in place."),TEXT("UPGRADES"),UHumanFishingInventory::BackpackUpgradeCost},
    {EOffer::TidalFins,TEXT("Tidal Fins"),TEXT("Permanent: hold Shift underwater to swim 60% faster for up to 5 seconds. Release to recharge; empty fins need 8 seconds after a short rest."),TEXT("UPGRADES"),UHumanFishingInventory::TidalFinsCost},
    {EOffer::CoveKey,TEXT("Scenic Cove Key"),TEXT("Permanently unlocks the western rock-arch passage to Scenic Cove."),TEXT("EXPLORATION"),UHumanFishingInventory::CoveKeyCost}
};

class SSeaweedShop:public SCompoundWidget,public FGCObject
{
public:
    SLATE_BEGIN_ARGS(SSeaweedShop){} SLATE_END_ARGS()
    virtual bool SupportsKeyboardFocus() const override{return true;}
    virtual FReply OnKeyDown(const FGeometry& G,const FKeyEvent& E) override
    {
        if(E.GetKey()==EKeys::Escape || E.GetKey()==EKeys::E || E.GetKey()==EKeys::Tab)
        {if(Owner.IsValid())Owner->CloseShop();return FReply::Handled();}
        return SCompoundWidget::OnKeyDown(G,E);
    }
    virtual FString GetReferencerName() const override{return TEXT("SSeaweedShop");}
    virtual void AddReferencedObjects(FReferenceCollector& C) override
    {for(const auto& B:ItemIcons){UObject* O=B.GetResourceObject();C.AddReferencedObject(O);}}
    void Construct(const FArguments&,UHumanFishingShopMenu* InOwner)
    {
        Owner=InOwner;ItemIcons.SetNum(4);
        for(auto& Q:Quantity)Q=1;
        if(const auto* UI=PlayerUI())if(const auto* Catalog=UI->GetCatalog())
        {ItemIcons[0].SetResourceObject(Catalog->HundredDollarBillPortrait.Get());ItemIcons[1].SetResourceObject(Catalog->FlashBaitPortrait.Get());}
        ItemIcons[2].SetResourceObject(LoadObject<UTexture2D>(nullptr,TEXT("/Game/HumanFishing/Equipment/Icons/T_Mudskipper.T_Mudskipper")));
        ItemIcons[3].SetResourceObject(LoadObject<UTexture2D>(nullptr,TEXT("/Game/HumanFishing/Equipment/Icons/T_BraidedLine.T_BraidedLine")));
        for(auto& Icon:ItemIcons)if(const auto* Texture=Cast<UTexture2D>(Icon.GetResourceObject()))
            Icon.SetImageSize(FVector2D(Texture->GetSizeX(),Texture->GetSizeY()));
        auto Products=SNew(SVerticalBox);
        for(int32 I=0;I<UE_ARRAY_COUNT(Offers);++I)
        {
            if(I==0 || FCString::Strcmp(Offers[I].Group,Offers[I-1].Group)!=0)
                Products->AddSlot().AutoHeight().Padding(0,I?12:0,0,6)[Label(Offers[I].Group,14,HumanFishingUI::Gold(),true)];
            Products->AddSlot().AutoHeight().Padding(0,0,0,6)[Product(I)];
        }
        ChildSlot[SNew(SBorder).BorderImage(HumanFishingUI::White()).BorderBackgroundColor(FLinearColor(0,.012f,.02f,.82f))
            .HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24)
            [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
                [SNew(SBox).WidthOverride(1140).HeightOverride(720)
                    [SNew(SBorder).BorderImage(HumanFishingUI::Art(EHumanFishingUIArt::ShopPanel)).Padding(26)
                        [SNew(SVerticalBox)
                            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)
                            [SNew(SHorizontalBox)
                                +SHorizontalBox::Slot().FillWidth(1)[Label(TEXT("SEAWEED EXCHANGE"),28,HumanFishingUI::Cream(),true)]
                                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [HumanFishingUI::Currency(TAttribute<FText>::CreateLambda([this](){const auto* I=Inventory();return FText::AsNumber(I?I->GetSeaweed():0);}),25)]]
                            +SVerticalBox::Slot().FillHeight(1)
                            [SNew(SHorizontalBox)
                                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,24,0)[Backpack()]
                                +SHorizontalBox::Slot().FillWidth(1)[SNew(SScrollBox)+SScrollBox::Slot()[Products]]]
                            +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0,14,0,0)
                            [SNew(SButton).ButtonStyle(&HumanFishingUI::CompactButton()).ContentPadding(FMargin(12,8)).OnClicked_Lambda([this](){if(Owner.IsValid())Owner->CloseShop();return FReply::Handled();})
                                [Label(TEXT("Close (Esc)"),16,HumanFishingUI::Text(),true)]]
                        ]]]]];
    }
private:
    TWeakObjectPtr<UHumanFishingShopMenu> Owner;
    TArray<FSlateBrush> ItemIcons;
    int32 Quantity[UE_ARRAY_COUNT(Offers)]={};
    int32 Expanded=INDEX_NONE,Selected=INDEX_NONE;
    UHumanFishingPlayerUI* PlayerUI() const
    {auto* Pawn=Owner.IsValid()?Owner->GetOwningPlayerPawn():nullptr;return Pawn?Pawn->FindComponentByClass<UHumanFishingPlayerUI>():nullptr;}
    UHumanFishingInventory* Inventory() const{const auto* UI=PlayerUI();return UI?UI->GetInventory():nullptr;}
    bool IsConsumable(int32 I) const
    {return Offers[I].Id==EOffer::Bill || Offers[I].Id==EOffer::Flash || Offers[I].Id==EOffer::Jellyshock || Offers[I].Id==EOffer::VioletJellyshock;}
    int32 Cost(int32 I) const{return Offers[I].Price*(IsConsumable(I)?Quantity[I]:1);}
    int32 Stock(int32 I) const
    {const auto* V=Inventory();if(!V)return 0;return Offers[I].Id==EOffer::Jellyshock?V->GetJellyshockCount():Offers[I].Id==EOffer::VioletJellyshock?V->GetVioletJellyshockCount():V->GetBaitCount(Offers[I].Id==EOffer::Bill?EHumanFishingBait::HundredDollarBill:EHumanFishingBait::Flash);}
    bool Owned(int32 I) const
    {
        const auto* V=Inventory();if(!V)return false;
        switch(Offers[I].Id)
        {
        case EOffer::Mudskipper:return V->HasMudskipper();case EOffer::BraidedLine:return V->HasBraidedLine();
        case EOffer::Backpack:return V->HasBackpackUpgrade();case EOffer::CoveKey:return V->HasCoveKey();
        case EOffer::TidalFins:return V->HasTidalFins();default:return false;
        }
    }
    void Purchase(int32 I)
    {
        auto* V=Inventory();if(!V)return;
        switch(Offers[I].Id)
        {
        case EOffer::Bill:V->BuyBait(EHumanFishingBait::HundredDollarBill,Quantity[I]);break;
        case EOffer::Flash:V->BuyBait(EHumanFishingBait::Flash,Quantity[I]);break;
        case EOffer::Jellyshock:V->BuyJellyshock(Quantity[I]);break;
        case EOffer::VioletJellyshock:V->BuyVioletJellyshock(Quantity[I]);break;
        case EOffer::Mudskipper:V->BuyMudskipper();break;case EOffer::BraidedLine:V->BuyBraidedLine();break;
        case EOffer::Backpack:V->BuyBackpackUpgrade();break;case EOffer::CoveKey:V->BuyCoveKey();break;
        case EOffer::TidalFins:V->BuyTidalFins();break;
        }
    }
    int32 SaleValue() const{const auto* V=Inventory();const auto* UI=PlayerUI();return V && UI?V->GetRedemptionValue(UI->GetCatalog()):0;}
    int32 SelectedValue() const
    {const auto* V=Inventory();const auto* UI=PlayerUI();const auto* C=V?V->FindCatch(Selected):nullptr;return C && UI && UI->GetCatalog()?UI->GetCatalog()->GetCatchSeaweedValue(*C):0;}
    static TSharedRef<STextBlock> Label(const TCHAR* Text,int32 Size,FLinearColor Color,bool Bold=false)
    {return SNew(STextBlock).Text(FText::FromString(Text)).Font(HumanFishingUI::Font(Size,Bold)).ColorAndOpacity(Color);}
    TSharedRef<SWidget> Backpack()
    {
        return SNew(SBox).WidthOverride(352)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Label(TEXT("BACKPACK"),20,HumanFishingUI::Cream(),true)]]
                +SVerticalBox::Slot().AutoHeight()
                [SNew(SBox).WidthOverride(352).HeightOverride(352)
                    [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[MakeHumanFishingBackpack(PlayerUI(),[this](int32 Id){Selected=Id;})]]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,8)
                [SNew(STextBlock).Font(HumanFishingUI::Font(16)).ColorAndOpacity(HumanFishingUI::Muted())
                    .Text_Lambda([this](){const auto* V=Inventory();const auto* C=V?V->FindCatch(Selected):nullptr;const auto* UI=PlayerUI();
                        const auto* T=C && UI && UI->GetCatalog()?UI->GetCatalog()->Types.Find(C->Type):nullptr;
                        return T?T->DisplayName:FText::FromString(V && V->GetCatches().IsEmpty()?TEXT("Backpack empty"):TEXT("Select a catch"));})]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
                [SNew(SButton).ButtonStyle(&HumanFishingUI::CompactButton()).ContentPadding(FMargin(12,9)).IsEnabled_Lambda([this](){return SelectedValue()>0;})
                    .ToolTipText(FText::FromString(TEXT("Sell selected catch")))
                    .OnClicked_Lambda([this](){if(auto* V=Inventory()){V->RedeemCatch(PlayerUI()->GetCatalog(),Selected);Selected=INDEX_NONE;}return FReply::Handled();})
                    [SNew(SHorizontalBox)
                        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Label(TEXT("Sell selected"),18,HumanFishingUI::Text(),true)]
                        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[HumanFishingUI::Currency(TAttribute<FText>::CreateLambda([this](){return FText::AsNumber(SelectedValue());}),20)]]]
                +SVerticalBox::Slot().AutoHeight()
                [SNew(SButton).ButtonStyle(&HumanFishingUI::PrimaryButton()).ContentPadding(FMargin(12,14))
                    .IsEnabled_Lambda([this](){return SaleValue()>0;}).ToolTipText(FText::FromString(TEXT("Sell every catch in your backpack")))
                    .OnClicked_Lambda([this](){if(auto* V=Inventory())V->RedeemCatches(PlayerUI()->GetCatalog());Selected=INDEX_NONE;return FReply::Handled();})
                    [SNew(SHorizontalBox)
                        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Label(TEXT("Sell all"),23,HumanFishingUI::Ink(),true)]
                        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[HumanFishingUI::Currency(TAttribute<FText>::CreateLambda([this](){return FText::AsNumber(SaleValue());}),23,true)]]]
            ];
    }
    TSharedRef<SWidget> Stepper(int32 I)
    {
        return SNew(SBox).WidthOverride(122)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).ButtonStyle(&HumanFishingUI::CompactButton()).ContentPadding(FMargin(8,6))
                    .IsEnabled_Lambda([this,I](){return Quantity[I]>1;}).OnClicked_Lambda([this,I](){Quantity[I]=FMath::Max(1,Quantity[I]-1);return FReply::Handled();})[Label(TEXT("−"),19,HumanFishingUI::Text())]]
                +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(2,0)
                [SNew(SNumericEntryBox<int32>).Font(HumanFishingUI::Font(18,true)).MinValue(1).MaxValue(99).AllowSpin(false).MinDesiredValueWidth(28)
                    .Value_Lambda([this,I](){return TOptional<int32>(Quantity[I]);})
                    .OnValueChanged_Lambda([this,I](int32 N){Quantity[I]=FMath::Clamp(N,1,99);})]
                +SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).ButtonStyle(&HumanFishingUI::CompactButton()).ContentPadding(FMargin(8,6))
                    .IsEnabled_Lambda([this,I](){return Quantity[I]<99;}).OnClicked_Lambda([this,I](){Quantity[I]=FMath::Min(99,Quantity[I]+1);return FReply::Handled();})[Label(TEXT("+"),19,HumanFishingUI::Text())]]];
    }
    TSharedRef<SWidget> Product(int32 I)
    {
        const auto Id=Offers[I].Id;
        const auto Art=Id==EOffer::VioletJellyshock?EHumanFishingUIArt::VioletJellyshock:Id==EOffer::Jellyshock?EHumanFishingUIArt::Jellyshock:Id==EOffer::TidalFins?EHumanFishingUIArt::TidalFins:Id==EOffer::Backpack?EHumanFishingUIArt::Backpack:EHumanFishingUIArt::CoveKey;
        auto Image=static_cast<int32>(Id)<4?HumanFishingUI::FitImage(&ItemIcons[static_cast<int32>(Id)]):HumanFishingUI::Icon(Art,48);
        return SNew(SBorder).BorderImage(HumanFishingUI::White()).BorderBackgroundColor(HumanFishingUI::Panel()).Padding(10)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,10,0)
                    [SNew(SBox).WidthOverride(64).HeightOverride(64)[SNew(SOverlay)
                        +SOverlay::Slot()[HumanFishingUI::FitImage(HumanFishingUI::Art(EHumanFishingUIArt::Slot))]
                        +SOverlay::Slot().Padding(9)[Image]]]
                    +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0,0,10,0)
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight()
                        [SNew(SButton).ButtonStyle(&HumanFishingUI::CompactButton()).ContentPadding(0).ToolTipText(FText::FromString(TEXT("Show item details")))
                            .OnClicked_Lambda([this,I](){Expanded=Expanded==I?INDEX_NONE:I;return FReply::Handled();})
                            [SNew(STextBlock).Font(HumanFishingUI::Font(20,true)).ColorAndOpacity(HumanFishingUI::Cream())
                                .Text_Lambda([this,I](){return FText::FromString(IsConsumable(I)?FString::Printf(TEXT("%s (%d)"),Offers[I].Name,Stock(I)):FString(Offers[I].Name));})]]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
                        [SNew(SBox).Visibility(IsConsumable(I)?EVisibility::Visible:EVisibility::Collapsed)
                            [HumanFishingUI::Currency(TAttribute<FText>(FText::FromString(FString::FromInt(Offers[I].Price)+TEXT(" / pc"))),16)]]]
                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,10,0)
                    [SNew(SBox).Visibility(IsConsumable(I)?EVisibility::Visible:EVisibility::Collapsed)[IsConsumable(I)?Stepper(I):StaticCastSharedRef<SWidget>(SNew(SBox))]]
                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [SNew(SBox).WidthOverride(136)
                        [SNew(SVerticalBox)
                            +SVerticalBox::Slot().AutoHeight()
                            [SNew(SButton).ButtonStyle(&HumanFishingUI::CompactButton()).ContentPadding(FMargin(8,10))
                                .IsEnabled_Lambda([this,I](){const auto* V=Inventory();return V && !Owned(I) && V->GetSeaweed()>=Cost(I);})
                                .ToolTipText_Lambda([this,I](){return FText::FromString(Owned(I)?TEXT("Already owned"):FString::Printf(TEXT("Buy %s"),Offers[I].Name));})
                                .OnClicked_Lambda([this,I](){Purchase(I);return FReply::Handled();})
                                [SNew(SWidgetSwitcher).WidgetIndex_Lambda([this,I](){return Owned(I)?1:0;})
                                    +SWidgetSwitcher::Slot()
                                    [SNew(SHorizontalBox)
                                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                    [HumanFishingUI::Currency(TAttribute<FText>::CreateLambda([this,I](){return FText::AsNumber(Cost(I));}),20)]]
                                    +SWidgetSwitcher::Slot()
                                    [SNew(SHorizontalBox)
                                        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(TEXT("Owned"),15,HumanFishingUI::Teal(),true)]]]]
                            +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                            [SNew(STextBlock).Font(HumanFishingUI::Font(12)).ColorAndOpacity(HumanFishingUI::Gold())
                                .Text_Lambda([this,I](){const auto* V=Inventory();return FText::FromString(V && !Owned(I) && V->GetSeaweed()<Cost(I)?TEXT("Not enough"):TEXT(""));})]]]
                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6,0,0,0)
                    [SNew(SButton).ButtonStyle(&HumanFishingUI::CompactButton()).ContentPadding(FMargin(5,8)).ToolTipText(FText::FromString(TEXT("Item details")))
                        .OnClicked_Lambda([this,I](){Expanded=Expanded==I?INDEX_NONE:I;return FReply::Handled();})
                        [SNew(SBox).WidthOverride(12).HeightOverride(12)
                            [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                                [SNew(SImage).ColorAndOpacity(HumanFishingUI::Muted()).Image_Lambda([this,I](){return FCoreStyle::Get().GetBrush(Expanded==I?TEXT("TreeArrow_Expanded"):TEXT("TreeArrow_Collapsed"));})]]]]]
                +SVerticalBox::Slot().AutoHeight().Padding(58,9,4,2)
                [SNew(SBox).Visibility_Lambda([this,I](){return Expanded==I?EVisibility::Visible:EVisibility::Collapsed;})
                    [SNew(STextBlock).Text(FText::FromString(Offers[I].Description)).AutoWrapText(true).Font(HumanFishingUI::Font(16)).ColorAndOpacity(HumanFishingUI::Text())]]
            ];
    }
};
}

UHumanFishingPlayerUI* UHumanFishingShopMenu::GetPlayerUI() const
{const auto* Pawn=GetOwningPlayerPawn();return Pawn?Pawn->FindComponentByClass<UHumanFishingPlayerUI>():nullptr;}
TSharedRef<SWidget> UHumanFishingShopMenu::RebuildWidget(){ShopContent=SNew(SSeaweedShop,this);return ShopContent.ToSharedRef();}
void UHumanFishingShopMenu::NativeOnInitialized(){Super::NativeOnInitialized();SetIsFocusable(true);}
void UHumanFishingShopMenu::NativeConstruct()
{
    Super::NativeConstruct();if(auto* UI=GetPlayerUI())UI->SetShopOpen(true);
    GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this]()
    {if(IsInViewport() && ShopContent.IsValid())if(auto* PC=GetOwningPlayer())
      {FInputModeUIOnly Mode;Mode.SetWidgetToFocus(ShopContent);PC->SetInputMode(Mode);PC->SetShowMouseCursor(true);FSlateApplication::Get().SetKeyboardFocus(ShopContent,EFocusCause::SetDirectly);}}));
}
void UHumanFishingShopMenu::NativeDestruct(){if(auto* UI=GetPlayerUI())UI->SetShopOpen(false);Super::NativeDestruct();}
void UHumanFishingShopMenu::CloseShop()
{
    if(!IsInViewport())return;if(auto* UI=GetPlayerUI())UI->SetShopOpen(false);
    if(auto* PC=GetOwningPlayer()){PC->SetIgnoreMoveInput(false);PC->SetIgnoreLookInput(false);PC->SetShowMouseCursor(false);PC->FlushPressedKeys();PC->SetInputMode(FInputModeGameOnly());}
    FSlateApplication::Get().CancelDragDrop();RemoveFromParent();
}
FReply UHumanFishingShopMenu::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{if(E.GetKey()==EKeys::E || E.GetKey()==EKeys::Escape || E.GetKey()==EKeys::Tab){CloseShop();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}
void UHumanFishingShopMenu::RedeemAll(){if(auto* UI=GetPlayerUI())if(auto* I=UI->GetInventory())I->RedeemCatches(UI->GetCatalog());}
void UHumanFishingShopMenu::BuyMudskipper(){if(auto* UI=GetPlayerUI())if(auto* I=UI->GetInventory())I->BuyMudskipper();}
void UHumanFishingShopMenu::BuyBraidedLine(){if(auto* UI=GetPlayerUI())if(auto* I=UI->GetInventory())I->BuyBraidedLine();}
void UHumanFishingShopMenu::BuyHundredDollarBill(){if(auto* UI=GetPlayerUI())if(auto* I=UI->GetInventory())I->BuyHundredDollarBill();}
void UHumanFishingShopMenu::Refresh(){}
