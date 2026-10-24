using UnrealBuildTool;

public class CyberSiegeTarget : TargetRules
{
    public CyberSiegeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("CyberSiege");
    }
}
