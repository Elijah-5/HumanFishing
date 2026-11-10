#include "HumanFishingSave.h"

#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogHumanFishingSave, Log, All);

const FName UHumanFishingSaveSubsystem::ShoreLevel(TEXT("L_ShorePrototype"));
const FName UHumanFishingSaveSubsystem::CoveLevel(TEXT("L_ScenicCove"));
const FName UHumanFishingSaveSubsystem::OceanLevel(TEXT("L_DeepOcean"));
const FName UHumanFishingSaveSubsystem::MenuLevel(TEXT("L_SaveMenu"));

namespace
{
constexpr int32 SaveVersion = 1;
constexpr int32 ChecksumSize = sizeof(uint32);
constexpr double HealthSaveInterval = 5.;

bool IsBeachLevel(FName Level)
{
    return Level == UHumanFishingSaveSubsystem::ShoreLevel || Level == UHumanFishingSaveSubsystem::CoveLevel;
}

bool IsPlayableLevel(FName Level)
{
    return IsBeachLevel(Level) || Level == UHumanFishingSaveSubsystem::OceanLevel;
}

FString SlotName(int32 Slot, int32 Generation)
{
    return FString::Printf(TEXT("HumanFishing_%d_%c"), Slot + 1, Generation == 0 ? 'A' : 'B');
}

UHumanFishingSaveGame* ReadSlot(const FString& Name)
{
    TArray<uint8> Bytes;
    if (!UGameplayStatics::LoadDataFromSlot(Bytes, Name, 0) || Bytes.Num() <= ChecksumSize)
    {
        return nullptr;
    }
    // Validate the entire snapshot before giving it to Unreal's object deserializer.
    const int32 PayloadSize = Bytes.Num() - ChecksumSize;
    uint32 Expected = 0;
    FMemoryReader Reader(Bytes, true);
    Reader.Seek(PayloadSize);
    Reader << Expected;
    if (Expected != FCrc::MemCrc32(Bytes.GetData(), PayloadSize))
    {
        return nullptr;
    }
    Bytes.SetNum(PayloadSize, false);
    return Cast<UHumanFishingSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
}

void OpenLevel(UWorld* World, FName Level)
{
    UGameplayStatics::SetGamePaused(World, false);
    const FString Path = TEXT("/Game/HumanFishing/Maps/") + Level.ToString();
    UGameplayStatics::OpenLevel(World, FName(*Path));
}
}

void UHumanFishingSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Inventory = Collection.InitializeDependency<UHumanFishingInventory>();
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UHumanFishingSaveSubsystem::Tick), 1.f);
    ExitHandle = FCoreDelegates::OnPreExit.AddUObject(this, &UHumanFishingSaveSubsystem::HandleExit);
}

void UHumanFishingSaveSubsystem::Deinitialize()
{
    FCoreDelegates::OnPreExit.Remove(ExitHandle);
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    Flush();
    Super::Deinitialize();
}

UHumanFishingSaveGame* UHumanFishingSaveSubsystem::ReadLatest(int32 Slot, int32& Generation, bool& bExists) const
{
    Generation = INDEX_NONE;
    bExists = false;
    if (Slot < 0 || Slot >= SlotCount)
    {
        return nullptr;
    }
    UHumanFishingSaveGame* Latest = nullptr;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FString Name = SlotName(Slot, Index);
        if (!UGameplayStatics::DoesSaveGameExist(Name, 0))
        {
            continue;
        }
        bExists = true;
        auto* Save = ReadSlot(Name);
        if (Save && Save->Version == SaveVersion && Save->Revision > 0 && Save->SavedAt.GetTicks() > 0 &&
            IsPlayableLevel(Save->Level) && Save->Progress.IsValid() &&
            (!Latest || Save->Revision > Latest->Revision))
        {
            Latest = Save;
            Generation = Index;
        }
    }
    return Latest;
}

FHumanFishingSlotSummary UHumanFishingSaveSubsystem::GetSlotSummary(int32 Slot, const UHumanFishingCatalog* Catalog) const
{
    FHumanFishingSlotSummary Summary;
    int32 Generation = INDEX_NONE;
    if (const UHumanFishingSaveGame* Save = ReadLatest(Slot, Generation, Summary.bExists))
    {
        Summary.bReadable = true;
        Summary.Captures = Save->Progress.NextId;
        Summary.Level = Save->Level;
        Summary.SavedAt = Save->SavedAt;
        for (FName Type : Save->Progress.CaughtTypes)
        {
            Summary.Discovered += Catalog && Catalog->Types.Contains(Type) ? 1 : 0;
        }
    }
    return Summary;
}

