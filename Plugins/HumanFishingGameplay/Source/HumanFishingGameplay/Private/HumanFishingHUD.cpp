#include "HumanFishingHUD.h"

#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HumanFishingGuide.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "HumanFishingSave.h"
#include "Input/DragAndDrop.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "HumanFishingHUD"

namespace
{
constexpr float CellSize = 92.f;
const FLinearColor Ink(.016f, .033f, .048f, .97f);
const FLinearColor Teal(.31f, .86f, .77f);
const FLinearColor Gold(.99f, .72f, .33f);

const FSlateBrush* WhiteBrush()
{
    return FCoreStyle::Get().GetBrush("WhiteBrush");
}

const FHumanFishingCatchType* Lookup(const UHumanFishingPlayerUI* UI, FName Type)
{
    const UHumanFishingCatalog* Catalog = UI ? UI->GetCatalog() : nullptr;
    return Catalog ? Catalog->Types.Find(Type) : nullptr;
}

class FCatchDrag : public FDragDropOperation
{
public:
    DRAG_DROP_OPERATOR_TYPE(FCatchDrag, FDragDropOperation)

    static TSharedRef<FCatchDrag> New(int32 CatchId, FVector2D PickupOffset)
    {
        TSharedRef<FCatchDrag> Drag = MakeShared<FCatchDrag>();
        Drag->Id = CatchId;
        Drag->Offset = PickupOffset;
        Drag->Construct();
        return Drag;
    }

