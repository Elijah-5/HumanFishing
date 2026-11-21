#include "HumanFishingInventory.h"
#include "Engine/GameInstance.h"
#include "HumanFishingSave.h"

namespace
{
bool CanPlaceCatch(const TArray<FHumanFishingCatch>& Catches, FIntPoint Cell, FIntPoint Size, int32 IgnoreId, int32 Columns)
{
    if (Size.X < 1 || Size.X > Columns || Size.Y < 1 || Size.Y > UHumanFishingInventory::Rows ||
        Cell.X < 0 || Cell.X > Columns - Size.X ||
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

int32 FHumanFishingProgress::GetColumns() const
{
    return bHasBackpackUpgrade ? UHumanFishingInventory::UpgradedColumns : UHumanFishingInventory::Columns;
}

bool FHumanFishingProgress::IsValid() const
{
    if (NextId < 0 || Seaweed < 0 || FlashBaitCount < 0 || HundredDollarBillCount < 0 || JellyshockCount < 0 || VioletJellyshockCount < 0 || !FMath::IsFinite(Health) || Health < 0 || Health > UHumanFishingInventory::MaxHealth ||
        CaughtTypes.Contains(NAME_None) || CaughtTypes.Num() > NextId ||
        Catches.Num() > UHumanFishingInventory::Rows * GetColumns())
    {
        return false;
    }
    TSet<int32> Ids;
    // Check bounds before the overlap test uses any of the saved coordinates.
    const TArray<FHumanFishingCatch> Empty;
    for (const FHumanFishingCatch& Item : Catches)
    {
        if (Item.Id < 0 || Item.Id >= NextId || Ids.Contains(Item.Id) || !CaughtTypes.Contains(Item.Type) ||
            !CanPlaceCatch(Empty, Item.Cell, Item.Size, INDEX_NONE, GetColumns()))
        {
            return false;
        }
        Ids.Add(Item.Id);
    }
    return !Catches.ContainsByPredicate([this](const FHumanFishingCatch& Item)
    {
        return !CanPlaceCatch(Catches, Item.Cell, Item.Size, Item.Id, GetColumns());
    });
}

FHumanFishingBaitDefinition UHumanFishingCatalog::GetBaitDefinition(EHumanFishingBait Bait) const
{
    switch (Bait)
    {
    case EHumanFishingBait::Flash:
        return {NSLOCTEXT("HumanFishingHUD", "FlashBaitName", "Flash Bait"),
            FlashBaitMesh ? FlashBaitMesh.Get() : DollarBaitMesh.Get(), FlashBaitPortrait.Get(), 2., 4., true};
    case EHumanFishingBait::HundredDollarBill:
        return {NSLOCTEXT("HumanFishingHUD", "HundredDollarBillName", "$100 Bill"),
            HundredDollarBillMesh ? HundredDollarBillMesh.Get() : DollarBaitMesh.Get(), HundredDollarBillPortrait.Get()};
    default:
        return {NSLOCTEXT("HumanFishingHUD", "DollarBaitName", "Dollar Bait"),
            DollarBaitMesh.Get(), DollarBaitPortrait.Get()};
    }
}

const FHumanFishingRarityRules& UHumanFishingCatalog::GetRarityRules(EHumanFishingRarity Rarity)
{
    static const FHumanFishingRarityRules Rules[] = {
        {52, {45, 60}, 1, FLinearColor(.39f, .44f, .49f)},
        {21, {36, 48}, 2, FLinearColor(.08f, .34f, .18f)},
        {12, {28, 38}, 3, FLinearColor(.08f, .26f, .53f)},
        {8, {22, 30}, 5, FLinearColor(.30f, .14f, .45f)},
        {5, {14, 22}, 8, FLinearColor(.55f, .35f, .07f)},
        {0, {9, 15}, 13, FLinearColor(.51f, .11f, .13f)},
    };
    static_assert(UE_ARRAY_COUNT(Rules) == static_cast<int32>(EHumanFishingRarity::Count));
    const int32 Index = static_cast<int32>(Rarity);
    return Rules[Index < UE_ARRAY_COUNT(Rules) ? Index : 0];
}

int32 UHumanFishingCatalog::GetCatchSeaweedValue(const FHumanFishingCatch& Item) const
{
    const FHumanFishingCatchType* Profile = Types.Find(Item.Type);
    if (!Profile || Item.Size.X < 1 || Item.Size.Y < 1 ||
        Item.Size.X > UHumanFishingInventory::UpgradedColumns || Item.Size.Y > UHumanFishingInventory::Rows)
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
    return CanPlaceCatch(Progress.Catches, Cell, Size, IgnoreId, GetColumns());
}

bool UHumanFishingInventory::TryAddCatch(FName Type, FIntPoint Size)
{
    if (Type.IsNone() || Size.X < 1 || Size.X > GetColumns() || Size.Y < 1 || Size.Y > Rows ||
        Progress.NextId == MAX_int32)
    {
        return false;
    }

    for (int32 Row = 0; Row <= Rows - Size.Y; ++Row)
    {
        for (int32 Column = 0; Column <= GetColumns() - Size.X; ++Column)
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

int32 UHumanFishingInventory::GetRedemptionValue(const UHumanFishingCatalog* Catalog) const
{
    if (!Catalog || Progress.Catches.IsEmpty())
    {
        return 0;
    }
    int32 Total = 0;
    for (const FHumanFishingCatch& Catch : Progress.Catches)
    {
        const int32 Value = Catalog->GetCatchSeaweedValue(Catch);
        if (Value <= 0)
        {
            return 0;
        }
        Total += Value;
    }
    return Total;
}

float UHumanFishingInventory::GetLoadFraction() const
{
    int32 Used = 0;
    for (const FHumanFishingCatch& Catch : Progress.Catches)
    {
        Used += Catch.Size.X * Catch.Size.Y;
    }
    return FMath::Clamp(static_cast<float>(Used) / (GetColumns() * Rows), 0.f, 1.f);
}

bool UHumanFishingInventory::StealMostValuableCatch(const UHumanFishingCatalog* Catalog, FHumanFishingCatch& OutStolen)
{
    int32 Best = INDEX_NONE;
    int32 BestScore = -1;
    for (int32 Index = 0; Index < Progress.Catches.Num(); ++Index)
    {
        const FHumanFishingCatch& Catch = Progress.Catches[Index];
        const int32 Score = Catalog ? Catalog->GetCatchSeaweedValue(Catch) : Catch.Size.X * Catch.Size.Y;
        if (Score > BestScore)
        {
            Best = Index;
            BestScore = Score;
        }
    }
    if (Best == INDEX_NONE)
    {
        return false;
    }
    OutStolen = Progress.Catches[Best];
    Progress.Catches.RemoveAt(Best);
    NotifyChange();
    return true;
}

int32 UHumanFishingInventory::RedeemCatches(const UHumanFishingCatalog* Catalog)
{
    const int32 Value = GetRedemptionValue(Catalog);
    if (Value <= 0 || Progress.Seaweed > MAX_int32 - Value) return 0;
    Progress.Catches.Reset();
    Progress.Seaweed += Value;
    NotifyChange();
    return Value;
}

int32 UHumanFishingInventory::RedeemCatch(const UHumanFishingCatalog* Catalog,int32 CatchId)
{
    const auto* Catch=FindCatch(CatchId);
    const int32 Value=Catch && Catalog?Catalog->GetCatchSeaweedValue(*Catch):0;
    if(Value<=0 || Progress.Seaweed>MAX_int32-Value)return 0;
    Progress.Catches.RemoveAll([CatchId](const FHumanFishingCatch& C){return C.Id==CatchId;});
    Progress.Seaweed+=Value;NotifyChange();return Value;
}

void UHumanFishingInventory::ToggleBait()
{
    if (bBaitReserved) return;
    switch (SelectedBait)
    {
    case EHumanFishingBait::Dollar: SelectedBait = EHumanFishingBait::HundredDollarBill; break;
    case EHumanFishingBait::HundredDollarBill: SelectedBait = EHumanFishingBait::Flash; break;
    default: SelectedBait = EHumanFishingBait::Dollar; break;
    }
}

int32 UHumanFishingInventory::GetBaitPrice(EHumanFishingBait Bait)
{
    return Bait == EHumanFishingBait::Flash ? FlashBaitCost :
        Bait == EHumanFishingBait::HundredDollarBill ? HundredDollarBillCost : 0;
}

int32 UHumanFishingInventory::GetBaitCount(EHumanFishingBait Bait) const
{
    if (Bait == EHumanFishingBait::Dollar) return INDEX_NONE; // Unlimited.
    const int32 Count = Bait == EHumanFishingBait::Flash ? Progress.FlashBaitCount :
        Bait == EHumanFishingBait::HundredDollarBill ? Progress.HundredDollarBillCount : 0;
    return FMath::Max(0, Count - (bBaitReserved && !bBaitCommitted && ReservedBait == Bait ? 1 : 0));
}

bool UHumanFishingInventory::CanCastSelectedBait() const
{
    return !bBaitReserved && (SelectedBait == EHumanFishingBait::Dollar || GetBaitCount(SelectedBait) > 0);
}

bool UHumanFishingInventory::BuyBait(EHumanFishingBait Bait, int32 Quantity)
{
    const int32 Price = GetBaitPrice(Bait);
    if (Price <= 0 || Quantity <= 0 || Quantity > Progress.Seaweed / Price) return false;
    int32& Count = Bait == EHumanFishingBait::Flash ? Progress.FlashBaitCount : Progress.HundredDollarBillCount;
    if (Count > MAX_int32 - Quantity) return false;
    Progress.Seaweed -= Price * Quantity;
    Count += Quantity;
    NotifyChange();
    return true;
}

bool UHumanFishingInventory::ReserveSelectedBait()
{
    if (!CanCastSelectedBait()) return false;
    ReservedBait = SelectedBait;
    bBaitReserved = true;
    bBaitCommitted = false;
    return true;
}

void UHumanFishingInventory::CommitHookedBait()
{
    if (!bBaitReserved || bBaitCommitted) return;
    bBaitCommitted = true;
    if (ReservedBait == EHumanFishingBait::Dollar) return;
    int32& Count = ReservedBait == EHumanFishingBait::Flash ? Progress.FlashBaitCount : Progress.HundredDollarBillCount;
    Count = FMath::Max(0, Count - 1);
    NotifyChange();
}

void UHumanFishingInventory::FinishBaitUse(bool bRetrieveBeforeHook)
{
    if (!bBaitReserved) return;
    if (!bRetrieveBeforeHook) CommitHookedBait();
    bBaitReserved = bBaitCommitted = false;
}

bool UHumanFishingInventory::BuyCoveKey()
{
    return BuyUpgrade(CoveKeyCost, Progress.bHasCoveKey);
}

bool UHumanFishingInventory::BuyBackpackUpgrade()
{
    return BuyUpgrade(BackpackUpgradeCost, Progress.bHasBackpackUpgrade);
}

bool UHumanFishingInventory::BuyJellyshock(int32 Quantity)
{
    if (Quantity <= 0 || Quantity > Progress.Seaweed / JellyshockCost ||
        Progress.JellyshockCount > MAX_int32 - Quantity) return false;
    Progress.Seaweed -= Quantity * JellyshockCost;
    Progress.JellyshockCount += Quantity;
    NotifyChange();
    return true;
}

bool UHumanFishingInventory::ConsumeJellyshock()
{
    if (Progress.JellyshockCount <= 0) return false;
    --Progress.JellyshockCount;
    NotifyChange();
    return true;
}

bool UHumanFishingInventory::BuyVioletJellyshock(int32 Quantity)
{
    if(Quantity<=0 || Quantity>Progress.Seaweed/VioletJellyshockCost || Progress.VioletJellyshockCount>MAX_int32-Quantity)return false;
    Progress.Seaweed-=Quantity*VioletJellyshockCost;Progress.VioletJellyshockCount+=Quantity;NotifyChange();return true;
}
bool UHumanFishingInventory::ConsumeVioletJellyshock()
{
    if(Progress.VioletJellyshockCount<=0)return false;
    --Progress.VioletJellyshockCount;NotifyChange();return true;
}

bool UHumanFishingInventory::BuyTidalFins()
{
    return BuyUpgrade(TidalFinsCost, Progress.bHasTidalFins);
}

bool UHumanFishingInventory::BuyMudskipper()
{
    return BuyUpgrade(MudskipperCost, Progress.bHasMudskipper);
}

bool UHumanFishingInventory::BuyBraidedLine()
{
    return BuyUpgrade(BraidedLineCost, Progress.bHasBraidedLine);
}

bool UHumanFishingInventory::BuyHundredDollarBill()
{
    return BuyBait(EHumanFishingBait::HundredDollarBill, 1);
}

bool UHumanFishingInventory::BuyUpgrade(int32 Cost, bool& bOwned)
{
    if (bOwned || Progress.Seaweed < Cost)
    {
        return false;
    }
    Progress.Seaweed -= Cost;
    bOwned = true;
    NotifyChange();
    return true;
}
