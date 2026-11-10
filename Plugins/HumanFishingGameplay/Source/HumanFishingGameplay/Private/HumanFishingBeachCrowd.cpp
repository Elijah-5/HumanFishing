#include "HumanFishingBeachCrowd.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HumanFishingInventory.h"
#include "HumanFishingSave.h"
#include "HumanFishingVisitorAnimation.h"
#include "Math/UnrealMathUtility.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

namespace
{
const FName Tourist(TEXT("BeachTourist"));
constexpr float EdgeInset = 115.f;
constexpr float ArrivalRadius = 75.f;
constexpr float FullInterest = 100.f;
constexpr double PulsePeriod = 2.2;
constexpr double PulseWarning = 1.3;
constexpr double BurstStart = 1.7;
constexpr double BurstWear = 18.;
constexpr double DiverLandSpeed = 330.;
constexpr double DiverWaterSpeed = 460.;
constexpr double DiverReturnSpeed = 300.;

double BurstSeconds(double Elapsed)
{
    const double Cycles = FMath::FloorToDouble(Elapsed / PulsePeriod);
    return Cycles * (PulsePeriod - BurstStart) + FMath::Max(0., FMath::Fmod(Elapsed, PulsePeriod) - BurstStart);
}

UHumanFishingVisitorAnimation* VisitorAnimation(ACharacter& Pawn)
{
    return Cast<UHumanFishingVisitorAnimation>(Pawn.GetMesh()->GetAnimInstance());
}

bool CapsulesTouch(const ACharacter& Pawn, const ACharacter& Fish)
{
    const auto* A = Pawn.GetCapsuleComponent();
    const auto* B = Fish.GetCapsuleComponent();
    const FVector AHalf = A->GetUpVector() * (A->GetScaledCapsuleHalfHeight() - A->GetScaledCapsuleRadius());
    const FVector BHalf = B->GetUpVector() * (B->GetScaledCapsuleHalfHeight() - B->GetScaledCapsuleRadius());
    const FVector ACenter = A->GetComponentLocation();
    const FVector BCenter = B->GetComponentLocation();
    FVector ClosestA;
    FVector ClosestB;
    FMath::SegmentDistToSegmentSafe(ACenter - AHalf, ACenter + AHalf, BCenter - BHalf, BCenter + BHalf,
        ClosestA, ClosestB);
    return FVector::DistSquared(ClosestA, ClosestB) <= FMath::Square(
        A->GetScaledCapsuleRadius() + B->GetScaledCapsuleRadius() + .5);
}

void MoveDiver(ACharacter& Pawn, FVector Goal, double Speed, float DeltaSeconds, FHitResult& Hit)
{
    const FVector Here = Pawn.GetActorLocation();
    const FVector Offset = Goal - Here;
    if (Offset.SizeSquared2D() > 1.)
    {
        Pawn.SetActorRotation(FRotator(0., Offset.Rotation().Yaw, 0.));
    }
    Pawn.SetActorLocation(FMath::VInterpConstantTo(Here, Goal, DeltaSeconds, Speed), true, &Hit);
}

void AttachAccessory(ACharacter& Pawn, const FHumanFishingAccessory& Accessory)
{
    USkeletalMeshComponent* Body = Pawn.GetMesh();
    const USkeletalMesh* Mesh = Body->GetSkeletalMeshAsset();
    if (!Accessory.Mesh || !Mesh)
    {
        return;
    }
    const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
    int32 Index = Skeleton.FindBoneIndex(Accessory.Bone);
    if (Index == INDEX_NONE)
    {
        return;
    }

    FTransform Reference = FTransform::Identity;
    for (; Index != INDEX_NONE; Index = Skeleton.GetParentIndex(Index))
    {
        Reference = Reference * Skeleton.GetRefBonePose()[Index];
    }

    UStaticMeshComponent* Prop = NewObject<UStaticMeshComponent>(&Pawn);
    Pawn.AddInstanceComponent(Prop);
    Prop->SetupAttachment(Body, Accessory.Bone);
    Prop->SetRelativeTransform(Reference.Inverse());
    Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Prop->SetCanEverAffectNavigation(false);
    Prop->SetStaticMesh(Accessory.Mesh.Get());
    Prop->RegisterComponent();
}

}

AHumanFishingBeachCrowd::AHumanFishingBeachCrowd()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorEnableCollision(false);
}

