#include "HumanFishingSaveMenu.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HumanFishingGuide.h"
#include "HumanFishingSave.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "HumanFishingSaveMenu"

namespace
{
const FLinearColor MenuCream(.95f, .89f, .75f);
const FLinearColor MenuInk(.022f, .07f, .075f);
const FLinearColor Sea(.28f, .73f, .70f);

class SSaveSlots : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSaveSlots) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, UWorld* InWorld, const UHumanFishingCatalog* Catalog)
    {
        World = InWorld;
        auto* Game = InWorld->GetGameInstance();
        Save = Game ? Game->GetSubsystem<UHumanFishingSaveSubsystem>() : nullptr;
        TypeCount = Catalog ? Catalog->Types.Num() : 0;
        SetGuideBrush(Cover, Catalog ? Catalog->GuideCover.Get() : nullptr, FLinearColor(.03f, .12f, .15f));
        SetGuideBrush(Paper, Catalog ? Catalog->GuidePaper.Get() : nullptr, FLinearColor(.88f, .78f, .61f));

        auto Slots = SNew(SHorizontalBox);
        for (int32 Slot = 0; Slot < UHumanFishingSaveSubsystem::SlotCount; ++Slot)
        {
            Summaries.Add(Save.IsValid() ? Save->GetSlotSummary(Slot, Catalog) : FHumanFishingSlotSummary{});
            Slots->AddSlot().FillWidth(1).Padding(6, 0)[MakeSlot(Slot)];
        }

        ChildSlot
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(.007f, .022f, .030f))
            .HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(14)
            [
                SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
                [
                    SNew(SBox).WidthOverride(916)
                    [
                        SNew(SBorder).BorderImage(&Cover).Padding(32)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(6, 0, 0, 3)
                            [ Label(LOCTEXT("Title", "HUMAN FISHING"), 34, MenuCream, true) ]
                            + SVerticalBox::Slot().AutoHeight().Padding(6, 0, 0, 24)
                            [ Label(LOCTEXT("Subtitle", "Choose a save to continue your expedition."), 15, MenuCream) ]
                            + SVerticalBox::Slot().AutoHeight()
                            [ Slots ]
                            + SVerticalBox::Slot().AutoHeight().Padding(6, 16, 6, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock).Text_Lambda([this]() { return Status; }).AutoWrapText(true)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).ColorAndOpacity(MenuCream)
                                ]
                                + SHorizontalBox::Slot().AutoWidth()
                                [
                                    SNew(SButton).Text(LOCTEXT("Quit", "Quit")).ContentPadding(FMargin(16, 6))
                                    .OnClicked_Lambda([this]()
                                    {
                                        if (World.IsValid())
                                        {
                                            UKismetSystemLibrary::QuitGame(World.Get(), World->GetFirstPlayerController(),
                                                EQuitPreference::Quit, false);
                                        }
                                        return FReply::Handled();
                                    })
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ];
    }

    virtual bool SupportsKeyboardFocus() const override { return true; }

