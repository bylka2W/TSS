#include "TSSShaders.h"

IMPLEMENT_GLOBAL_SHADER(FTSSLuminancePyramidCS, "/Plugin/TSS/Private/TSSLuminancePyramid.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FTSSReconstructDilateCS, "/Plugin/TSS/Private/TSSReconstructDilate.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FTSSDepthClipCS, "/Plugin/TSS/Private/TSSDepthClip.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FTSSLockCS, "/Plugin/TSS/Private/TSSLock.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FTSSAccumulateCS, "/Plugin/TSS/Private/TSSAccumulate.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FTSSRCASCS, "/Plugin/TSS/Private/TSSRCAS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FTSSBlitPS, "/Plugin/TSS/Private/TSSBlit.usf", "MainPS", SF_Pixel);
