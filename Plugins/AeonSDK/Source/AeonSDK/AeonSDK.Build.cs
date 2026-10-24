using System.IO;
using UnrealBuildTool;

public class AeonSDK : ModuleRules
{
    public AeonSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

        string ThirdParty = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "libaeon");
        PublicIncludePaths.Add(ThirdParty);

        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            // prebuilt on the Aeon dev box: sdk/libaeon -> make
            string Lib = Path.Combine(ThirdParty, "Linux", "x86_64", "libaeon.so");
            PublicAdditionalLibraries.Add(Lib);
            RuntimeDependencies.Add("$(BinaryOutputDir)/libaeon.so", Lib);
            PublicDefinitions.Add("AEON_SDK_AVAILABLE=1");
        }
        else
        {
            PublicDefinitions.Add("AEON_SDK_AVAILABLE=0");   // editor / other platforms: every call is a no-op
        }
    }
}