#if !UE_BUILD_SHIPPING
FAutoConsoleCommandWithWorldArgsAndOutputDevice AHumanFishingBeachCrowd::DeveloperCommand(
    TEXT("hf.dev"), TEXT("Beach developer tools: hf.dev on | off | list | spawn <type>"),
    FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda(
        [](const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)
        {
            if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
            {
                UGameInstance* Game = World->GetGameInstance();
                auto* Save = Game ? Game->GetSubsystem<UHumanFishingSaveSubsystem>() : nullptr;
                if (Save && Args.Num() == 1 && Args[0].Equals(TEXT("on"), ESearchCase::IgnoreCase))
                {
                    Output.Log(Save->EnterDeveloperMode(World)
                        ? TEXT("Developer mode on. Field Guide preview enabled. Random arrivals paused.")
                        : TEXT("Could not enter developer mode. Start a game and check save status."));
                    return;
                }
                if (Save && Args.Num() == 1 && Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
                {
                    Output.Log(Save->LeaveDeveloperMode(World)
                        ? TEXT("Developer mode off. Restoring the previous checkpoint.")
                        : TEXT("Developer mode is already off."));
                    return;
                }
                for (TActorIterator<AHumanFishingBeachCrowd> Crowd(World); Crowd; ++Crowd)
                {
                    Crowd->RunDeveloperCommand(Args, Output);
                    return;
                }
            }
            Output.Log(TEXT("Use hf.dev on or off in a game. List and spawn are available on the beach."));
        }), ECVF_Cheat);

void AHumanFishingBeachCrowd::RunDeveloperCommand(const TArray<FString>& Args, FOutputDevice& Output)
{
    if (!Catalog || !PedestrianClass || !BeachBounds.IsValid)
    {
        Output.Log(TEXT("Beach crowd is not ready."));
        return;
    }

    const auto* Save = GetGameInstance()->GetSubsystem<UHumanFishingSaveSubsystem>();
    const bool bDeveloperMode = Save && Save->IsDeveloperMode();
    if (Args.Num() == 1 && Args[0].Equals(TEXT("list"), ESearchCase::IgnoreCase))
    {
        TArray<FString> Ids;
        for (const auto& Pair : Catalog->Types)
        {
            Ids.Add(Pair.Key.ToString());
        }
        Ids.Sort();
        Output.Logf(TEXT("Visitor IDs: %s"), *FString::Join(Ids, TEXT(", ")));
    }
    else if (Args.Num() == 2 && Args[0].Equals(TEXT("spawn"), ESearchCase::IgnoreCase))
    {
        if (!bDeveloperMode)
        {
            Output.Log(TEXT("Run hf.dev on first."));
            return;
        }
        const FName RequestedType(*Args[1]);
        FName Type = NAME_None;
        for (const auto& Pair : Catalog->Types)
        {
            if (Pair.Key == RequestedType)
            {
                Type = Pair.Key;
                break;
            }
        }
        if (Type.IsNone())
        {
            Output.Logf(TEXT("Unknown visitor ID '%s'. Use hf.dev list."), *Args[1]);
            return;
        }

        FVector Point = BeachBounds.GetCenter();
        const APlayerController* Player = GetWorld()->GetFirstPlayerController();
        if (const APawn* Fish = Player ? Player->GetPawn() : nullptr)
        {
            const FVector Here = Fish->GetActorLocation();
            const FVector Extent = BeachBounds.GetExtent();
            const double MarginX = FMath::Min(Extent.X, EdgeInset);
            const double MarginY = FMath::Min(Extent.Y, EdgeInset);
            Point.X = FMath::Clamp(Here.X + 250., BeachBounds.Min.X + MarginX, BeachBounds.Max.X - MarginX);
            Point.Y = FMath::Clamp(Here.Y, BeachBounds.Min.Y + MarginY, BeachBounds.Max.Y - MarginY);
        }
        Point.Z = BeachBounds.Max.Z;
        const double Now = GetWorld()->GetTimeSeconds();
        const bool bNorth = Point.Y >= BeachBounds.GetCenter().Y;
        const bool bSpawned = SpawnVisitor(Now, bNorth, Type, Point, true) ||
            SpawnVisitor(Now, bNorth, Type, SidePoint(bNorth), true) ||
            SpawnVisitor(Now, !bNorth, Type, SidePoint(!bNorth), true);
        if (bSpawned)
        {
            Output.Logf(TEXT("Spawned %s."), *Type.ToString());
        }
        else
        {
            Output.Logf(TEXT("Could not spawn %s on a clear beach NavMesh point."), *Type.ToString());
        }
    }
    else
    {
        Output.Logf(TEXT("Developer mode %s. Usage: hf.dev on | off | list | spawn <type>"),
            bDeveloperMode ? TEXT("on") : TEXT("off"));
    }
}
#endif

void AHumanFishingBeachCrowd::BeginPlay()
{
    Super::BeginPlay();
    if (!IsValid(Beach) || !PedestrianClass || !Catalog)
    {
        SetActorTickEnabled(false);
        return;
    }

    BeachBounds = Beach->GetComponentsBoundingBox();
    if (!BeachBounds.IsValid)
    {
        SetActorTickEnabled(false);
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    for (TActorIterator<ACharacter> It(GetWorld(), PedestrianClass); It && Visitors.Num() < MaxPedestrians; ++It)
    {
        FVisitor& Visitor = Visitors.AddDefaulted_GetRef();
        Visitor.Pawn = *It;
        Visitor.Type = Tourist;
        Visitor.bExitNorth = (*It)->GetActorLocation().Y <= BeachBounds.GetCenter().Y;
        if (auto* Anim = VisitorAnimation(**It))
        {
            Anim->Configure(FindProfile(&Visitor));
        }
        BeginStay(Visitor, Now);
    }
    QueueSpawn(Now);
}

bool AHumanFishingBeachCrowd::IsOnBeach(FVector Position) const
{
    return BeachBounds.IsValid && Position.X >= BeachBounds.Min.X && Position.X <= BeachBounds.Max.X &&
        Position.Y >= BeachBounds.Min.Y && Position.Y <= BeachBounds.Max.Y &&
        Position.Z >= BeachBounds.Max.Z - 5 && Position.Z <= BeachBounds.Max.Z + 130;
}

void AHumanFishingBeachCrowd::QueueSpawn(double Now)
{
    const float Min = FMath::Max(.25f, SpawnIntervalMin);
    SpawnAt = Now + FMath::FRandRange(Min, FMath::Max(Min, SpawnIntervalMax));
}

void AHumanFishingBeachCrowd::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const double Now = GetWorld()->GetTimeSeconds();
    for (FVisitor& Visitor : Visitors)
    {
        if (Visitor.WaterPhase == EWaterReelPhase::Returning)
        {
            if (ACharacter* Pawn = Visitor.Pawn.Get())
            {
                AdvanceReturn(Visitor, *Pawn, DeltaSeconds);
            }
        }
    }
    if (Now < NextCrowdUpdate)
    {
        return;
    }
    NextCrowdUpdate = Now + .25;
    WalkVisitors(Now);

#if !UE_BUILD_SHIPPING
    const auto* Save = GetGameInstance()->GetSubsystem<UHumanFishingSaveSubsystem>();
    if (Save && Save->IsDeveloperMode())
    {
        return;
    }
#endif
    if (Now >= SpawnAt)
    {
        QueueSpawn(Now);
        if (Visitors.Num() < MaxPedestrians)
        {
            const bool bNorth = RepeatedSide >= 2 ? !bLastSpawnNorth : FMath::RandBool();
            if (SpawnVisitor(Now, bNorth, ChooseType(), SidePoint(bNorth)))
            {
                RepeatedSide = RepeatedSide > 0 && bNorth == bLastSpawnNorth ? RepeatedSide + 1 : 1;
                bLastSpawnNorth = bNorth;
            }
        }
    }
}

FVector AHumanFishingBeachCrowd::SidePoint(bool bNorth) const
{
    const double NearShore = BeachBounds.Min.X + EdgeInset + 45.;
    const double Far = FMath::Max(NearShore, BeachBounds.Min.X + BeachBounds.GetSize().X * .61);
    return FVector(FMath::FRandRange(NearShore, Far),
        bNorth ? BeachBounds.Max.Y - EdgeInset : BeachBounds.Min.Y + EdgeInset, BeachBounds.Max.Z);
}

bool AHumanFishingBeachCrowd::ProjectToBeach(FVector Point, FVector& Projected) const
{
    const auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation Result;
    if (!Nav || !Nav->ProjectPointToNavigation(Point, Result, FVector(90, 90, 150)))
    {
        return false;
    }
    Projected = Result.Location;
    return IsOnBeach(Projected);
}

bool AHumanFishingBeachCrowd::CanReach(ACharacter& Pawn, FVector Goal) const
{
    const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        GetWorld(), Pawn.GetActorLocation(), Goal, &Pawn);
    return Path && Path->IsValid() && !Path->IsPartial();
}

