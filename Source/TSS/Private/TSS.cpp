#include "TSS.h"
#include "Modules/ModuleManager.h"
#include "TSSViewExtension.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Misc/CoreDelegates.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FTSSModule"

int32 GTSSActive = 1;
int32 GTSSDebugMode = 0;

static FAutoConsoleVariableRef CVarTSSEnable(
    TEXT("tss.Enable"),
    GTSSActive,
    TEXT("Enable TSS upscaling. 0=off, 1=on"),
    ECVF_RenderThreadSafe
);

static FAutoConsoleVariableRef CVarTSSDebug(
    TEXT("tss.Debug"),
    GTSSDebugMode,
    TEXT("TSS debug mode. 0=normal, 1=show low-res, 2=show history, 3=show motion"),
    ECVF_RenderThreadSafe
);

static FAutoConsoleCommand CCmdTSSToggle(
    TEXT("tss.Toggle"),
    TEXT("Toggle TSS upscaling on/off"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        GTSSActive = GTSSActive ? 0 : 1;
        UE_LOG(LogTemp, Warning, TEXT("TSS: %s"), GTSSActive ? TEXT("ENABLED") : TEXT("DISABLED"));
    })
);

void FTSSModule::StartupModule()
{
    FString PluginShaderDir = FPaths::Combine(
        IPluginManager::Get().FindPlugin(TEXT("TSS"))->GetBaseDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/TSS"), PluginShaderDir);

    UE_LOG(LogTemp, Warning, TEXT("TSS: Shader dir mapped: %s"), *PluginShaderDir);

    FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTSSModule::InitializeForRendering);
}

void FTSSModule::InitializeForRendering()
{
    if (!bViewExtensionRegistered)
    {
        SceneViewExtension = FSceneViewExtensions::NewExtension<FTSSViewExtension>();
        bViewExtensionRegistered = true;
        UE_LOG(LogTemp, Warning, TEXT("TSS: ViewExtension registered"));

        if (FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().OnApplicationPreInputKeyDownListener().AddLambda(
                [](const FKeyEvent& KeyEvent)
                {
                    if (KeyEvent.GetKey() == EKeys::F)
                    {
                        GTSSActive = GTSSActive ? 0 : 1;
                        UE_LOG(LogTemp, Warning, TEXT("TSS: %s"), GTSSActive ? TEXT("ENABLED") : TEXT("DISABLED"));
                    }
                });
        }
    }
}

void FTSSModule::ShutdownModule()
{
    SceneViewExtension.Reset();
    UE_LOG(LogTemp, Warning, TEXT("TSS: Module unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTSSModule, TSS)
