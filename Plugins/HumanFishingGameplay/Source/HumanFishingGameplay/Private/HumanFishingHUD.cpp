#include "HumanFishingHUD.h"

#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "Input/DragAndDrop.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
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
    int32 Id = INDEX_NONE;
    FVector2D Offset;
};

class SBackpackGrid : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SBackpackGrid) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, UHumanFishingPlayerUI* UI)
    {
        PlayerUI = UI;
        if (const UHumanFishingCatalog* Catalog = UI->GetCatalog())
        {
            for (const auto& Pair : Catalog->Types)
            {
                Portraits.Add(Pair.Key).SetResourceObject(Pair.Value.Portrait.Get());
            }
        }
    }

    void ClearPreview()
    {
        PreviewId = INDEX_NONE;
        PressedId = INDEX_NONE;
        Invalidate(EInvalidateWidgetReason::Paint);
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
            Draw(1, Origin + FVector2D(3), Size - FVector2D(6), WhiteBrush(), FLinearColor(.11f, .26f, .30f, Opacity));
            if (const FSlateBrush* Portrait = Portraits.Find(Item.Type))
            {
                const float Height = FMath::Min(Size.Y - 8, (Size.X - 8) * 2);
                const FVector2D ImageSize(Height * .5, Height);
                Draw(2, Origin + (Size - ImageSize) * .5, ImageSize, Portrait, FLinearColor(1, 1, 1, Opacity));
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
                        Draw(3, FVector2D(Cell.X * At.X, Cell.Y * At.Y) + FVector2D(2),
                            Cell - FVector2D(4), WhiteBrush(), Color);
                    }
                }
            }
        }
        return Layer + 3;
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
                    PickupOffset = Cursor - FVector2D(Cell.X * Item.Cell.X, Cell.Y * Item.Cell.Y);
                    return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
                }
            }
        }
        return FReply::Handled();
    }

    virtual FReply OnDragDetected(const FGeometry&, const FPointerEvent&) override
    {
        if (PressedId == INDEX_NONE)
        {
            return FReply::Unhandled();
        }
        TSharedRef<FCatchDrag> Drag = MakeShared<FCatchDrag>();
        Drag->Id = PressedId;
        Drag->Offset = PickupOffset;
        Drag->Construct();
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
                                [ SAssignNew(Grid, SBackpackGrid, UI) ]
                            ]
                            + SOverlay::Slot()
                            [ SNew(SImage).Image(&Frame).Visibility(EVisibility::HitTestInvisible) ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
                    [
                        SNew(STextBlock).Text(LOCTEXT("BackpackKeys", "Drag to rearrange     •     Tab / Esc to close"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
                        .ColorAndOpacity(FLinearColor(.72f, .84f, .87f))
                    ]
                ]
            ]
        ];
    }

    virtual bool SupportsKeyboardFocus() const override { return true; }

    virtual FReply OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event) override
    {
        if (Event.GetKey() == EKeys::Tab || Event.GetKey() == EKeys::Escape)
        {
            if (!Event.IsRepeat() && PlayerUI.IsValid())
            {
                FSlateApplication::Get().CancelDragDrop();
                Grid->ClearPreview();
                PlayerUI->SetBackpackOpen(false);
            }
            return FReply::Handled();
        }
        return FReply::Unhandled();
    }

private:
    TWeakObjectPtr<UHumanFishingPlayerUI> PlayerUI;
    TSharedPtr<SBackpackGrid> Grid;
    FSlateBrush Frame;
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
                Portraits.Add(Pair.Key).SetResourceObject(Pair.Value.Portrait.Get());
            }
        }

        ChildSlot
        [
            SNew(SBorder).Padding(FMargin(12, 7)).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 11, 0)
                [
                    SNew(SBox).WidthOverride(32).HeightOverride(64)
                    [ SNew(SImage).Image_Lambda([this]() { return Portraits.Find(ActiveType()); }) ]
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
    const TSharedRef<SCaptureToast> Toast = SNew(SCaptureToast, InPlayerUI);
    const TSharedRef<SBackpackPanel> Panel = SNew(SBackpackPanel, InPlayerUI);
    Backpack = Panel;
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
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 20, 20, 0)
        [ Toast ]
        + SOverlay::Slot()
        [ Panel ]
    ];
}

#undef LOCTEXT_NAMESPACE
