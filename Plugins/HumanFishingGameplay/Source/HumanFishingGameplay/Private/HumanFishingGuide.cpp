#include "HumanFishingGuide.h"

#include "Engine/Texture2D.h"
#include "HumanFishingPlayerUI.h"
#include "HumanFishingSave.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "HumanFishingGuide"

void SetGuideBrush(FSlateBrush& Brush, UTexture2D* Texture, FLinearColor Fallback)
{
    if (Texture)
    {
        Brush.SetResourceObject(Texture);
        Brush.SetImageSize(FVector2D(Texture->GetSizeX(), Texture->GetSizeY()));
    }
    else
    {
        Brush = *FCoreStyle::Get().GetBrush("WhiteBrush");
        Brush.TintColor = FSlateColor(Fallback);
    }
}

namespace
{
const FLinearColor Paper(.89f, .80f, .65f);
const FLinearColor Canvas(.017f, .062f, .073f);
const FLinearColor GuideInk(.014f, .063f, .071f);
const FLinearColor GuideCream(.93f, .87f, .72f);
const FLinearColor Accent(.99f, .70f, .31f);
const FLinearColor Muted(.25f, .37f, .38f);

class SFieldGuide : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFieldGuide) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, UHumanFishingPlayerUI* InUI)
    {
        UI = InUI;
        const UHumanFishingCatalog* Catalog = InUI->GetCatalog();
        SetGuideBrush(Cover, Catalog ? Catalog->GuideCover.Get() : nullptr, Canvas);
        SetGuideBrush(Page, Catalog ? Catalog->GuidePaper.Get() : nullptr, Paper);
        SetGuideBrush(Unknown, Catalog ? Catalog->GuideUnknown.Get() : nullptr, Muted);
        SetGuideBrush(Book, Catalog ? Catalog->GuideBookIcon.Get() : nullptr);
        SetGuideBrush(Notes, Catalog ? Catalog->GuideNotesIcon.Get() : nullptr);
        SetGuideBrush(Tips, Catalog ? Catalog->GuideTipsIcon.Get() : nullptr);
        SetGuideBrush(Lock, Catalog ? Catalog->GuideLockIcon.Get() : nullptr);

        auto List = SNew(SScrollBox);
        if (Catalog)
        {
            Catalog->Types.GenerateKeyArray(Types);
            Types.Sort([Catalog](FName A, FName B)
            {
                const auto& First = Catalog->Types.FindChecked(A);
                const auto& Second = Catalog->Types.FindChecked(B);
                return First.Rarity == Second.Rarity ? A.LexicalLess(B) : First.Rarity < Second.Rarity;
            });
            for (FName Type : Types)
            {
                if (UTexture2D* Portrait = Catalog->Types.FindChecked(Type).Portrait.Get())
                {
                    SetGuideBrush(Portraits.Add(Type), Portrait);
                }
            }
            for (FName Type : Types)
            {
                List->AddSlot().Padding(0, 0, 9, 6)[MakeCard(Type)];
            }
        }
        Selected = Types.IsEmpty() ? NAME_None : Types[0];

        ChildSlot
        [
            SNew(SBox).WidthOverride(914).HeightOverride(512)
            [
                SNew(SBorder).BorderImage(&Cover).Padding(FMargin(28, 20, 28, 24))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
                    [
                        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(FLinearColor(.015f, .057f, .062f, .91f)).Padding(FMargin(10, 5))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
                            [ Icon(&Book, 28) ]
                            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                            [ Label(LOCTEXT("Title", "COASTAL FIELD GUIDE"), 22, GuideCream, true) ]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                            [
                                SNew(STextBlock).Text(this, &SFieldGuide::Progress)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)).ColorAndOpacity(Accent)
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().FillHeight(1)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 13, 0)
                        [
                            SNew(SBox).WidthOverride(246)
                            [ List ]
                        ]
                        + SHorizontalBox::Slot().FillWidth(1)
                        [
                            SNew(SBorder).BorderImage(&Page).Padding(21)
                            [
                                SAssignNew(Details, SScrollBox)
                                + SScrollBox::Slot()
                                [ MakeDetails() ]
                            ]
                        ]
                    ]
                ]
            ]
        ];
    }

