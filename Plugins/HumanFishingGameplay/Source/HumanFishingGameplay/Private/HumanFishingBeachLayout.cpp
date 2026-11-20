#include "HumanFishingBeachLayout.h"

#include "HumanFishingBeachCrowd.h"
#include "PrimitiveSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"

#if WITH_EDITOR
namespace
{
using FLine = UHumanFishingBeachLayoutComponent::FLine;

// Raised above the top of the Beach bounds so the lines don't z-fight with the ground.
constexpr double LayoutHeight = 8.;
// Screen pixels, so the lines stay readable from far away.
constexpr float LineThickness = 3.f;

void AddRectangle(TArray<FLine>& Lines, const FBox2D& Area, double Z, const FLinearColor& Color)
{
    const FVector Corners[] = {FVector(Area.Min.X, Area.Min.Y, Z), FVector(Area.Max.X, Area.Min.Y, Z),
        FVector(Area.Max.X, Area.Max.Y, Z), FVector(Area.Min.X, Area.Max.Y, Z)};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Corners); ++Index)
    {
        Lines.Add({Corners[Index], Corners[(Index + 1) % UE_ARRAY_COUNT(Corners)], Color});
    }
}

class FBeachLayoutSceneProxy final : public FPrimitiveSceneProxy
{
public:
    FBeachLayoutSceneProxy(const UPrimitiveComponent* Component, const TArray<FLine>& InLines)
        : FPrimitiveSceneProxy(Component), Lines(InLines)
    {
    }

    virtual SIZE_T GetTypeHash() const override
    {
        static size_t UniquePointer;
        return reinterpret_cast<size_t>(&UniquePointer);
    }

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily,
        uint32 VisibilityMap, FMeshElementCollector& Collector) const override
    {
        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
        {
            if (VisibilityMap & (1 << ViewIndex))
            {
                FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
                for (const FLine& Line : Lines)
                {
                    PDI->DrawLine(Line.Start, Line.End, Line.Color, SDPG_World, LineThickness, 0.f, true);
                }
            }
        }
    }

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
    {
        FPrimitiveViewRelevance Result;
        Result.bDrawRelevance = IsShown(View);
        Result.bDynamicRelevance = true;
        Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
        return Result;
    }

    virtual uint32 GetMemoryFootprint() const override
    {
        return static_cast<uint32>(sizeof(*this) + GetAllocatedSize() + Lines.GetAllocatedSize());
    }

private:
    TArray<FLine> Lines;
};
}
#endif

UHumanFishingBeachLayoutComponent::UHumanFishingBeachLayoutComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bTickInEditor = true;
    bHiddenInGame = true;
    bUseEditorCompositing = true;
    CastShadow = false;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
}

#if WITH_EDITOR
void UHumanFishingBeachLayoutComponent::BeginPlay()
{
    Super::BeginPlay();
    // Play worlds never show the layout, so only the editor world refreshes it.
    SetComponentTickEnabled(false);
}

void UHumanFishingBeachLayoutComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Polled so the lines follow the Beach actor while it is moved, scaled, or reassigned.
    const auto* Crowd = Cast<AHumanFishingBeachCrowd>(GetOwner());
    FBox Bounds(ForceInit);
    if (Crowd && Crowd->bShowBeachLayout && IsValid(Crowd->Beach))
    {
        Bounds = Crowd->Beach->GetComponentsBoundingBox();
    }
    if (Bounds.IsValid == DrawnBounds.IsValid && (!Bounds.IsValid || Bounds == DrawnBounds))
    {
        return;
    }

    DrawnBounds = Bounds;
    Lines.Reset();
    if (Bounds.IsValid)
    {
        const FHumanFishingBeachLayout Layout = AHumanFishingBeachCrowd::GetBeachLayout(Bounds);
        const double Z = Bounds.Max.Z + LayoutHeight;
        AddRectangle(Lines, FBox2D(FVector2D(Bounds.Min.X, Bounds.Min.Y), FVector2D(Bounds.Max.X, Bounds.Max.Y)), Z,
            FColor::Cyan);
        AddRectangle(Lines, Layout.Roaming, Z, FColor::Green);
        for (const double Y : {Layout.NorthY, Layout.SouthY})
        {
            Lines.Add({FVector(Layout.EntryMinX, Y, Z), FVector(Layout.EntryMaxX, Y, Z), FColor::Orange});
        }
    }
    UpdateBounds();
    MarkRenderStateDirty();
}

FPrimitiveSceneProxy* UHumanFishingBeachLayoutComponent::CreateSceneProxy()
{
    return Lines.IsEmpty() ? nullptr : new FBeachLayoutSceneProxy(this, Lines);
}

FBoxSphereBounds UHumanFishingBeachLayoutComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    // The lines are in world space, independent of the component transform.
    FBox Box(ForceInit);
    for (const FLine& Line : Lines)
    {
        Box += Line.Start;
        Box += Line.End;
    }
    return Box.IsValid ? FBoxSphereBounds(Box.ExpandBy(1.)) : Super::CalcBounds(LocalToWorld);
}
#endif
