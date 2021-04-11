#pragma once

#include "RuntimeInclude.h"
RUNTIME_MODIFIABLE_INCLUDE; //recompile runtime files when this changes

#include "ObjectInterfacePerModule.h"
#include "Common/DeviceResources.h"
#include "Scene/Scene.h"
#include "DirectXTK12/Inc/DescriptorHeap.h"

struct RCCppMainLoopI;
struct ImGuiContext;
struct pDeviceResources;
struct pd3dSrvDescHeap;

struct pSceneManager;

struct IRuntimeObjectSystem;
struct ICompilerLogger;

struct ImDrawData;
struct ID3D12GraphicsCommandList;
typedef void (*ImGui_ImplDX12_RenderDrawDataFunc)(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx);

static SystemTable*& g_pSys = PerModuleInterface::g_pSystemTable;

struct SystemTable
{
    RCCppMainLoopI*             pRCCppMainLoopI = 0;
    ImGuiContext*               pImContext = 0;
    
    std::unique_ptr<DX::DeviceResources>            pDeviceResources = NULL;
    std::unique_ptr<DescriptorHeap>                 pd3dSrvDescHeap = NULL;

    ImGui_ImplDX12_RenderDrawDataFunc ImGui_ImplDX12_RenderDrawData = NULL;
    
    std::unique_ptr<SceneManager>                   pSceneManager = NULL;

    IRuntimeObjectSystem* pRuntimeObjectSystem = NULL;
    ICompilerLogger* pLogger = NULL;
};