FName AHumanFishingBeachCrowd::ChooseType() const
{
    const UGameInstance* Game = GetWorld()->GetGameInstance();
    const UHumanFishingInventory* Inventory = Game ? Game->GetSubsystem<UHumanFishingInventory>() : nullptr;
    const int32 Captures = Inventory ? Inventory->GetLifetimeCatchCount() : 0;
    constexpr int32 RarityCount = static_cast<int32>(EHumanFishingRarity::Count);
    TArray<FName> Eligible[RarityCount];
    for (const auto& Pair : Catalog->Types)
    {
        const FHumanFishingCatchType& Type = Pair.Value;
        const int32 Index = static_cast<int32>(Type.Rarity);
        if (Index >= RarityCount || Captures < Type.RequiredCatches)
        {
            continue;
        }
        if (Type.bUniquePerSession &&
            ((Inventory && Inventory->HasCaughtType(Pair.Key)) ||
             Visitors.ContainsByPredicate([&Pair](const FVisitor& Visitor)
                { return Visitor.Pawn.IsValid() && Visitor.Type == Pair.Key; })))
        {
            continue;
        }
        Eligible[Index].Add(Pair.Key);
    }

    int32 Weights[RarityCount] = {};
    int32 TotalWeight = 0;
    for (int32 Index = 0; Index < RarityCount; ++Index)
    {
        if (!Eligible[Index].IsEmpty())
        {
            Weights[Index] = UHumanFishingCatalog::GetRarityRules(static_cast<EHumanFishingRarity>(Index)).SpawnWeight;
            TotalWeight += Weights[Index];
        }
    }
    if (TotalWeight == 0)
    {
        return NAME_None;
    }
    int32 Ticket = FMath::RandRange(1, TotalWeight);
    for (int32 Index = 0; Index < RarityCount; ++Index)
    {
        Ticket -= Weights[Index];
        if (Ticket <= 0)
        {
            return Eligible[Index][FMath::RandRange(0, Eligible[Index].Num() - 1)];
        }
    }
    return NAME_None;
}