    int32 Id = INDEX_NONE;
    FVector2D Offset;
};

class SBackpackGrid : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SBackpackGrid) {}
        SLATE_EVENT(FSimpleDelegate, OnSelectionChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args, UHumanFishingPlayerUI* UI)
    {
        PlayerUI = UI;
        OnSelectionChanged = Args._OnSelectionChanged;
        if (const UHumanFishingCatalog* Catalog = UI->GetCatalog())
        {
            for (const auto& Pair : Catalog->Types)
            {
                if (const UTexture2D* Texture = Pair.Value.Portrait.Get())
                {
                    FSlateBrush& Brush = Portraits.Add(Pair.Key);
                    Brush.SetResourceObject(Pair.Value.Portrait.Get());
                    Brush.SetImageSize(FVector2D(Texture->GetSizeX(), Texture->GetSizeY()));
                }
            }
        }
    }

    void ClearPreview()
    {
        PreviewId = INDEX_NONE;
        PressedId = INDEX_NONE;
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    const FHumanFishingCatch* SelectedCatch() const
    {
        const UHumanFishingInventory* Data = Inventory();
        return Data ? Data->FindCatch(SelectedId) : nullptr;
    }

    void Select(int32 Id)
    {
        if (SelectedId != Id)
        {
            SelectedId = Id;
            Invalidate(EInvalidateWidgetReason::Paint);
            OnSelectionChanged.ExecuteIfBound();
        }
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(CellSize * UHumanFishingInventory::Columns, CellSize * UHumanFishingInventory::Rows);
    }

    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
    {
        const FVector2D Cell = Unit(Geometry);
        auto Draw = [&](int32 Z, FVector2D At, FVector2D Size, const FSlateBrush* Brush, FLinearColor Color)
        {
            FSlateDrawElement::MakeBox(Out, Layer + Z,
                Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(At)), Brush, ESlateDrawEffect::None, Color);
        };

        for (int32 Row = 0; Row < UHumanFishingInventory::Rows; ++Row)
        {
            for (int32 Col = 0; Col < UHumanFishingInventory::Columns; ++Col)
            {
                const FVector2D Position(Cell.X * Col, Cell.Y * Row);
                Draw(0, Position + FVector2D(2), Cell - FVector2D(4), WhiteBrush(), FLinearColor(.040f, .096f, .119f, 1));
            }
        }

        const UHumanFishingInventory* Data = Inventory();
        if (!Data)
        {
            return Layer;
        }

        for (const FHumanFishingCatch& Item : Data->GetCatches())
        {
            const FVector2D Origin(Cell.X * Item.Cell.X, Cell.Y * Item.Cell.Y);
            const FVector2D Size(Cell.X * Item.Size.X, Cell.Y * Item.Size.Y);
            const float Opacity = Item.Id == PreviewId ? .35f : 1.f;
            const FHumanFishingCatchType* Profile = Lookup(PlayerUI.Get(), Item.Type);
            FLinearColor Color = UHumanFishingCatalog::GetRarityRules(
                Profile ? Profile->Rarity : EHumanFishingRarity::White).BackpackColor;
            Color.A = Opacity;
            Draw(1, Origin + FVector2D(3), Size - FVector2D(6), WhiteBrush(), Color);
            if (const FSlateBrush* Portrait = Portraits.Find(Item.Type))
            {
                const FVector2D Pixels(Portrait->GetImageSize());
                const FVector2D Interior = Size - FVector2D(8);
                const double Scale = FMath::Min(Interior.X / Pixels.X, Interior.Y / Pixels.Y);
                const FVector2D ImageSize = Pixels * Scale;
                Draw(2, Origin + (Size - ImageSize) * .5, ImageSize, Portrait, FLinearColor(1, 1, 1, Opacity));
            }
            if (Item.Id == SelectedId && Item.Id != PreviewId)
            {
                const FVector2D TopLeft = Origin + FVector2D(4);
                const FVector2D BottomRight = Origin + Size - FVector2D(4);
                const TArray<FVector2D> Outline = {TopLeft, {BottomRight.X, TopLeft.Y},
                    BottomRight, {TopLeft.X, BottomRight.Y}, TopLeft};
                FSlateDrawElement::MakeLines(Out, Layer + 3, Geometry.ToPaintGeometry(), Outline,
                    ESlateDrawEffect::None, Teal, true, 2.f);
            }
        }

        if (const FHumanFishingCatch* Item = Data->FindCatch(PreviewId))
        {
            const bool bFits = Data->CanPlace(PreviewCell, Item->Size, Item->Id);
            const FLinearColor Color = bFits ? FLinearColor(.25f, .88f, .60f, .42f) : FLinearColor(.98f, .22f, .16f, .42f);
            for (int32 Row = 0; Row < Item->Size.Y; ++Row)
            {
                for (int32 Col = 0; Col < Item->Size.X; ++Col)
                {
                    const FIntPoint At = PreviewCell + FIntPoint(Col, Row);
                    if (At.X >= 0 && At.X < UHumanFishingInventory::Columns && At.Y >= 0 && At.Y < UHumanFishingInventory::Rows)
                    {
                        Draw(4, FVector2D(Cell.X * At.X, Cell.Y * At.Y) + FVector2D(2),
                            Cell - FVector2D(4), WhiteBrush(), Color);
                    }
                }
            }
        }
        return Layer + 4;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            return FReply::Unhandled();
        }

        PressedId = INDEX_NONE;
        if (const UHumanFishingInventory* Data = Inventory())
        {
            const FVector2D Cursor = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
            const FVector2D Cell = Unit(Geometry);
            const FIntPoint Hit = AtCell(Cursor, Cell);
            for (const FHumanFishingCatch& Item : Data->GetCatches())
            {
                const FIntPoint Relative = Hit - Item.Cell;
                if (Relative.X >= 0 && Relative.X < Item.Size.X && Relative.Y >= 0 && Relative.Y < Item.Size.Y)
                {
                    PressedId = Item.Id;
                    Select(Item.Id);
                    PickupOffset = Cursor - FVector2D(Cell.X * Item.Cell.X, Cell.Y * Item.Cell.Y);
                    return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
                }
            }
        }
        Select(INDEX_NONE);
        return FReply::Handled();
    }

    virtual FReply OnDragDetected(const FGeometry&, const FPointerEvent&) override
    {
        if (PressedId == INDEX_NONE)
        {
            return FReply::Unhandled();
        }
        OnSelectionChanged.ExecuteIfBound();
        TSharedRef<FCatchDrag> Drag = FCatchDrag::New(PressedId, PickupOffset);
        return FReply::Handled().BeginDragDrop(Drag);
    }

    virtual FReply OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event) override
    {
        const TSharedPtr<FCatchDrag> Drag = Event.GetOperationAs<FCatchDrag>();
        if (!Drag)
        {
            return FReply::Unhandled();
        }
        PreviewId = Drag->Id;
        PreviewCell = DropCell(Geometry, Event, *Drag);
        Invalidate(EInvalidateWidgetReason::Paint);
        return FReply::Handled();
    }

    virtual void OnDragLeave(const FDragDropEvent&) override
    {
        ClearPreview();
    }

    virtual FReply OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event) override
    {
        const TSharedPtr<FCatchDrag> Drag = Event.GetOperationAs<FCatchDrag>();
        UHumanFishingInventory* Data = Inventory();
        if (!Drag || !Data)
        {
            return FReply::Unhandled();
        }

        Data->TryMoveCatch(Drag->Id, DropCell(Geometry, Event, *Drag));
        ClearPreview();
        return FReply::Handled();
    }

