using UnrealBuildTool;
using System.IO;

public class TSS : ModuleRules
{
    public TSS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "RHICore",
            "Projects",
            "InputCore",
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Renderer",
            "Slate",
            "SlateCore",
        });

        PrivateIncludePaths.AddRange(new[]
        {
            Path.Combine(ModuleDirectory, "..", "..", "Shaders", "Private"),
        });
    }
}
