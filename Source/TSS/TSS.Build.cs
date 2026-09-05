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
                "Core",
                "CoreUObject",
                "Engine"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "RenderCore",
                "RHI",
                "Renderer",
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