private:
    UHumanFishingInventory* Inventory() const
    {
        return PlayerUI.IsValid() ? PlayerUI->GetInventory() : nullptr;
    }

    static FVector2D Unit(const FGeometry& Geometry)
    {
        const FVector2D Size = Geometry.GetLocalSize();
        return FVector2D(Size.X / UHumanFishingInventory::Columns, Size.Y / UHumanFishingInventory::Rows);
    }

    static FIntPoint AtCell(FVector2D Cursor, FVector2D Cell)
    {
        return FIntPoint(FMath::FloorToInt(Cursor.X / Cell.X), FMath::FloorToInt(Cursor.Y / Cell.Y));
    }

    static FIntPoint DropCell(const FGeometry& Geometry, const FDragDropEvent& Event, const FCatchDrag& Drag)
    {
        return AtCell(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()) - Drag.Offset, Unit(Geometry));
    }

    TWeakObjectPtr<UHumanFishingPlayerUI> PlayerUI;
    TMap<FName, FSlateBrush> Portraits;
    FSimpleDelegate OnSelectionChanged;
    int32 SelectedId = INDEX_NONE;
    int32 PressedId = INDEX_NONE;
    int32 PreviewId = INDEX_NONE;
    FIntPoint PreviewCell;
    FVector2D PickupOffset;
};

class SBackpackPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SBackpackPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, UHumanFishingPlayerUI* UI)
    {
        PlayerUI = UI;
        if (const UHumanFishingCatalog* Catalog = UI->GetCatalog())
        {
            Frame.SetResourceObject(Catalog->BackpackFrame.Get());
        }
        SAssignNew(Grid, SBackpackGrid, UI).OnSelectionChanged(this, &SBackpackPanel::CancelDiscard);

        ChildSlot
        [
            SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(.005f, .015f, .024f, .84f))
            .HAlign(HAlign_Center).VAlign(VAlign_Center)
            [
                SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)
                    [
                        SNew(STextBlock).Text(LOCTEXT("Backpack", "BACKPACK"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)).ColorAndOpacity(Teal)
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SBox).WidthOverride(484).HeightOverride(484)
                        [
                            SNew(SOverlay)
                            + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                            [
                                SNew(SBorder).Padding(0).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink)
                                [ Grid.ToSharedRef() ]
                            ]
                            + SOverlay::Slot()
                            [ SNew(SImage).Image(&Frame).Visibility(EVisibility::HitTestInvisible) ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
                    [
                        SNew(SBox).WidthOverride(460).HeightOverride(80)
                        [
                            SNew(SBorder).Padding(FMargin(12, 8)).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(STextBlock).Text(this, &SBackpackPanel::CatchDetails)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                    .ColorAndOpacity(FLinearColor(.92f, .97f, .98f))
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 0)
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                                    [
                                        SNew(SButton)
                                        .ContentPadding(FMargin(12, 5))
                                        .Text_Lambda([this]() {
                                            return IsConfirmingDiscard() ? LOCTEXT("ConfirmDiscard", "Confirm discard") : LOCTEXT("Discard", "Discard...");
                                        })
                                        .Visibility_Lambda([this]() { return Grid->SelectedCatch() ? EVisibility::Visible : EVisibility::Hidden; })
                                        .OnClicked(this, &SBackpackPanel::DiscardSelected)
                                    ]
                                    + SHorizontalBox::Slot().AutoWidth()
                                    [
                                        SNew(SButton).Text(LOCTEXT("KeepCatch", "Keep"))
                                        .ContentPadding(FMargin(12, 5))
                                        .Visibility_Lambda([this]() { return IsConfirmingDiscard() ? EVisibility::Visible : EVisibility::Collapsed; })
                                        .OnClicked_Lambda([this]() { CancelDiscard(); return FReply::Handled(); })
                                    ]
                                ]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
                    [
                        SNew(STextBlock).Text(LOCTEXT("BackpackKeys", "Click for value     •     Drag to rearrange     •     Tab / Esc to close"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
                        .ColorAndOpacity(FLinearColor(.72f, .84f, .87f))
                    ]
                ]
            ]
        ];
    }

    void ResetSelection()
    {
        FSlateApplication::Get().CancelDragDrop();
        Grid->ClearPreview();
        Grid->Select(INDEX_NONE);
        CancelDiscard();
    }

