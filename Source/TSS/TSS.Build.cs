using System.IO;
using UnrealBuildTool;

public class TSS : ModuleRules
{
    public TSS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        string PluginPath = Path.Combine(ModuleDirectory, "..", "..");

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "RenderCore",
                "RHI",
                "Projects"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                Path.Combine(PluginPath, "Shaders")
            }
        );
    }
}