bool AHumanFishingBeachCrowd::SpawnVisitor(double Now, bool bNorth, FName Type, FVector Point, bool bStartInland)
{
    const FHumanFishingCatchType* Profile = Catalog->Types.Find(Type);
    FVector Entry;
    if (!Profile || !ProjectToBeach(Point, Entry))
    {
        return false;
    }

    const auto* Default = PedestrianClass->GetDefaultObject<ACharacter>();
    const float HalfHeight = Default->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    ACharacter* Pawn = GetWorld()->SpawnActor<ACharacter>(PedestrianClass,
        Entry + FVector(0, 0, HalfHeight + 2.f), FRotator(0, bNorth ? -90 : 90, 0), Parameters);
    if (!Pawn)
    {
        return false;
    }

    if (Profile->BeachMesh)
    {
        Pawn->GetMesh()->SetSkeletalMeshAsset(Profile->BeachMesh.Get());
    }
    for (const FHumanFishingAccessory& Accessory : Profile->Accessories)
    {
        AttachAccessory(*Pawn, Accessory);
    }
    Pawn->GetCharacterMovement()->MaxWalkSpeed = Profile->WalkSpeed;
    if (auto* Anim = VisitorAnimation(*Pawn))
    {
        Anim->Configure(Profile);
    }
    Pawn->SpawnDefaultController();

    FVisitor& Visitor = Visitors.AddDefaulted_GetRef();
    Visitor.Pawn = Pawn;
    Visitor.Type = Type;
    Visitor.bExitNorth = !bNorth;
    Visitor.RouteAfter = Now;
    if (bStartInland)
    {
        BeginStay(Visitor, Now);
    }
    return true;
}

void AHumanFishingBeachCrowd::WalkVisitors(double Now)
{
    for (int32 Index = Visitors.Num() - 1; Index >= 0; --Index)
    {
        FVisitor& Visitor = Visitors[Index];
        ACharacter* Pawn = Visitor.Pawn.Get();
        if (!Pawn || Pawn->IsActorBeingDestroyed())
        {
            Visitors.RemoveAt(Index);
            continue;
        }
        if (Pawn != HeldVisitor.Get() && Visitor.WaterPhase != EWaterReelPhase::Returning &&
            AdvanceVisitor(Visitor, *Pawn, Now))
        {
            Pawn->Destroy();
            Visitors.RemoveAt(Index);
        }
    }
}

void AHumanFishingBeachCrowd::BeginStay(FVisitor& Visitor, double Now)
{
    const FHumanFishingCatchType* Profile = FindProfile(&Visitor);
    const auto& Rules = UHumanFishingCatalog::GetRarityRules(Profile ? Profile->Rarity : EHumanFishingRarity::White);
    const FVector2D Override = Profile ? Profile->StaySecondsOverride : FVector2D::ZeroVector;
    const FVector2D Duration = Override.X > 0 && Override.Y >= Override.X ? Override : Rules.StaySeconds;
    Visitor.Phase = EVisitPhase::Staying;
    Visitor.StayUntil = Now + FMath::FRandRange(Duration.X, Duration.Y);
    Visitor.RouteAfter = Now + FMath::FRandRange(3., 6.);
}

