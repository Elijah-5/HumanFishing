#include "HumanFishingInventory.h"

const FHumanFishingCatch* UHumanFishingInventory::FindCatch(int32 Id) const
{
    return Catches.FindByPredicate([Id](const FHumanFishingCatch& Catch) { return Catch.Id == Id; });
}

bool UHumanFishingInventory::CanPlace(FIntPoint Cell, FIntPoint Size, int32 IgnoreId) const
{
    if (Size.X < 1 || Size.X > Columns || Size.Y < 1 || Size.Y > Rows ||
        Cell.X < 0 || Cell.X > Columns - Size.X || Cell.Y < 0 || Cell.Y > Rows - Size.Y)
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

bool UHumanFishingInventory::TryAddCatch(FName Type, FIntPoint Size)
{
    if (Type.IsNone() || Size.X < 1 || Size.X > Columns || Size.Y < 1 || Size.Y > Rows)
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
                FHumanFishingCatch& Catch = Catches.AddDefaulted_GetRef();
                Catch.Id = NextId++;
                Catch.Type = Type;
                Catch.Cell = Cell;
                Catch.Size = Size;
                return true;
            }
        }
    }
    return false;
}

bool UHumanFishingInventory::TryMoveCatch(int32 Id, FIntPoint Cell)
{
    FHumanFishingCatch* Catch = Catches.FindByPredicate(
        [Id](const FHumanFishingCatch& Entry) { return Entry.Id == Id; });
    if (!Catch || !CanPlace(Cell, Catch->Size, Id))
    {
        return false;
    }
    Catch->Cell = Cell;
    return true;
}

void UHumanFishingInventory::ChangeHealth(float Amount)
{
    Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
}
