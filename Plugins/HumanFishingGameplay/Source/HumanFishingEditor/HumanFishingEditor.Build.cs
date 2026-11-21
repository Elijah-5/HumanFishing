using UnrealBuildTool;
public class HumanFishingEditor : ModuleRules
{
    public HumanFishingEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "HumanFishingGameplay" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "Kismet", "KismetCompiler", "BlueprintGraph", "Json", "JsonUtilities", "AssetRegistry", "UMG", "UMGEditor" });
    }
}
