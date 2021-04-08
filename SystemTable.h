#pragma once

#include "RuntimeInclude.h"
RUNTIME_MODIFIABLE_INCLUDE; //recompile runtime files when this changes

#include "DeviceResource.h"

struct RCCppMainLoopI;
struct ImGuiContext;
struct pDeviceResources;

struct SystemTable
{
    RCCppMainLoopI*             pRCCppMainLoopI = 0;
    ImGuiContext*               pImContext = 0;
    
    std::unique_ptr<DX::DeviceResources>        pDeviceResources = NULL;

};