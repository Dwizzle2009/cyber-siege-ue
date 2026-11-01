using UnrealBuildTool;

public class CyberSiege : ModuleRules
{
    public CyberSiege(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(ModuleDirectory);   // "Mission/CSMissionDirector.h" style includes from anywhere in the module

        PublicDependencyModuleNames.AddRange(new string[]
        { "AeonSDK",
            "Core", "CoreUObject", "Engine", "InputCore",
            "EnhancedInput",
            "AIModule", "NavigationSystem",
            "HTTP", "Json", "JsonUtilities"
        });
    }
}
