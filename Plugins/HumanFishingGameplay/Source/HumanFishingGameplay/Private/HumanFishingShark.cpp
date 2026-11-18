#include "HumanFishingShark.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "HumanFishingInventory.h"
#include "HumanFishingPlayerUI.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HumanFishingShark"

namespace
{
constexpr float ReachedPatrolPoint = 200.f;
constexpr float PatrolPointTimeout = 12.f;
constexpr float MaxSwimPitch = 40.f;
const FLinearColor SharkGrey(.32f, .38f, .44f);

UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, FName Name, UStaticMesh* Mesh,
    const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
    auto* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Part->SetupAttachment(Parent);
    Part->SetStaticMesh(Mesh);
    Part->SetRelativeTransform(FTransform(Rotation, Location, Scale));
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(false);
    return Part;
}

#if WITH_EDITORONLY_DATA
USphereComponent* MakePreview(AActor* Owner, USceneComponent* Parent, FName Name, const FColor& Color)
{
    auto* Preview = Owner->CreateDefaultSubobject<USphereComponent>(Name);
    Preview->SetupAttachment(Parent);
    Preview->bIsEditorOnly = true;
    Preview->SetHiddenInGame(true);
    Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Preview->SetGenerateOverlapEvents(false);
    Preview->SetCanEverAffectNavigation(false);
    Preview->ShapeColor = Color;
    Preview->bDrawOnlyIfSelected = true;
    return Preview;
}
#endif
}

AHumanFishingShark::AHumanFishingShark()
{
    PrimaryActorTick.bCanEverTick = true;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(90.f);
    // The shark sweeps against level geometry so it can't swim through rocks, but it never blocks the fish.
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetGenerateOverlapEvents(false);
    Collision->SetCanEverAffectNavigation(false);
    RootComponent = Collision;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Body = MakePart(this, Collision, TEXT("Body"), SphereMesh.Object,
        FVector::ZeroVector, FRotator::ZeroRotator, FVector(3.2f, 1.f, 1.1f));
    Fin = MakePart(this, Collision, TEXT("Fin"), ConeMesh.Object,
        FVector(10.f, 0.f, 60.f), FRotator(0.f, 0.f, 0.f), FVector(.55f, .12f, .65f));
    Tail = MakePart(this, Collision, TEXT("Tail"), ConeMesh.Object,
        FVector(-175.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f), FVector(.7f, .12f, .6f));

#if WITH_EDITORONLY_DATA
    DetectionPreview = MakePreview(this, Collision, TEXT("DetectionPreview"), FColor(255, 170, 40));
    LosePreview = MakePreview(this, Collision, TEXT("LosePreview"), FColor(90, 160, 255));
    PatrolPreview = MakePreview(this, Collision, TEXT("PatrolPreview"), FColor(80, 220, 120));
#endif
}

EHumanFishingSharkState AHumanFishingShark::ChooseState(EHumanFishingSharkState Current,
    const FHumanFishingSharkSenses& Senses, const FHumanFishingSharkRanges& Ranges, bool& bOutSteal)
{
    bOutSteal = false;
    if (Senses.bCoolingDown)
    {
        return EHumanFishingSharkState::Retreat;
    }
    if (!Senses.bTargetAvailable)
    {
        return EHumanFishingSharkState::Patrol;
    }

    const float Lose = FMath::Max(Ranges.LoseRadius, Ranges.DetectionRadius);
    if (Current == EHumanFishingSharkState::Chase)
    {
        if (Senses.TargetDistance <= Ranges.ContactRadius)
        {
            bOutSteal = true;
            return EHumanFishingSharkState::Retreat;
        }
        return Senses.TargetDistance > Lose ? EHumanFishingSharkState::Patrol : EHumanFishingSharkState::Chase;
    }
    return Senses.TargetDistance <= Ranges.DetectionRadius ? EHumanFishingSharkState::Chase
        : EHumanFishingSharkState::Patrol;
}

void AHumanFishingShark::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    for (UStaticMeshComponent* Part : { Body.Get(), Fin.Get(), Tail.Get() })
    {
        if (Part && Part->GetStaticMesh() && !Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0)))
        {
            if (UMaterialInstanceDynamic* Material = Part->CreateAndSetMaterialInstanceDynamic(0))
            {
                Material->SetVectorParameterValue(TEXT("Color"), SharkGrey);
            }
        }
    }
#if WITH_EDITORONLY_DATA
    // The spheres scale with the actor, so cancel that out to show the real ranges.
    const float Scale = FMath::Max(GetActorScale3D().GetAbsMax(), KINDA_SMALL_NUMBER);
    DetectionPreview->SetSphereRadius(DetectionRadius / Scale);
    LosePreview->SetSphereRadius(FMath::Max(LoseRadius, DetectionRadius) / Scale);
    PatrolPreview->SetSphereRadius(PatrolRadius / Scale);
