using UnrealBuildTool;

public class IshibashiriPrototype : ModuleRules
{
    public IshibashiriPrototype(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "ControlRig", "MotionWarping" });
    }
}
