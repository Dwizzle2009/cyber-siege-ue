using UnrealBuildTool;

public class CyberSiege : ModuleRules
{
    public CyberSiege(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        { "AeonSDK",
            "Core", "CoreUObject", "Engine", "InputCore",
            "EnhancedInput",
            "AIModule",
            "HTTP", "Json", "JsonUtilities"
        });
    }
}
