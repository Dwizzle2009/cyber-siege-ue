using UnrealBuildTool;

public class CyberSiegeEditorTarget : TargetRules
{
    public CyberSiegeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("CyberSiege");
    }
}