bool AHumanFishingBeachCrowd::FindInlandPoint(ACharacter& Pawn, bool bRoaming, FVector& Destination) const
{
    const FVector Here = Pawn.GetActorLocation();
    const FVector Extent = BeachBounds.GetSize();
    const FVector2D Near(BeachBounds.Min.X + Extent.X * .12, BeachBounds.Min.Y + Extent.Y * .22);
    const FVector2D Far(BeachBounds.Min.X + Extent.X * .45, BeachBounds.Max.Y - Extent.Y * .22);
    for (int32 Attempt = 0; Attempt < 6; ++Attempt)
    {
        FVector Point;
        if (bRoaming)
        {
            const FVector Offset = FRotator(0, FMath::FRandRange(0., 360.), 0).Vector() * FMath::FRandRange(200., 460.);
            Point.X = FMath::Clamp(Here.X + Offset.X, Near.X, Far.X);
            Point.Y = FMath::Clamp(Here.Y + Offset.Y, Near.Y, Far.Y);
        }
        else
        {
            Point.X = FMath::FRandRange(Near.X, Far.X);
            Point.Y = FMath::FRandRange(Near.Y, Far.Y);
        }
        Point.Z = BeachBounds.Max.Z;
        if (ProjectToBeach(Point, Destination) &&
            FVector::DistSquared2D(Here, Destination) >= FMath::Square(120.) && CanReach(Pawn, Destination))
        {
            return true;
        }
    }
    return false;
}

bool AHumanFishingBeachCrowd::StartRoute(FVisitor& Visitor, AAIController& AI, FVector Goal, double Now)
{
    if (AI.MoveToLocation(Goal, 50.f, false, true, true, false, nullptr, false) == EPathFollowingRequestResult::Failed)
    {
        return false;
    }
    Visitor.Destination = Goal;
    Visitor.RouteStartedAt = Now;
    return true;
}

bool AHumanFishingBeachCrowd::AdvanceVisitor(FVisitor& Visitor, ACharacter& Pawn, double Now)
{
    AAIController* AI = Cast<AAIController>(Pawn.GetController());
    if (!AI)
    {
        return false;
    }
    if (Visitor.Phase == EVisitPhase::Staying && Now >= Visitor.StayUntil)
    {
        AI->StopMovement();
        Visitor.Destination.Reset();
        Visitor.Phase = EVisitPhase::Leaving;
        Visitor.RouteAfter = Now;
    }
    if (Visitor.Destination.IsSet())
    {
        const bool bArrived = FVector::DistSquared2D(Pawn.GetActorLocation(), Visitor.Destination.GetValue()) <=
            FMath::Square(ArrivalRadius);
        if (bArrived)
        {
            AI->StopMovement();
            Visitor.Destination.Reset();
            if (Visitor.Phase == EVisitPhase::Leaving)
            {
                return true;
            }
            if (Visitor.Phase == EVisitPhase::Entering)
            {
                BeginStay(Visitor, Now);
            }
            else
            {
                Visitor.RouteAfter = Now + FMath::FRandRange(3., 6.);
            }
        }
        else if (AI->GetMoveStatus() == EPathFollowingStatus::Idle || Now - Visitor.RouteStartedAt >= 30.)
        {
            AI->StopMovement();
            Visitor.Destination.Reset();
            if (Visitor.Phase == EVisitPhase::Leaving)
            {
                Visitor.bExitNorth = !Visitor.bExitNorth;
            }
            Visitor.RouteAfter = Now + 1.;
        }
        return false;
    }
    if (Now < Visitor.RouteAfter)
    {
        return false;
    }
    Visitor.RouteAfter = Now + 1.;

    FVector Goal;
    if (Visitor.Phase == EVisitPhase::Leaving)
    {
        for (bool bNorth : {Visitor.bExitNorth, !Visitor.bExitNorth})
        {
            if (ProjectToBeach(SidePoint(bNorth), Goal) && CanReach(Pawn, Goal) && StartRoute(Visitor, *AI, Goal, Now))
            {
                Visitor.bExitNorth = bNorth;
                break;
            }
        }
    }
    else if (FindInlandPoint(Pawn, Visitor.Phase == EVisitPhase::Staying, Goal))
    {
        StartRoute(Visitor, *AI, Goal, Now);
    }
    return false;
}