#endif
}

void AHumanFishingShark::BeginPlay()
{
    Super::BeginPlay();
    Home = GetActorLocation();
    PickPatrolPoint(Home);
}

float AHumanFishingShark::ScaleForLoad(float Base, float Load, float FullLoadBonus)
{
    return Base * (1.f + FMath::Clamp(Load, 0.f, 1.f) * FMath::Max(FullLoadBonus, 0.f));
}

FHumanFishingSharkRanges AHumanFishingShark::GetRanges(float Load) const
{
    FHumanFishingSharkRanges Ranges;
    Ranges.DetectionRadius = ScaleForLoad(DetectionRadius, Load, FullLoadDetectionBonus);
    Ranges.LoseRadius = ScaleForLoad(FMath::Max(LoseRadius, DetectionRadius), Load, FullLoadDetectionBonus);
    Ranges.ContactRadius = ContactRadius;
    return Ranges;
}

APawn* AHumanFishingShark::FindTarget(UHumanFishingPlayerUI*& OutUI) const
{
    APawn* Fish = UGameplayStatics::GetPlayerPawn(this, 0);
    OutUI = Fish ? Fish->FindComponentByClass<UHumanFishingPlayerUI>() : nullptr;
    if (!Fish)
    {
        return nullptr;
    }
    // Trading at the stall or looking at the backpack is a safe moment.
    if (OutUI && (OutUI->IsShopOpen() || OutUI->IsBackpackOpen()))
    {
        return nullptr;
    }
    if (bOnlyChaseWhenCarrying)
    {
        const UHumanFishingInventory* Inventory = OutUI ? OutUI->GetInventory() : nullptr;
        if (!Inventory || Inventory->GetCatches().Num() == 0)
        {
            return nullptr;
        }
    }
    return Fish;
}

void AHumanFishingShark::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    CooldownLeft = FMath::Max(0.f, CooldownLeft - DeltaSeconds);

    UHumanFishingPlayerUI* UI = nullptr;
    APawn* Target = FindTarget(UI);
    FHumanFishingSharkSenses Senses;
    Senses.bTargetAvailable = Target != nullptr;
    Senses.TargetDistance = Target ? FVector::Dist(Target->GetActorLocation(), GetActorLocation())
        : TNumericLimits<float>::Max();
    Senses.bCoolingDown = CooldownLeft > 0.f;

    // A heavier backpack is easier to notice and worth chasing harder.
    const UHumanFishingInventory* Inventory = UI ? UI->GetInventory() : nullptr;
    const float Load = Inventory ? Inventory->GetLoadFraction() : 0.f;

    bool bSteal = false;
    const EHumanFishingSharkState Next = ChooseState(State, Senses, GetRanges(Load), bSteal);
    if (bSteal)
    {
        StealFrom(UI);
    }
    else
    {
        SetState(Next, UI);
    }

    if (State == EHumanFishingSharkState::Chase && Target)
    {
        SwimToward(ClampToSwimArea(Target->GetActorLocation()),
            ScaleForLoad(ChaseSpeed, Load, FullLoadChaseSpeedBonus), ChaseTurnRate, DeltaSeconds);
        return;
    }

    PatrolPointTimeLeft -= DeltaSeconds;
    if (PatrolPointTimeLeft <= 0.f || FVector::Dist(GetActorLocation(), PatrolPoint) < ReachedPatrolPoint)
    {
        PickPatrolPoint(Target ? Target->GetActorLocation() : GetActorLocation());
    }
    SwimToward(PatrolPoint, PatrolSpeed, PatrolTurnRate, DeltaSeconds);
}

void AHumanFishingShark::SetState(EHumanFishingSharkState NewState, UHumanFishingPlayerUI* UI)
{
    if (NewState == State)
    {
        return;
    }
    const EHumanFishingSharkState Previous = State;
    State = NewState;
    if (UI)
    {
        if (NewState == EHumanFishingSharkState::Chase)
        {
            UI->ShowAlert(LOCTEXT("Chasing", "A shark is chasing you!"), 2.5f);
        }
        else if (Previous == EHumanFishingSharkState::Chase && NewState == EHumanFishingSharkState::Patrol)
        {
            UI->ShowAlert(LOCTEXT("Escaped", "You escaped the shark."), 2.f);
        }
    }
    OnSharkStateChanged(NewState);
}

