using UnrealBuildTool;

public class IshibashiriPrototypeTarget : TargetRules
{
    public IshibashiriPrototypeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("IshibashiriPrototype");
    }
}