private:
    static TSharedRef<STextBlock> Label(FText Text, int32 Size, FLinearColor Color, bool bBold = false)
    {
        return SNew(STextBlock).Text(Text).ColorAndOpacity(Color)
            .Font(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
    }

    static TSharedRef<SWidget> Icon(const FSlateBrush* Brush, float Size)
    {
        return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
            [ SNew(SImage).Image(Brush) ];
    }

    const FHumanFishingCatchType* Profile(FName Type) const
    {
        const auto* Catalog = UI.IsValid() ? UI->GetCatalog() : nullptr;
        return Catalog ? Catalog->Types.Find(Type) : nullptr;
    }

    bool IsDeveloper() const
    {
        const auto* Save = UI.IsValid() ? UI->GetSaveSubsystem() : nullptr;
        return Save && Save->IsDeveloperMode();
    }

    bool IsRevealed(FName Type) const
    {
        const auto* Inventory = UI.IsValid() ? UI->GetInventory() : nullptr;
        return !Type.IsNone() && (IsDeveloper() || (Inventory && Inventory->HasCaughtType(Type)));
    }

    FText Progress() const
    {
        int32 Count = 0;
        for (FName Type : Types)
        {
            Count += IsRevealed(Type) ? 1 : 0;
        }
        return FText::Format(IsDeveloper() ? LOCTEXT("Preview", "Developer Preview  •  {0}/{1}")
            : LOCTEXT("Discovered", "{0}/{1} discovered"), Count, Types.Num());
    }

    FText Name(FName Type) const
    {
        const auto* Entry = IsRevealed(Type) ? Profile(Type) : nullptr;
        return Entry ? Entry->DisplayName : LOCTEXT("Unknown", "???");
    }

    const FSlateBrush* Portrait(FName Type) const
    {
        const auto* Brush = IsRevealed(Type) ? Portraits.Find(Type) : nullptr;
        return Brush ? Brush : &Unknown;
    }

    FSlateColor RarityColor(FName Type) const
    {
        const auto* Entry = IsRevealed(Type) ? Profile(Type) : nullptr;
        return Entry ? UHumanFishingCatalog::GetRarityRules(Entry->Rarity).BackpackColor : Muted;
    }

    TSharedRef<SWidget> MakeCard(FName Type)
    {
        return SNew(SButton).ContentPadding(6).ButtonColorAndOpacity_Lambda([this, Type]()
            { return Selected == Type ? FLinearColor(.23f, .55f, .56f) : FLinearColor(.43f, .52f, .53f); })
            .OnClicked_Lambda([this, Type]()
            {
                Selected = Type;
                if (Details)
                {
                    Details->ScrollToStart();
                }
                return FReply::Handled();
            })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 7, 0)
                [
                    SNew(SBox).WidthOverride(4)
                    [
                        SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .ColorAndOpacity_Lambda([this, Type]() { return RarityColor(Type); })
                    ]
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                [
                    SNew(SBox).WidthOverride(43).HeightOverride(42)
                    [
                        SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                        [ SNew(SImage).Image_Lambda([this, Type]() { return Portrait(Type); }) ]
                    ]
                ]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Text_Lambda([this, Type]() { return Name(Type); }).AutoWrapText(true)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)).ColorAndOpacity(GuideCream)
                ]
            ];
    }

    TSharedRef<SWidget> Section(FText Title, const FSlateBrush* Brush, bool bTips)
    {
        return SNew(SVerticalBox)
            .Visibility_Lambda([this]() { return IsRevealed(Selected) ? EVisibility::Visible : EVisibility::Collapsed; })
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [ Icon(Brush, 20) ]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [ Label(Title, 16, GuideInk, true) ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock).ColorAndOpacity(GuideInk).AutoWrapText(true)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
                .Text_Lambda([this, bTips]()
                {
                    const auto* Entry = IsRevealed(Selected) ? Profile(Selected) : nullptr;
                    return Entry ? (bTips ? Entry->CaptureTips : Entry->FieldNotes) : FText::GetEmpty();
                })
            ];
    }

    TSharedRef<SWidget> MakeDetails()
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 13)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)
                [
                    SNew(SBox).WidthOverride(150).HeightOverride(136)
                    [
                        SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                        [ SNew(SImage).Image_Lambda([this]() { return Portrait(Selected); }) ]
                    ]
                ]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
                    [
                        SNew(STextBlock).Text_Lambda([this]() { return Name(Selected); })
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)).ColorAndOpacity(GuideInk).AutoWrapText(true)
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(STextBlock).ColorAndOpacity(GuideInk)
                        .Text_Lambda([this]()
                        {
                            const auto* Entry = IsRevealed(Selected) ? Profile(Selected) : nullptr;
                            return Entry ? StaticEnum<EHumanFishingRarity>()->GetDisplayNameTextByValue(
                                static_cast<int64>(Entry->Rarity)) : FText::GetEmpty();
                        })
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                    ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                .Visibility_Lambda([this]() { return IsRevealed(Selected) ? EVisibility::Collapsed : EVisibility::Visible; })
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                [ Icon(&Lock, 22) ]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [ Label(LOCTEXT("Locked", "Land this human to reveal the entry."), 15, GuideInk) ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
            [ Section(LOCTEXT("Notes", "Field Notes"), &Notes, false) ]
            + SVerticalBox::Slot().AutoHeight()
            [ Section(LOCTEXT("Tips", "Capture Tips"), &Tips, true) ];
    }

    TWeakObjectPtr<UHumanFishingPlayerUI> UI;
    TArray<FName> Types;
    TMap<FName, FSlateBrush> Portraits;
    FName Selected;
    TSharedPtr<SScrollBox> Details;
    FSlateBrush Cover;
    FSlateBrush Page;
    FSlateBrush Unknown;
    FSlateBrush Book;
    FSlateBrush Notes;
    FSlateBrush Tips;
    FSlateBrush Lock;
};
}

TSharedRef<SWidget> MakeHumanFishingGuide(UHumanFishingPlayerUI* UI)
{
    return SNew(SFieldGuide, UI);
}

#undef LOCTEXT_NAMESPACE