private:
    FText CatchDetails() const
    {
        const FHumanFishingCatch* Item = Grid->SelectedCatch();
        if (!Item)
        {
            return LOCTEXT("SelectCatch", "Select a catch to see its value");
        }
        const UHumanFishingCatalog* Catalog = PlayerUI.IsValid() ? PlayerUI->GetCatalog() : nullptr;
        const FHumanFishingCatchType* Profile = Lookup(PlayerUI.Get(), Item->Type);
        const EHumanFishingRarity Rarity = Profile ? Profile->Rarity : EHumanFishingRarity::White;
        return FText::Format(LOCTEXT("CatchDetails", "{0}  •  {1}  •  {2}×{3}  •  {4} Seaweed"),
            Profile ? Profile->DisplayName : FText::FromName(Item->Type),
            StaticEnum<EHumanFishingRarity>()->GetDisplayNameTextByValue(static_cast<int64>(Rarity)),
            Item->Size.X, Item->Size.Y, Catalog ? Catalog->GetCatchSeaweedValue(*Item) : 0);
    }

    bool IsConfirmingDiscard() const
    {
        const FHumanFishingCatch* Item = Grid->SelectedCatch();
        return Item && Item->Id == DiscardId;
    }

    void CancelDiscard()
    {
        DiscardId = INDEX_NONE;
    }

    FReply DiscardSelected()
    {
        const FHumanFishingCatch* Item = Grid->SelectedCatch();
        UHumanFishingInventory* Inventory = PlayerUI.IsValid() ? PlayerUI->GetInventory() : nullptr;
        if (Item && Inventory)
        {
            if (IsConfirmingDiscard())
            {
                Inventory->TryDiscardCatch(DiscardId);
                Grid->Select(INDEX_NONE);
                Grid->ClearPreview();
            }
            else
            {
                DiscardId = Item->Id;
            }
        }
        return FReply::Handled();
    }

    TWeakObjectPtr<UHumanFishingPlayerUI> PlayerUI;
    TSharedPtr<SBackpackGrid> Grid;
    FSlateBrush Frame;
    int32 DiscardId = INDEX_NONE;
};

class SFishingPausePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFishingPausePanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, UHumanFishingPlayerUI* UI)
    {
        PlayerUI = UI;
        Backpack = SNew(SBackpackPanel, UI);
        ChildSlot
        [
            SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(.005f, .015f, .024f, .88f))
            .HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(14)
            [
                SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
                        [ Tab(LOCTEXT("BackpackTab", "Backpack"), 0) ]
                        + SHorizontalBox::Slot().AutoWidth()
                        [ Tab(LOCTEXT("GuideTab", "Field Guide"), 1) ]
                        + SHorizontalBox::Slot().FillWidth(1)
                        [ SNew(SSpacer) ]
                        + SHorizontalBox::Slot().AutoWidth()
                        [
                            SNew(SButton).Text(LOCTEXT("ClosePause", "Close")).ContentPadding(FMargin(14, 5))
                            .OnClicked(this, &SFishingPausePanel::Close)
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SAssignNew(Pages, SWidgetSwitcher)
                        + SWidgetSwitcher::Slot()[Backpack.ToSharedRef()]
                        + SWidgetSwitcher::Slot()[MakeHumanFishingGuide(UI)]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth()
                        [
                            SNew(SButton).ContentPadding(FMargin(14, 5))
                            .Text_Lambda([this]()
                            {
                                const auto* Save = SaveSubsystem();
                                return Save && Save->IsDeveloperMode() ? LOCTEXT("DiscardTest", "Discard Test & Return")
                                    : LOCTEXT("SaveReturn", "Save & Return");
                            })
                            .OnClicked(this, &SFishingPausePanel::ReturnToMenu)
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 8, 0)
                        [
                            SNew(SButton).Text(LOCTEXT("RetrySave", "Retry Save"))
                            .Visibility(this, &SFishingPausePanel::SaveErrorVisibility)
                            .OnClicked_Lambda([this]()
                            {
                                if (auto* Save = SaveSubsystem())
                                {
                                    Save->Flush();
                                }
                                return FReply::Handled();
                            })
                        ]
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                        [
                            SNew(STextBlock).Text(LOCTEXT("SaveError", "Save failed. The previous save is available."))
                            .ColorAndOpacity(FLinearColor(1.f, .58f, .35f))
                            .Visibility(this, &SFishingPausePanel::SaveErrorVisibility)
                        ]
                    ]
                ]
            ]
        ];
    }

    void Reset()
    {
        Backpack->ResetSelection();
        Pages->SetActiveWidgetIndex(0);
    }

    virtual bool SupportsKeyboardFocus() const override { return true; }

    virtual FReply OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event) override
    {
        if (Event.GetKey() == EKeys::Tab || Event.GetKey() == EKeys::Escape)
        {
            return Event.IsRepeat() ? FReply::Handled() : Close();
        }
        return FReply::Unhandled();
    }