bool AHumanFishingBeachCrowd::ObserveBait(int32 CastId, FVector Bait, FVector Goal, double Appeal,
    ACharacter*& Candidate, double& Interest)
{
    Candidate = nullptr;
    Interest = 0;
    if (HeldVisitor.IsValid())
    {
        return false;
    }

    const bool bNewCast = ObservedCast != CastId;
    ObservedCast = CastId;
    const double Now = GetWorld()->GetTimeSeconds();
    const double Delta = GetWorld()->GetDeltaSeconds();
    double BestDistance = TNumericLimits<double>::Max();

    for (FVisitor& Visitor : Visitors)
    {
        ACharacter* Pawn = Visitor.Pawn.Get();
        const FHumanFishingCatchType* Profile = FindProfile(&Visitor);
        if (bNewCast)
        {
            Visitor.Interest = 0;
            Visitor.CheckPathAfter = 0;
        }
        if (!Pawn || Pawn->IsActorBeingDestroyed() || !Profile || Visitor.IgnoredCast == CastId ||
            Visitor.Phase != EVisitPhase::Staying || Visitor.WaterPhase == EWaterReelPhase::Returning ||
            Now >= Visitor.StayUntil)
        {
            continue;
        }

        const double Distance = FVector::Dist2D(Pawn->GetActorLocation(), Bait);
        if (Distance > Profile->InterestRadius)
        {
            Visitor.Interest = 0;
            Visitor.CheckPathAfter = 0;
            continue;
        }
        if (Now >= Visitor.CheckPathAfter)
        {
            Visitor.bCanReachBait = CanReach(*Pawn, Goal);
            Visitor.CheckPathAfter = Now + .25;
        }
        if (!Visitor.bCanReachBait)
        {
            Visitor.Interest = 0;
            continue;
        }

        const double DistanceFactor = FMath::Lerp(.50, .25, FMath::Clamp((Distance - 100.) / 500., 0., 1.));
        const double Gain = FullInterest * FMath::Max(0., Appeal) * Profile->MoneyPreference *
            Profile->InterestGainMultiplier * DistanceFactor * Delta;
        Visitor.Interest = FMath::Clamp(Visitor.Interest + Gain, 0., static_cast<double>(FullInterest));
        if (Visitor.Interest > Interest || (Visitor.Interest > 0 && Visitor.Interest == Interest && Distance < BestDistance))
        {
            Candidate = Pawn;
            Interest = Visitor.Interest;
            BestDistance = Distance;
        }
    }

    if (Candidate && Interest >= FullInterest)
    {
        HeldVisitor = Candidate;
        HeldSince = Now;
        if (FVisitor* Visitor = FindVisitor(Candidate))
        {
            Visitor->Destination.Reset();
        }
        if (AAIController* AI = Cast<AAIController>(Candidate->GetController()))
        {
            AI->StopMovement();
        }
        for (FVisitor& Visitor : Visitors)
        {
            Visitor.Interest = 0;
        }
        return true;
    }
    return false;
}

bool AHumanFishingBeachCrowd::ReleaseTarget(ACharacter* Pedestrian)
{
    if (!Pedestrian || Pedestrian != HeldVisitor.Get())
    {
        return false;
    }

    HeldVisitor.Reset();
    if (FVisitor* Visitor = FindVisitor(Pedestrian))
    {
        const double Now = GetWorld()->GetTimeSeconds();
        Visitor->RouteAfter = Now + .5;
        Visitor->StayUntil += Now - HeldSince;
        Visitor->bReelActive = Visitor->bPulling = false;
        if (const auto* Profile = FindProfile(Visitor))
        {
            Pedestrian->GetCharacterMovement()->MaxWalkSpeed = Profile->WalkSpeed;
        }
        if (Visitor->WaterPhase != EWaterReelPhase::Dry)
        {
            Visitor->WaterPhase = EWaterReelPhase::Returning;
            if (auto* Anim = VisitorAnimation(*Pedestrian))
            {
                Anim->BeginWaterReturn();
            }
        }
        else if (auto* Anim = VisitorAnimation(*Pedestrian))
        {
            Anim->ClearFishingAction();
        }
    }
    return true;
}

void AHumanFishingBeachCrowd::IgnoreCast(ACharacter* Pedestrian, int32 CastId)
{
    if (FVisitor* Visitor = FindVisitor(Pedestrian))
    {
        Visitor->IgnoredCast = CastId;
    }
}

FName AHumanFishingBeachCrowd::GetCatchType(const ACharacter* Pedestrian) const
{
    const FVisitor* Visitor = FindVisitor(Pedestrian);
    return Visitor ? Visitor->Type : Tourist;
}

double AHumanFishingBeachCrowd::GetInterestDecay(const ACharacter* Pedestrian) const
{
    const FHumanFishingCatchType* Profile = FindProfile(FindVisitor(Pedestrian));
    return Profile ? Profile->InterestDecayPerSecond : 8.;
}

double AHumanFishingBeachCrowd::GetLineWear(const ACharacter* Pedestrian) const
{
    const FHumanFishingCatchType* Profile = FindProfile(FindVisitor(Pedestrian));
    return Profile ? Profile->LineWearPerSecond : 8.;
}

bool AHumanFishingBeachCrowd::NeedsWaterContact(const ACharacter* Pedestrian) const
{
    const auto* Profile = FindProfile(FindVisitor(Pedestrian));
    return Profile && Profile->CatchFinish == EHumanFishingCatchFinish::WaterContact;
}

bool AHumanFishingBeachCrowd::IsWaterReel(const ACharacter* Pedestrian) const
{
    const FVisitor* Visitor = FindVisitor(Pedestrian);
    return Visitor && (Visitor->WaterPhase == EWaterReelPhase::Crossing || Visitor->WaterPhase == EWaterReelPhase::InWater);
}

