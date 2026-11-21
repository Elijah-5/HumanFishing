#include "HumanFishingAmbientFish.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

AHumanFishingAmbientFish::AHumanFishingAmbientFish()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickInterval=1.f/30.f;
    Visual=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FishVisual"));RootComponent=Visual;
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);Visual->SetCanEverAffectNavigation(false);
    Visual->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Visual->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    Visual->bEnableUpdateRateOptimizations=true;Visual->SetCullDistance(5000.f);
}
void AHumanFishingAmbientFish::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if(FishMesh)
    {
        Visual->SetSkeletalMesh(FishMesh);
        const FVector Size=FishMesh->GetBounds().BoxExtent*2.;
        Visual->SetRelativeScale3D(FVector(FishLength/FMath::Max(Size.X,Size.Y)));
        if(SwimAnimation) Visual->PlayAnimation(SwimAnimation,true);
    }
}
void AHumanFishingAmbientFish::BeginPlay()
{
    Super::BeginPlay();Home=GetActorLocation();Phase=(GetUniqueID()%360)*PI/180.f;
    if(SwimAnimation) Visual->PlayAnimation(SwimAnimation,true);
}
void AHumanFishingAmbientFish::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);Phase+=DeltaSeconds*SwimSpeed/FMath::Max(SwimRadius,1.f);
    SetActorLocation(Home+FVector(FMath::Cos(Phase)*SwimRadius,FMath::Sin(Phase)*SwimRadius,FMath::Sin(Phase*1.7f)*35.f));
    SetActorRotation(FRotator(0,FMath::RadiansToDegrees(Phase)+90.f,0));
}