private:
    UHumanFishingSaveSubsystem* SaveSubsystem() const
    {
        return PlayerUI.IsValid() ? PlayerUI->GetSaveSubsystem() : nullptr;
    }

    EVisibility SaveErrorVisibility() const
    {
        const auto* Save = SaveSubsystem();
        return Save && Save->HasSaveError() ? EVisibility::Visible : EVisibility::Collapsed;
    }

    TSharedRef<SWidget> Tab(FText Text, int32 Index)
    {
        return SNew(SButton).Text(Text).ContentPadding(FMargin(16, 6))
            .ButtonColorAndOpacity_Lambda([this, Index]()
            {
                return Pages && Pages->GetActiveWidgetIndex() == Index ? Teal : FLinearColor(.56f, .65f, .66f);
            })
            .OnClicked_Lambda([this, Index]()
            {
                Backpack->ResetSelection();
                Pages->SetActiveWidgetIndex(Index);
                return FReply::Handled();
            });
    }

    FReply Close()
    {
        Reset();
        if (PlayerUI.IsValid())
        {
            PlayerUI->SetBackpackOpen(false);
        }
        return FReply::Handled();
    }

    FReply ReturnToMenu()
    {
        if (auto* Save = SaveSubsystem())
        {
            Save->ReturnToMenu(PlayerUI->GetWorld());
        }
        return FReply::Handled();
    }

    TWeakObjectPtr<UHumanFishingPlayerUI> PlayerUI;
    TSharedPtr<SBackpackPanel> Backpack;
    TSharedPtr<SWidgetSwitcher> Pages;
};

class SHealthReadout : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHealthReadout) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, UHumanFishingPlayerUI* UI)
    {
        const TWeakObjectPtr<UHumanFishingPlayerUI> Weak(UI);
        auto Health = [Weak]()
        {
            const UHumanFishingInventory* Data = Weak.IsValid() ? Weak->GetInventory() : nullptr;
            return Data ? Data->GetHealth() : UHumanFishingInventory::MaxHealth;
        };

        BarStyle.SetBackgroundImage(*WhiteBrush()).SetFillImage(*WhiteBrush());
        BarStyle.BackgroundImage.TintColor = FLinearColor(.12f, .22f, .25f);
        ChildSlot
        [
            SNew(SBox).WidthOverride(208)
            [
                SNew(SBorder).Padding(FMargin(10, 7)).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([Health]() { return FText::Format(LOCTEXT("HealthValue", "HP   {0} / 100"), FMath::CeilToInt(Health())); })
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)).ColorAndOpacity(FLinearColor(.92f, .97f, .98f))
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SBox).HeightOverride(6)
                        [
                            SNew(SProgressBar).Style(&BarStyle)
                            .Percent_Lambda([Health]() { return TOptional<float>(Health() / UHumanFishingInventory::MaxHealth); })
                            .FillColorAndOpacity_Lambda([Health]() { return Health() > 25.f ? Teal : FLinearColor(.96f, .26f, .17f); })
                        ]
                    ]
                ]
            ]
        ];
    }
private:
    FProgressBarStyle BarStyle;
};