FString AHumanFishingBeachCrowd::GetInterestCaption(const ACharacter* Pedestrian) const
{
    const auto* Profile = FindProfile(FindVisitor(Pedestrian));
    return Profile && Profile->InterestGainMultiplier < 1.f && Profile->InterestRadius > 600.f
        ? TEXT("Observing") : TEXT("Interest");
}

FString AHumanFishingBeachCrowd::GetReelPrompt(const ACharacter* Pedestrian, const FString& DefaultText) const
{
    const FVisitor* Visitor = FindVisitor(Pedestrian);
    const auto* Profile = FindProfile(Visitor);
    if (!Visitor || !Visitor->bReelActive || !Profile)
    {
        return DefaultText;
    }
    if (Profile->Struggle == EHumanFishingStruggle::Pulse &&
        FMath::Fmod(GetWorld()->GetTimeSeconds() - Visitor->ReelStartedAt, PulsePeriod) >= PulseWarning)
    {
        return TEXT("Release E to brace");
    }
    return IsWaterReel(Pedestrian) ? TEXT("Hold E to pull the diver to you") : DefaultText;
}

UAnimMontage* AHumanFishingBeachCrowd::GetInspectAnimation(const ACharacter* Pedestrian, UAnimMontage* DefaultAnimation) const
{
    const auto* Profile = FindProfile(FindVisitor(Pedestrian));
    return Profile && Profile->InspectAnimation ? Profile->InspectAnimation.Get() : DefaultAnimation;
}

void AHumanFishingBeachCrowd::BeginReelSession(ACharacter* Pedestrian, const ACharacter* Fish, FVector ShoreGoal)
{
    FVisitor* Visitor = FindVisitor(Pedestrian);
    if (!Visitor || !Fish)
    {
        return;
    }
    Visitor->bReelActive = true;
    Visitor->bPulling = false;
    Visitor->ReelStartedAt = GetWorld()->GetTimeSeconds();
    Visitor->LastWearSample = Visitor->ExtraWear = 0.;
    if (NeedsWaterContact(Pedestrian))
    {
        Visitor->ShoreGoal = ShoreGoal;
        Visitor->FishAnchor = Fish->GetActorLocation();
        const auto* Capsule = Pedestrian->GetCapsuleComponent();
        const FVector DryCenter(ShoreGoal.X, ShoreGoal.Y, BeachBounds.Max.Z + Capsule->GetScaledCapsuleHalfHeight() + 2.);
        Visitor->WaterAnchor = FVector(BeachBounds.Min.X - Capsule->GetScaledCapsuleRadius() - 10., DryCenter.Y, DryCenter.Z);
        Visitor->TotalReelDistance = FMath::Max(1., FVector::Dist2D(Pedestrian->GetActorLocation(), ShoreGoal) +
            FVector::Dist(DryCenter, Visitor->WaterAnchor) + FVector::Dist(Visitor->WaterAnchor, Visitor->FishAnchor));
        Pedestrian->GetCharacterMovement()->MaxWalkSpeed = DiverLandSpeed;
    }
}

void AHumanFishingBeachCrowd::SetReelInput(ACharacter* Pedestrian, bool bPulling)
{
    if (FVisitor* Visitor = FindVisitor(Pedestrian); Visitor && Visitor->bReelActive)
    {
        SampleLineWear(Pedestrian);
        Visitor->bPulling = bPulling;
    }
}

double AHumanFishingBeachCrowd::SampleLineWear(ACharacter* Pedestrian)
{
    FVisitor* Visitor = FindVisitor(Pedestrian);
    const auto* Profile = FindProfile(Visitor);
    if (!Visitor || !Visitor->bReelActive || !Profile)
    {
        return 0.;
    }
    const double Elapsed = FMath::Max(0., GetWorld()->GetTimeSeconds() - Visitor->ReelStartedAt);
    if (Profile->Struggle == EHumanFishingStruggle::Pulse)
    {
        if (Visitor->bPulling)
        {
            Visitor->ExtraWear += BurstWear * (BurstSeconds(Elapsed) - BurstSeconds(Visitor->LastWearSample));
        }
        if (auto* Anim = VisitorAnimation(*Pedestrian))
        {
            Anim->ShowStruggle(FMath::Fmod(Elapsed, PulsePeriod) - PulseWarning);
        }
    }
    Visitor->LastWearSample = Elapsed;
    return Elapsed * Profile->LineWearPerSecond + Visitor->ExtraWear;
}