private:
    static TSharedRef<STextBlock> Label(FText Text, int32 Size, FLinearColor Color, bool bBold = false)
    {
        return SNew(STextBlock).Text(Text).ColorAndOpacity(Color)
            .Font(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size)).AutoWrapText(true);
    }

    TSharedRef<SWidget> MakeSlot(int32 Slot)
    {
        const auto& Summary = Summaries[Slot];
        FText Area = LOCTEXT("Shore", "Shore");
        if (Summary.Level == UHumanFishingSaveSubsystem::OceanLevel)
        {
            Area = LOCTEXT("Ocean", "Deep Ocean");
        }
        else if (Summary.Level == UHumanFishingSaveSubsystem::CoveLevel)
        {
            Area = LOCTEXT("Cove", "Scenic Cove");
        }
        const FText Description = Summary.bReadable
            ? FText::Format(LOCTEXT("Stats", "{0}\n{1} captures\n{2}/{3} discovered\nSaved {4}"),
                Area, Summary.Captures, Summary.Discovered, TypeCount,
                FText::AsDateTime(Summary.SavedAt, EDateTimeStyle::Short, EDateTimeStyle::Short))
            : (Summary.bExists ? LOCTEXT("Corrupt", "No readable save in this slot.") : LOCTEXT("Empty", "Empty slot"));
        return SNew(SBorder).BorderImage(&Paper).Padding(FMargin(16, 14))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                [ Label(FText::Format(LOCTEXT("Slot", "SLOT {0}"), Slot + 1), 18, MenuInk, true) ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBox).HeightOverride(108)
                    [ Label(Description, 14, MenuInk) ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
                [
                    SNew(SButton).Text(LOCTEXT("Continue", "Continue")).ContentPadding(FMargin(10, 5))
                    .ButtonColorAndOpacity(Sea).Visibility(Summary.bReadable ? EVisibility::Visible : EVisibility::Hidden)
                    .OnClicked_Lambda([this, Slot]() { return Launch(Slot, false); })
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
                [
                    SNew(SButton).ContentPadding(FMargin(10, 5))
                    .Text_Lambda([this, Slot]()
                    {
                        return PendingOverwrite == Slot ? LOCTEXT("Overwrite", "Confirm Overwrite")
                            : LOCTEXT("New", "New Game");
                    })
                    .OnClicked_Lambda([this, Slot]()
                    {
                        if (Summaries[Slot].bExists && PendingOverwrite != Slot)
                        {
                            PendingOverwrite = Slot;
                            Status = FText::Format(LOCTEXT("Confirm", "Start over in slot {0}? Its previous progress will be replaced."),
                                Slot + 1);
                            return FReply::Handled();
                        }
                        return Launch(Slot, true);
                    })
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SButton).Text(LOCTEXT("Cancel", "Cancel")).ContentPadding(FMargin(10, 5))
                    .Visibility_Lambda([this, Slot]() { return PendingOverwrite == Slot ? EVisibility::Visible : EVisibility::Hidden; })
                    .OnClicked_Lambda([this]()
                    {
                        PendingOverwrite = INDEX_NONE;
                        Status = FText::GetEmpty();
                        return FReply::Handled();
                    })
                ]
            ];
    }

    FReply Launch(int32 Slot, bool bNew)
    {
        if (World.IsValid() && Save.IsValid() && Save->StartSlot(World.Get(), Slot, bNew))
        {
            SetEnabled(false);
            if (APlayerController* Player = World->GetFirstPlayerController())
            {
                // The viewport survives travel, including its UI-only input setting.
                FInputModeGameOnly Mode;
                Mode.SetConsumeCaptureMouseDown(false);
                Player->SetInputMode(Mode);
                Player->SetShowMouseCursor(false);
                Player->FlushPressedKeys();
            }
        }
        else
        {
            Status = LOCTEXT("SaveFailed", "The slot could not be opened. Check storage and try again.");
        }
        return FReply::Handled();
    }

    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<UHumanFishingSaveSubsystem> Save;
    TArray<FHumanFishingSlotSummary> Summaries;
    FSlateBrush Cover;
    FSlateBrush Paper;
    FText Status;
    int32 TypeCount = 0;
    int32 PendingOverwrite = INDEX_NONE;
};
}

AHumanFishingSaveMenu::AHumanFishingSaveMenu()
{
    DefaultPawnClass = nullptr;
}

void AHumanFishingSaveMenu::StartPlay()
{
    Super::StartPlay();
    UGameInstance* Game = GetGameInstance();
    UGameViewportClient* Viewport = Game ? Game->GetGameViewportClient() : nullptr;
    APlayerController* Player = GetWorld()->GetFirstPlayerController();
    if (!Viewport || !Player || !Player->IsLocalController())
    {
        return;
    }

    Catalog = LoadObject<UHumanFishingCatalog>(nullptr,
        TEXT("/Game/HumanFishing/UI/Backpack/DA_HumanFishingCatalog.DA_HumanFishingCatalog"));
    Menu = SNew(SSaveSlots, GetWorld(), Catalog.Get());
    Viewport->AddViewportWidgetContent(Menu.ToSharedRef(), 100);
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Menu);
    Player->SetInputMode(Mode);
    Player->SetShowMouseCursor(true);
}

void AHumanFishingSaveMenu::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UGameInstance* Game = GetGameInstance())
    {
        if (UGameViewportClient* Viewport = Game->GetGameViewportClient(); Viewport && Menu)
        {
            Viewport->RemoveViewportWidgetContent(Menu.ToSharedRef());
        }
    }
    Menu.Reset();
    Super::EndPlay(Reason);
}

#undef LOCTEXT_NAMESPACE