class SCaptureToast : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCaptureToast) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, UHumanFishingPlayerUI* UI)
    {
        PlayerUI = UI;
        if (const UHumanFishingCatalog* Catalog = UI->GetCatalog())
        {
            for (const auto& Pair : Catalog->Types)
            {
                if (const UTexture2D* Texture = Pair.Value.Portrait.Get())
                {
                    FSlateBrush& Brush = Portraits.Add(Pair.Key);
                    Brush.SetResourceObject(Pair.Value.Portrait.Get());
                    Brush.SetImageSize(FVector2D(Texture->GetSizeX(), Texture->GetSizeY()));
                }
            }
        }

        ChildSlot
        [
            SNew(SBorder).Padding(FMargin(12, 7)).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 11, 0)
                [
                    SNew(SBox).WidthOverride(64).HeightOverride(64)
                    [
                        SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                        [ SNew(SImage).Image_Lambda([this]() { return Portraits.Find(ActiveType()); }) ]
                    ]
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(STextBlock).Text(LOCTEXT("Captured", "CAPTURED"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11)).ColorAndOpacity(Gold)
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
                    [
                        SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                        .ColorAndOpacity(FLinearColor(.92f, .97f, .98f))
                        .Text_Lambda([this]()
                        {
                            const FHumanFishingCatchType* Type = Lookup(PlayerUI.Get(), ActiveType());
                            return Type ? Type->DisplayName : FText::GetEmpty();
                        })
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
                    [
                        SNew(STextBlock).Text(LOCTEXT("NewGuide", "New Field Guide Entry"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(Teal)
                        .Visibility_Lambda([this]()
                        {
                            return PlayerUI.IsValid() && PlayerUI->IsNewGuideToast() ? EVisibility::Visible
                                : EVisibility::Collapsed;
                        })
                    ]
                ]
            ]
        ];
    }

    FName ActiveType() const { return PlayerUI.IsValid() ? PlayerUI->GetToastType() : NAME_None; }

private:
    TWeakObjectPtr<UHumanFishingPlayerUI> PlayerUI;
    TMap<FName, FSlateBrush> Portraits;
};
}

void SHumanFishingHUD::Construct(const FArguments&, UHumanFishingPlayerUI* InPlayerUI)
{
    const TWeakObjectPtr<UHumanFishingPlayerUI> Weak(InPlayerUI);
    const auto IsFlashBait = [Weak]()
    {
        const auto* Data = Weak.IsValid() ? Weak->GetInventory() : nullptr;
        return Data && Data->GetSelectedBait() == EHumanFishingBait::Flash;
    };
    if (const UHumanFishingCatalog* Catalog = InPlayerUI->GetCatalog())
    {
        DollarBaitPortrait.SetResourceObject(Catalog->DollarBaitPortrait.Get());
        FlashBaitPortrait.SetResourceObject(Catalog->FlashBaitPortrait.Get());
        DollarBaitPortrait.SetImageSize(FVector2D(42.f, 42.f));
        FlashBaitPortrait.SetImageSize(FVector2D(42.f, 42.f));
    }
    const TSharedRef<SCaptureToast> Toast = SNew(SCaptureToast, InPlayerUI);
    const TSharedRef<SFishingPausePanel> Panel = SNew(SFishingPausePanel, InPlayerUI);
    PausePanel = Panel;
    ResetPage = FSimpleDelegate::CreateSP(Panel, &SFishingPausePanel::Reset);
    Panel->SetVisibility(TAttribute<EVisibility>::CreateLambda([Weak]()
    {
        return Weak.IsValid() && Weak->IsBackpackOpen() ? EVisibility::Visible : EVisibility::Collapsed;
    }));
    Toast->SetVisibility(TAttribute<EVisibility>::CreateLambda([Weak]()
    {
        return Weak.IsValid() && !Weak->GetToastType().IsNone() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
    }));

    SetVisibility(EVisibility::SelfHitTestInvisible);
    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24, 136, 0, 0)
        [ SNew(SHealthReadout, InPlayerUI).Visibility(EVisibility::HitTestInvisible) ]
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24, 184, 0, 0)
        [
            SNew(SBorder).Padding(FMargin(6, 3, 11, 3)).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink)
            .Visibility_Lambda([Weak]()
            {
                return Weak.IsValid() && !Weak->IsDeepOcean() && !Weak->IsBackpackOpen()
                    ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
            })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
                [
                    SNew(SImage).Image_Lambda([this, IsFlashBait]() -> const FSlateBrush*
                    {
                        return IsFlashBait() ? &FlashBaitPortrait : &DollarBaitPortrait;
                    })
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
                    .Text_Lambda([IsFlashBait]()
                    {
                        return IsFlashBait() ? LOCTEXT("FlashBait", "Flash Bait  ·  B Switch")
                            : LOCTEXT("DollarBait", "Dollar Bait  ·  B Switch");
                    })
                ]
            ]
        ]
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 20, 20, 0)
        [ Toast ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, 20, 0, 0)
        [
            SNew(STextBlock).Text(LOCTEXT("AutosaveWarning", "Autosave failed. Open Tab to retry."))
            .ColorAndOpacity(FLinearColor(1.f, .60f, .37f))
            .Visibility_Lambda([Weak]()
            {
                const auto* Save = Weak.IsValid() ? Weak->GetSaveSubsystem() : nullptr;
                return Save && Save->HasSaveError() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
            })
        ]
        + SOverlay::Slot()
        [ Panel ]
    ];
}

#undef LOCTEXT_NAMESPACE
