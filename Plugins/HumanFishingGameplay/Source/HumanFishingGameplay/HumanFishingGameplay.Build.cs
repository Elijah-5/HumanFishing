using UnrealBuildTool;

public class HumanFishingGameplay : ModuleRules
{
    public HumanFishingGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "InputCore", "Slate", "SlateCore", "AIModule", "NavigationSystem", "AnimGraphRuntime", "ApplicationCore" });
        if (Target.Platform == UnrealTargetPlatform.Mac) PublicFrameworks.AddRange(new[] { "Cocoa", "QuartzCore" });
    }
}
