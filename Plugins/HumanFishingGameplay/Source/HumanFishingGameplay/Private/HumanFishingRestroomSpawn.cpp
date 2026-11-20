#include "HumanFishingBeachCrowd.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"

ACharacter* AHumanFishingBeachCrowd::SpawnRestroomVisitor(FVector ExitLocation, FRotator ExitRotation)
{
    UWorld* World = GetWorld();
    if (!World || !HasActorBegunPlay() || !BeachBounds.IsValid ||
        !PedestrianClass || !IsValid(Catalog) || Visitors.Num() >= MaxPedestrians ||
        ExitLocation.ContainsNaN() || ExitRotation.ContainsNaN())
    {
        return nullptr;
    }

    // The marker represents the capsule centre, but SpawnVisitor expects a floor point.
    ExitLocation.Z = BeachBounds.Max.Z;
    FVector Entry;
    if (!ProjectToBeach(ExitLocation, Entry))
    {
        return nullptr;
    }

    const double Now = World->GetTimeSeconds();
    const bool bNorth = Entry.Y > BeachBounds.GetCenter().Y;
    if (!SpawnVisitor(Now, bNorth, ChooseType(), Entry))
    {
        return nullptr;
    }

    // Reuse the existing catalog, accessories, controller and visitor registration.
    // The Entering phase now takes this visitor toward the normal wandering area.
    ACharacter* Pawn = Visitors.Last().Pawn.Get();
    if (Pawn)
    {
        Pawn->SetActorRotation(FRotator(0.f, ExitRotation.Yaw, 0.f));
    }
    return Pawn;
}
