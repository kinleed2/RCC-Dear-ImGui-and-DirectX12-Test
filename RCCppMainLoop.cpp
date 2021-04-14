#include "RCCppMainLoop.h"

#include "IObject.h"
#include "SystemTable.h"
#include "ISimpleSerializer.h"
#include "IRuntimeObjectSystem.h"
#include "IObjectFactorySystem.h"

#include "imgui.h"
#include "Common/DeviceResources.h"
#include "Common/StepTimer.h"
#include "Common/Camera.h"
#include "Common/d3dUtil.h"
#include "Scene/Scene.h"

#include <DirectXColors.h>
#include <memory>


// add imgui source dependencies
// an alternative is to put imgui into a library and use RuntimeLinkLibrary
#include "RuntimeSourceDependency.h"
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("imgui/imgui", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("imgui/imgui_widgets", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("imgui/imgui_draw", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("imgui/imgui_demo", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("imgui/imgui_tables", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("Common/DeviceResources", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("Scene/SceneManager", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("Common/d3dUtil", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("Common/Camera", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("Scene/SceneMain", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("Scene/SceneTitle", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("Scene/SceneLoad", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("ModelLoader/ModelLoader", ".cpp");
RUNTIME_COMPILER_SOURCEDEPENDENCY_FILE("FrameResource/FrameResource", ".cpp");

#include "RuntimeLinkLibrary.h"
RUNTIME_COMPILER_LINKLIBRARY("d3d12.lib");  
RUNTIME_COMPILER_LINKLIBRARY("dxgi.lib");
RUNTIME_COMPILER_LINKLIBRARY("dxguid.lib");
RUNTIME_COMPILER_LINKLIBRARY("DirectXTK12.lib");
RUNTIME_COMPILER_LINKLIBRARY("d3dcompiler.lib");
RUNTIME_COMPILER_LINKLIBRARY("assimp-vc142-mtd.lib");





// Rendering loop timer.
DX::StepTimer                        g_timer;

// RCC++ uses interface Id's to distinguish between different classes
// here we have only one, so we don't need a header for this enum and put it in the same
// source code file as the rest of the code
enum InterfaceIDEnumRCCppDX12Example
{
    IID_IRCCPP_MAIN_LOOP = IID_ENDInterfaceID, // IID_ENDInterfaceID from IObject.h InterfaceIDEnum

    IID_ENDInterfaceIDEnumRCCppDX12Example
};

struct RCCppMainLoop : RCCppMainLoopI, TInterface<IID_IRCCPP_MAIN_LOOP, IObject>
{

    // data for compiling window
    static constexpr double SHOW_AFTER_COMPILE_TIME = 3.0f;
    double compileStartTime = -SHOW_AFTER_COMPILE_TIME;
    double compileEndTime = -SHOW_AFTER_COMPILE_TIME;
    unsigned int compiledModules = 0;

    RCCppMainLoop()
    {
        g_pSys->pRCCppMainLoopI = this;
        g_pSys->pRuntimeObjectSystem->GetObjectFactorySystem()->SetObjectConstructorHistorySize(10);
        g_pSys->pRuntimeObjectSystem->AddLibraryDir("build/x64/Debug");
        g_pSys->pRuntimeObjectSystem->AddLibraryDir("Assimp/Libs");
        g_pSys->pRuntimeObjectSystem->AddIncludeDir("Assimp");
    
    }

    void Init(bool isFirstInit) override
    {
        // If you want to do any initialization which is expensive and done after state
        // has been serialized you can do this here.

        if (isFirstInit)
        {
            // do any init needed to be done only once here, isFirstInit only set
            // when object is first constructed at program start.
        }
        // can do any initialization you might want to change here.
    }

    void Serialize(ISimpleSerializer* pSerializer) override
    {
        SERIALIZE(compileStartTime);
        SERIALIZE(compileEndTime);
    }


    void MainLoop() override
    {
        SetUI();

        g_timer.Tick([&]()
        {
            Update(g_timer);
        });

        // Rendering
        ImGui::Render();

        Render();
    }

    bool CreateDeviceD3D(HWND hWnd, int width, int height)
    {
        g_pSys->pDeviceResources = std::make_unique<DX::DeviceResources>();

        g_pSys->pDeviceResources->SetWindow(hWnd, width, height);
        g_pSys->pDeviceResources->CreateDeviceResources();
        g_pSys->pDeviceResources->CreateWindowSizeDependentResources();

        // Descriptor Heap for Dear ImGui
        {
            g_pSys->pd3dSrvDescHeap = std::make_unique<DescriptorHeap>(g_pSys->pDeviceResources->GetD3DDevice(),
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                1);
      
        }
        return true;
    }

    void CleanupDeviceD3D() override
    {
        if (g_pSys->pDeviceResources) { g_pSys->pDeviceResources.reset();}
    }

    void Update(DX::StepTimer const& timer)
    {
        g_pSys->pSceneManager->Update(timer);
    }

    void Clear()
    {
        auto cmdList = g_pSys->pDeviceResources->GetCommandList();

        // Clear the views.
        auto rtvDescriptor = g_pSys->pDeviceResources->GetRenderTargetView();
        auto dsvDescriptor = g_pSys->pDeviceResources->GetDepthStencilView();

        cmdList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
        cmdList->ClearRenderTargetView(rtvDescriptor, DirectX::Colors::Aqua, 0, nullptr);
        cmdList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // Set the viewport and scissor rect.
        auto viewport = g_pSys->pDeviceResources->GetScreenViewport();
        auto scissorRect = g_pSys->pDeviceResources->GetScissorRect();
        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);
    }

    void Render()
    {
        // Don't try to render anything before the first Update.
        if (g_timer.GetFrameCount() == 0)
        {
            return;
        }

        // Prepare the command list to render a new frame.
        g_pSys->pDeviceResources->Prepare();
        Clear();
        
        // TODO: Add your rendering code here.


        auto cmdList = g_pSys->pDeviceResources->GetCommandList();
        ID3D12DescriptorHeap* heaps[] = { g_pSys->pd3dSrvDescHeap->Heap() };
        cmdList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

        g_pSys->ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

        g_pSys->pSceneManager->Render();

        // Show the new frame.
        g_pSys->pDeviceResources->Present();
    }

    void SetUI()
    {
        ImGui::SetCurrentContext(g_pSys->pImContext);

        ImGui::NewFrame();

        // Show compiling info
        double time = ImGui::GetTime();
        bool bCompiling = g_pSys->pRuntimeObjectSystem->GetIsCompiling();
        double timeSinceLastCompile = time - compileEndTime;
        if (bCompiling || timeSinceLastCompile < SHOW_AFTER_COMPILE_TIME)
        {
            if (bCompiling)
            {
                if (timeSinceLastCompile > SHOW_AFTER_COMPILE_TIME)
                {
                    compileStartTime = time;
                }
                compileEndTime = time; // ensure always updated
            }
            bool bCompileOk = g_pSys->pRuntimeObjectSystem->GetLastLoadModuleSuccess();

            ImVec4 windowBgCol = ImVec4(0.1f, 0.4f, 0.1f, 1.0f);
            if (!bCompiling)
            {
                if (bCompileOk)
                {
                    windowBgCol = ImVec4(0.1f, 0.7f, 0.1f, 1.0f);
                }
                else
                {
                    windowBgCol = ImVec4(0.7f, 0.1f, 0.1f, 1.0f);
                }
            }
            ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgCol);

            ImVec2 sizeAppWindow = ImGui::GetIO().DisplaySize;
            ImGui::SetNextWindowPos(ImVec2(sizeAppWindow.x - 300, sizeAppWindow.y - 50), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Always);
            ImGui::Begin("Compiling", NULL, ImGuiWindowFlags_NoTitleBar);
            if (bCompiling)
            {
                ImGui::Text("Compiling... time %.2fs", (float)(time - compileStartTime));
            }
            else
            {
                if (bCompileOk)
                {
                    ImGui::Text("Compiling... time %.2fs. SUCCEED", (float)(compileEndTime - compileStartTime));
                }
                else
                {
                    ImGui::Text("Compiling... time %.2fs. FAILED", (float)(compileEndTime - compileStartTime));
                }
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        // Developer tools window
        bool doRCCppUndo = false;
        bool doRCCppRedo = false;

        ImVec2 sizeAppWindow = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(200, 0), ImGuiCond_Always);

        ImGui::Begin("RCC++ Developer Tools");
        {
            bool bAutoCompile = g_pSys->pRuntimeObjectSystem->GetAutoCompile();
            if (ImGui::Checkbox("Auto Compile", &bAutoCompile))
            {
                g_pSys->pRuntimeObjectSystem->SetAutoCompile(bAutoCompile);
            } if (ImGui::IsItemHovered()) ImGui::SetTooltip("Compilation is triggered by saving a runtime compiled file.");

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("Optimization"); ImGui::Spacing();
            int optLevel = g_pSys->pRuntimeObjectSystem->GetOptimizationLevel();
            ImGui::RadioButton("Default", &optLevel, 0);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("RCCPPOPTIMIZATIONLEVEL_DEBUG in DEBUG, RCCPPOPTIMIZATIONLEVEL_PERF in RELEASE.\nThis is the default state.");
            ImGui::RadioButton("Debug", &optLevel, 1);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("RCCPPOPTIMIZATIONLEVEL_DEBUG\nDefault in DEBUG.\nLow optimization, improve debug experiece.");
            ImGui::RadioButton("Performance", &optLevel, 2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("RCCPPOPTIMIZATIONLEVEL_PERF\nDefaul in RELEASE.\nOptimization for performance, debug experience may suffer.");
            ImGui::RadioButton("Not Set", &optLevel, 3);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("No optimization set in compile, soeither underlying compiler default or set through SetAdditionalCompileOptions.");
            g_pSys->pRuntimeObjectSystem->SetOptimizationLevel((RCppOptimizationLevel)optLevel);

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Clean"))
            {
                g_pSys->pRuntimeObjectSystem->CleanObjectFiles();
            } if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove all compiled intermediates.");

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Undo"))
            {
                doRCCppUndo = true;
            } if (ImGui::IsItemHovered()) ImGui::SetTooltip("Undo the last save."); ImGui::SameLine();
            if (ImGui::Button("Redo"))
            {
                doRCCppRedo = true;
            } if (ImGui::IsItemHovered()) ImGui::SetTooltip("Redo the last save.");
        }
        ImGui::End();

    }
};

REGISTERSINGLETON(RCCppMainLoop, true);