int32 AHumanFishingBeachCrowd::AdvanceContactReel(ACharacter* Pedestrian, const ACharacter* Fish, double& Progress)
{
    Progress = 0.;
    FVisitor* Visitor = FindVisitor(Pedestrian);
    if (!Visitor || !Visitor->bPulling || !Fish)
    {
        return -1;
    }
    const FVector Here = Pedestrian->GetActorLocation();
    double Remaining = 0.;
    if (Visitor->WaterPhase == EWaterReelPhase::Dry)
    {
        const double BankDistance = FVector::Dist2D(Here, Visitor->ShoreGoal);
        if (BankDistance > 55.)
        {
            const AAIController* AI = Cast<AAIController>(Pedestrian->GetController());
            Remaining = BankDistance + FVector::Dist2D(Visitor->ShoreGoal, Visitor->WaterAnchor) +
                FVector::Dist(Visitor->WaterAnchor, Visitor->FishAnchor);
            Progress = FMath::Clamp(1. - Remaining / Visitor->TotalReelDistance, 0., .99);
            return AI && AI->GetMoveStatus() != EPathFollowingStatus::Idle ? 0 : -1;
        }
        if (auto* AI = Cast<AAIController>(Pedestrian->GetController()))
        {
            AI->StopMovement();
        }
        Pedestrian->GetCharacterMovement()->DisableMovement();
        Visitor->DryAnchor = Here;
        Visitor->WaterAnchor.Y = Here.Y;
        Visitor->WaterAnchor.Z = Here.Z;
        Visitor->bReachedWaterAnchor = false;
        Visitor->WaterPhase = EWaterReelPhase::Crossing;
    }
    const bool bCrossing = Visitor->WaterPhase == EWaterReelPhase::Crossing;
    const FVector Goal = bCrossing ? Visitor->WaterAnchor : Visitor->FishAnchor;
    FHitResult Hit;
    MoveDiver(*Pedestrian, Goal, DiverWaterSpeed, GetWorld()->GetDeltaSeconds(), Hit);
    const FVector After = Pedestrian->GetActorLocation();
    // A blocking sweep may leave a separation margin at the reported impact.
    const bool bHitFishCapsule = Hit.GetComponent() == Fish->GetCapsuleComponent();
    if (After.X < BeachBounds.Min.X && (bHitFishCapsule || CapsulesTouch(*Pedestrian, *Fish)))
    {
        return 1;
    }
    if (Hit.IsValidBlockingHit() || Hit.bStartPenetrating)
    {
        return -1;
    }
    if (bCrossing && FVector::DistSquared(After, Goal) < 1.)
    {
        Visitor->bReachedWaterAnchor = true;
        Visitor->WaterPhase = EWaterReelPhase::InWater;
        if (auto* Anim = VisitorAnimation(*Pedestrian))
        {
            Anim->BeginWaterEntry();
        }
    }
    Remaining = FVector::Dist(After, Goal) + (bCrossing ? FVector::Dist(Goal, Visitor->FishAnchor) : 0.);
    Progress = FMath::Clamp(1. - Remaining / Visitor->TotalReelDistance, 0., .99);
    return 0;
}

void AHumanFishingBeachCrowd::AdvanceReturn(FVisitor& Visitor, ACharacter& Pawn, float DeltaSeconds)
{
    const FVector Goal = Visitor.bReachedWaterAnchor ? Visitor.WaterAnchor : Visitor.DryAnchor;
    FHitResult Hit;
    MoveDiver(Pawn, Goal, DiverReturnSpeed, DeltaSeconds, Hit);
    if (FVector::DistSquared(Pawn.GetActorLocation(), Goal) >= 1.)
    {
        Visitor.StayUntil += DeltaSeconds;
        return;
    }
    if (Visitor.bReachedWaterAnchor)
    {
        Visitor.bReachedWaterAnchor = false;
        return;
    }
    Visitor.WaterPhase = EWaterReelPhase::Dry;
    Visitor.RouteAfter = GetWorld()->GetTimeSeconds() + .5;
    Pawn.GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (auto* Anim = VisitorAnimation(Pawn))
    {
        Anim->ClearFishingAction();
    }
}

AHumanFishingBeachCrowd::FVisitor* AHumanFishingBeachCrowd::FindVisitor(const ACharacter* Pedestrian)
{
    return Pedestrian
        ? Visitors.FindByPredicate([Pedestrian](const FVisitor& Visitor) { return Visitor.Pawn.Get() == Pedestrian; })
        : nullptr;
}

const AHumanFishingBeachCrowd::FVisitor* AHumanFishingBeachCrowd::FindVisitor(const ACharacter* Pedestrian) const
{
    return Pedestrian
        ? Visitors.FindByPredicate([Pedestrian](const FVisitor& Visitor) { return Visitor.Pawn.Get() == Pedestrian; })
        : nullptr;
}

const FHumanFishingCatchType* AHumanFishingBeachCrowd::FindProfile(const FVisitor* Visitor) const
{
    return Catalog ? Catalog->Types.Find(Visitor ? Visitor->Type : Tourist) : nullptr;
}