UHumanFishingSaveGame* UHumanFishingSaveSubsystem::WriteSlot(int32 Slot, int32 Generation, int64 Revision,
    const FHumanFishingProgress& Progress, FName Level) const
{
    auto* Save = NewObject<UHumanFishingSaveGame>();
    Save->Version = SaveVersion;
    Save->Revision = Revision;
    Save->Progress = Progress;
    Save->Level = IsBeachLevel(Level) && Progress.Health <= 0.f ? OceanLevel : Level;
    Save->SavedAt = FDateTime::UtcNow();
    const FString Name = SlotName(Slot, Generation);
    TArray<uint8> Bytes;
    if (UGameplayStatics::SaveGameToMemory(Save, Bytes))
    {
        uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
        FMemoryWriter Writer(Bytes, true);
        Writer.Seek(Bytes.Num());
        Writer << Checksum;
        if (UGameplayStatics::SaveDataToSlot(Bytes, Name, 0))
        {
            return Save;
        }
    }
    UE_LOG(LogHumanFishingSave, Warning, TEXT("Could not write save %s"), *Name);
    return nullptr;
}

bool UHumanFishingSaveSubsystem::StartSlot(UWorld* World, int32 Slot, bool bNewGame)
{
    if (!World || !Inventory || Slot < 0 || Slot >= SlotCount || ActiveSlot != INDEX_NONE)
    {
        return false;
    }

    int32 Generation = INDEX_NONE;
    bool bExists = false;
    UHumanFishingSaveGame* Save = ReadLatest(Slot, Generation, bExists);
    if (bNewGame)
    {
        if (Save && Save->Revision == MAX_int64)
        {
            return false;
        }
        Generation = Generation == 0 ? 1 : 0;
        Save = WriteSlot(Slot, Generation, Save ? Save->Revision + 1 : 1, FHumanFishingProgress{}, ShoreLevel);
    }
    if (!Save)
    {
        return false;
    }

    Inventory->Progress = Save->Progress;
    Inventory->SelectedBait = EHumanFishingBait::Dollar;
    ActiveSlot = Slot;
    ActiveGeneration = Generation;
    ActiveRevision = Save->Revision;
    ResumeLevel = Save->Level;
    bDirty = bSaveError = false;
    LastSaveAttempt = FPlatformTime::Seconds();
    OpenLevel(World, ResumeLevel);
    return true;
}

bool UHumanFishingSaveSubsystem::ReturnToMenu(UWorld* World)
{
    if (!World || !Flush())
    {
        return false;
    }
    DeveloperSnapshot.Reset();
    ActiveSlot = INDEX_NONE;
    ActiveGeneration = INDEX_NONE;
    ActiveRevision = 0;
    ResumeLevel = NAME_None;
    bDirty = bSaveError = false;
    Inventory->Progress = FHumanFishingProgress{};
    Inventory->SelectedBait = EHumanFishingBait::Dollar;
    OpenLevel(World, MenuLevel);
    return true;
}

void UHumanFishingSaveSubsystem::LevelReady(FName Level)
{
    if (IsPlayableLevel(Level) && Level != ResumeLevel)
    {
        ResumeLevel = Level;
        ProgressChanged(true);
    }
}

void UHumanFishingSaveSubsystem::ProgressChanged(bool bImmediate)
{
    if (ActiveSlot == INDEX_NONE || IsDeveloperMode())
    {
        return;
    }
    bDirty = true;
    if (bImmediate)
    {
        Flush();
    }
}

bool UHumanFishingSaveSubsystem::Flush()
{
    if (ActiveSlot == INDEX_NONE || !bDirty || IsDeveloperMode())
    {
        return true;
    }
    LastSaveAttempt = FPlatformTime::Seconds();
    const int32 Generation = 1 - ActiveGeneration;
    if (ActiveRevision == MAX_int64 || !Inventory || !IsPlayableLevel(ResumeLevel) ||
        !WriteSlot(ActiveSlot, Generation, ActiveRevision + 1, Inventory->GetProgress(), ResumeLevel))
    {
        bSaveError = true;
        return false;
    }
    ActiveGeneration = Generation;
    ++ActiveRevision;
    bDirty = bSaveError = false;
    return true;
}

bool UHumanFishingSaveSubsystem::Tick(float)
{
    if (bDirty && FPlatformTime::Seconds() - LastSaveAttempt >= HealthSaveInterval)
    {
        Flush();
    }
    return true;
}

void UHumanFishingSaveSubsystem::HandleExit()
{
    Flush();
}

bool UHumanFishingSaveSubsystem::EnterDeveloperMode(UWorld* World)
{
    if (!World || !Inventory || !IsPlayableLevel(ResumeLevel) || !Flush())
    {
        return false;
    }
    if (!IsDeveloperMode())
    {
        DeveloperSnapshot.Emplace(FDeveloperSnapshot{Inventory->GetProgress(), ResumeLevel});
    }
    return true;
}

bool UHumanFishingSaveSubsystem::LeaveDeveloperMode(UWorld* World)
{
    if (!World || !IsDeveloperMode())
    {
        return false;
    }
    Inventory->Progress = MoveTemp(DeveloperSnapshot->Progress);
    ResumeLevel = DeveloperSnapshot->Level;
    DeveloperSnapshot.Reset();
    OpenLevel(World, ResumeLevel);
    return true;
}
