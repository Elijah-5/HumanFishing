#include "HumanFishingInventory.h"
#include "Engine/GameInstance.h"
#include "HumanFishingSave.h"

namespace
{
bool CanPlaceCatch(const TArray<FHumanFishingCatch>& Catches, FIntPoint Cell, FIntPoint Size, int32 IgnoreId)
{
    if (Size.X < 1 || Size.X > UHumanFishingInventory::Columns || Size.Y < 1 || Size.Y > UHumanFishingInventory::Rows ||
        Cell.X < 0 || Cell.X > UHumanFishingInventory::Columns - Size.X ||
        Cell.Y < 0 || Cell.Y > UHumanFishingInventory::Rows - Size.Y)
    {
        return false;
    }

    return !Catches.ContainsByPredicate([=](const FHumanFishingCatch& Other)
    {
        return Other.Id != IgnoreId &&
            Cell.X < Other.Cell.X + Other.Size.X && Other.Cell.X < Cell.X + Size.X &&
            Cell.Y < Other.Cell.Y + Other.Size.Y && Other.Cell.Y < Cell.Y + Size.Y;
    });
}
}

bool FHumanFishingProgress::IsValid() const
{
    if (NextId < 0 || Seaweed < 0 || !FMath::IsFinite(Health) || Health < 0 || Health > UHumanFishingInventory::MaxHealth ||
        CaughtTypes.Contains(NAME_None) || CaughtTypes.Num() > NextId ||
        Catches.Num() > UHumanFishingInventory::Rows * UHumanFishingInventory::Columns)
    {
        return false;
    }
    TSet<int32> Ids;
    // Check bounds before the overlap test uses any of the saved coordinates.
    const TArray<FHumanFishingCatch> Empty;
    for (const FHumanFishingCatch& Item : Catches)
    {
        if (Item.Id < 0 || Item.Id >= NextId || Ids.Contains(Item.Id) || !CaughtTypes.Contains(Item.Type) ||
            !CanPlaceCatch(Empty, Item.Cell, Item.Size, INDEX_NONE))
        {
            return false;
        }
        Ids.Add(Item.Id);
    }
    return !Catches.ContainsByPredicate([this](const FHumanFishingCatch& Item)
    {
        return !CanPlaceCatch(Catches, Item.Cell, Item.Size, Item.Id);
    });
}

const FHumanFishingRarityRules& UHumanFishingCatalog::GetRarityRules(EHumanFishingRarity Rarity)
{
    static const FHumanFishingRarityRules Rules[] = {
        {52, {45, 60}, 1, FLinearColor(.39f, .44f, .49f)},
        {21, {36, 48}, 2, FLinearColor(.08f, .34f, .18f)},
        {12, {28, 38}, 3, FLinearColor(.08f, .26f, .53f)},
        {8, {22, 30}, 5, FLinearColor(.30f, .14f, .45f)},
        {5, {14, 22}, 8, FLinearColor(.55f, .35f, .07f)},
        {2, {9, 15}, 13, FLinearColor(.51f, .11f, .13f)},
    };
    static_assert(UE_ARRAY_COUNT(Rules) == static_cast<int32>(EHumanFishingRarity::Count));
    const int32 Index = static_cast<int32>(Rarity);
    return Rules[Index < UE_ARRAY_COUNT(Rules) ? Index : 0];
}

int32 UHumanFishingCatalog::GetCatchSeaweedValue(const FHumanFishingCatch& Item) const
{
    const FHumanFishingCatchType* Profile = Types.Find(Item.Type);
    if (!Profile || Item.Size.X < 1 || Item.Size.Y < 1 ||
        Item.Size.X > UHumanFishingInventory::Columns || Item.Size.Y > UHumanFishingInventory::Rows)
    {
        return 0;
    }
    return Item.Size.X * Item.Size.Y * GetRarityRules(Profile->Rarity).SeaweedPerCell;
}

const FHumanFishingCatch* UHumanFishingInventory::FindCatch(int32 Id) const
{
    return Progress.Catches.FindByPredicate([Id](const FHumanFishingCatch& Catch) { return Catch.Id == Id; });
}

bool UHumanFishingInventory::CanPlace(FIntPoint Cell, FIntPoint Size, int32 IgnoreId) const
{
    return CanPlaceCatch(Progress.Catches, Cell, Size, IgnoreId);
}

bool UHumanFishingInventory::TryAddCatch(FName Type, FIntPoint Size)
{
    if (Type.IsNone() || Size.X < 1 || Size.X > Columns || Size.Y < 1 || Size.Y > Rows ||
        Progress.NextId == MAX_int32)
    {
        return false;
    }

    for (int32 Row = 0; Row <= Rows - Size.Y; ++Row)
    {
        for (int32 Column = 0; Column <= Columns - Size.X; ++Column)
        {
            const FIntPoint Cell(Column, Row);
            if (CanPlace(Cell, Size))
            {
                FHumanFishingCatch& Catch = Progress.Catches.AddDefaulted_GetRef();
                Catch.Id = Progress.NextId++;
                Catch.Type = Type;
                Catch.Cell = Cell;
                Catch.Size = Size;
                Progress.CaughtTypes.Add(Type);
                NotifyChange();
                return true;
            }
        }
    }
    return false;
}

bool UHumanFishingInventory::TryMoveCatch(int32 Id, FIntPoint Cell)
{
    FHumanFishingCatch* Catch = Progress.Catches.FindByPredicate(
        [Id](const FHumanFishingCatch& Entry) { return Entry.Id == Id; });
    if (!Catch || !CanPlace(Cell, Catch->Size, Id))
    {
        return false;
    }
    if (Catch->Cell != Cell)
    {
        Catch->Cell = Cell;
        NotifyChange();
    }
    return true;
}

bool UHumanFishingInventory::TryDiscardCatch(int32 Id)
{
    const bool bRemoved = Progress.Catches.RemoveAll([Id](const FHumanFishingCatch& Item) { return Item.Id == Id; }) > 0;
    if (bRemoved)
    {
        NotifyChange();
    }
    return bRemoved;
}

void UHumanFishingInventory::ChangeHealth(float Amount)
{
    const float Updated = FMath::Clamp(Progress.Health + Amount, 0.f, MaxHealth);
    if (Updated != Progress.Health)
    {
        Progress.Health = Updated;
        NotifyChange(false);
    }
}

void UHumanFishingInventory::SetSeaweed(int32 Amount)
{
    Amount = FMath::Max(Amount, 0);
    if (Progress.Seaweed != Amount)
    {
        Progress.Seaweed = Amount;
        NotifyChange();
    }
}

void UHumanFishingInventory::NotifyChange(bool bImmediate)
{
    if (auto* Save = GetGameInstance()->GetSubsystem<UHumanFishingSaveSubsystem>())
    {
        Save->ProgressChanged(bImmediate);
    }
}
