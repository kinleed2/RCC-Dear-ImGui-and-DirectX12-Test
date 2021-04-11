#pragma once

#include "RuntimeInclude.h"
RUNTIME_MODIFIABLE_INCLUDE; //recompile runtime files when this changes
#include <Windows.h>

// abstract interface to our RCCppMainLoop class, using I at end to denote Interface
struct RCCppMainLoopI
{
    virtual void MainLoop() = 0;
    virtual bool CreateDeviceD3D(HWND hWnd, int width, int height) = 0;
    virtual void CleanupDeviceD3D() = 0;
};