void AHumanFishingShark::StealFrom(UHumanFishingPlayerUI* UI)
{
    // Swim away from where the bite happened, before a knockout moves the fish.
    const AActor* Fish = UI ? UI->GetOwner() : nullptr;
    PickPatrolPoint(Fish ? Fish->GetActorLocation() : GetActorLocation());
    CooldownLeft = StealCooldown;
    SetState(EHumanFishingSharkState::Retreat, nullptr);

    // Each bite takes the most valuable catch, so escaping early saves the rest.
    UHumanFishingInventory* Inventory = UI ? UI->GetInventory() : nullptr;
    const UHumanFishingCatalog* Catalog = UI ? UI->GetCatalog() : nullptr;
    FHumanFishingCatch Stolen;
    const bool bStole = Inventory && Inventory->StealMostValuableCatch(Catalog, Stolen);
    const bool bKnockedOut = UI && UI->ApplyAttackDamage(BiteDamage, HealingDelayAfterBite);

    if (UI)
    {
        const FHumanFishingCatchType* Profile = bStole && Catalog ? Catalog->Types.Find(Stolen.Type) : nullptr;
        const FText Name = Profile ? Profile->DisplayName : FText::FromName(Stolen.Type);
        FText Message;
        if (bStole)
        {
            Message = bKnockedOut
                ? FText::Format(LOCTEXT("StoleKnockedOut", "The shark stole your {0} and knocked you out!"), Name)
                : FText::Format(LOCTEXT("Stole", "The shark bit you and stole your {0}!"), Name);
        }
        else
        {
            Message = bKnockedOut ? LOCTEXT("KnockedOut", "The shark knocked you out!")
                : LOCTEXT("Bit", "The shark bit you!");
        }
        UI->ShowAlert(Message, 3.5f);
    }
    OnCatchesStolen(bStole ? 1 : 0);
}

void AHumanFishingShark::PickPatrolPoint(const FVector& AvoidLocation)
{
    // Try a few random points and keep the one farthest from whatever the shark should swim away from.
    float BestDistance = -1.f;
    for (int32 Attempt = 0; Attempt < 4; ++Attempt)
    {
        const FVector2D Flat = FMath::RandPointInCircle(PatrolRadius);
        const FVector Candidate = Home + FVector(Flat.X, Flat.Y, FMath::FRandRange(-VerticalRange, VerticalRange));
        const float Distance = FVector::DistSquared(Candidate, AvoidLocation);
        if (Distance > BestDistance)
        {
            BestDistance = Distance;
            PatrolPoint = Candidate;
        }
    }
    PatrolPointTimeLeft = PatrolPointTimeout;
}

FVector AHumanFishingShark::ClampToSwimArea(FVector Location) const
{
    Location.Z = FMath::Clamp(Location.Z, Home.Z - VerticalRange, Home.Z + VerticalRange);
    return Location;
}

void AHumanFishingShark::SwimToward(const FVector& Goal, float Speed, float TurnRate, float DeltaSeconds)
{
    const FVector ToGoal = Goal - GetActorLocation();
    if (ToGoal.IsNearlyZero() || DeltaSeconds <= 0.f)
    {
        return;
    }

    FRotator Desired = ToGoal.Rotation();
    Desired.Pitch = FMath::Clamp(Desired.Pitch, -MaxSwimPitch, MaxSwimPitch);
    Desired.Roll = 0.f;
    const FRotator Facing = FMath::RInterpConstantTo(GetActorRotation(), Desired, DeltaSeconds, TurnRate);
    SetActorRotation(Facing);

    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, Speed, DeltaSeconds, 2.f);
    const FVector Move = Facing.Vector() * CurrentSpeed * DeltaSeconds;
    FHitResult Hit;
    AddActorWorldOffset(Move, true, &Hit);
    if (Hit.bBlockingHit)
    {
        // Slide along rocks instead of stopping dead, and pick a new patrol point.
        AddActorWorldOffset(FVector::VectorPlaneProject(Move * (1.f - Hit.Time), Hit.Normal), true);
        PatrolPointTimeLeft = 0.f;
    }

    const FVector Location = GetActorLocation();
    const FVector Clamped = ClampToSwimArea(Location);
    if (!Clamped.Equals(Location))
    {
        SetActorLocation(Clamped);
    }
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs SpawnSharkCommand(
    TEXT("hf.shark"), TEXT("Spawn a shark ahead of the fish for testing: hf.shark [distance, default 2000]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        APawn* Fish = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
        if (!Fish)
        {
            return;
        }
        const float Distance = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 2000.f;
        FVector Ahead = Fish->GetViewRotation().Vector();
        Ahead.Z = 0.f;
        Ahead = Ahead.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
        const FVector Location = Fish->GetActorLocation() + Ahead * Distance;
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        World->SpawnActor<AHumanFishingShark>(Location, (-Ahead).Rotation(), Params);
    }));
#endif

#undef LOCTEXT_NAMESPACE
