using UnrealBuildTool;

public class IshibashiriPrototypeEditorTarget : TargetRules
{
    public IshibashiriPrototypeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("IshibashiriPrototype");
    }
}
