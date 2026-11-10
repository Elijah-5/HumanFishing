using UnrealBuildTool;

public class HumanFishingGameplay : ModuleRules
{
    public HumanFishingGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "InputCore", "Slate", "SlateCore" });
    }
}
