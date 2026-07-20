#pragma once

#include "Modules/ModuleManager.h"
#include "SceneViewExtension.h"

class FTSSViewExtension;

extern int32 GTSSActive;

class FTSSModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    void InitializeForRendering();

private:
    TSharedPtr<FTSSViewExtension, ESPMode::ThreadSafe> SceneViewExtension;
    bool bViewExtensionRegistered = false;
};
