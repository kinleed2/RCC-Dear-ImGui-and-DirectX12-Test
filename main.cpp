// Dear ImGui: standalone example application for DirectX 12
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important: to compile on 32-bit systems, the DirectX12 backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// This define is set in the example .vcxproj file and need to be replicated in your app or by adding it to your imconfig.h file.

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <tchar.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include "RuntimeObjectSystem.h"

// headers from our example 
#include "StdioLogSystem.h"
#include "SystemTable.h"
#include "RCCppMainLoop.h"

#include <DirectXColors.h>
#include "DeviceResource.h"
#include "StepTimer.h"
using namespace DirectX;


// RCC++ Data
static IRuntimeObjectSystem* g_pRuntimeObjectSystem;
static StdioLogSystem        g_Logger;
static SystemTable           g_systemTable;


// DX12 Data
std::unique_ptr<DX::DeviceResources> g_pDeviceResources = NULL;
ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
// Rendering loop timer.
DX::StepTimer                        g_timer;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// forward declaractions of RCC++
bool RCCppInit();
void RCCppCleanup();
void RCCppUpdate();

// Game method
void Clear();
void Render();

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(
        wc.lpszClassName, 
        _T("Dear ImGui DirectX12 Example"), 
        WS_OVERLAPPEDWINDOW, 
        100, 100, 
        1280, 800, 
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd)
        return 1;

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        //CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Initialize RCC++
    RCCppInit();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    g_systemTable.pImContext = ImGui::GetCurrentContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(g_pDeviceResources->GetD3DDevice(), 2,
        DXGI_FORMAT_B8G8R8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        RCCppUpdate();

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        
        g_systemTable.pRCCppMainLoopI->MainLoop();

        g_timer.Tick([&]()
        {
            
        });
        
        Render();
    }

    g_pDeviceResources->WaitForGpu();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    RCCppCleanup();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    g_pDeviceResources = std::make_unique<DX::DeviceResources>();

    g_pDeviceResources->SetWindow(hWnd, 1280, 800);
    g_pDeviceResources->CreateDeviceResources();
    g_pDeviceResources->CreateWindowSizeDependentResources();

    {
        
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pDeviceResources->GetD3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    return true;
}

void CleanupDeviceD3D()
{
    
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = NULL; }


#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = NULL;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pDeviceResources != NULL && wParam != SIZE_MINIMIZED)
        {
            //WaitForLastSubmittedFrame();
            ImGui_ImplDX12_InvalidateDeviceObjects();
            g_pDeviceResources->WindowSizeChanged((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
            //CleanupRenderTarget();
            //ResizeSwapChain(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
            //CreateRenderTarget();
            ImGui_ImplDX12_CreateDeviceObjects();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool RCCppInit()
{
    g_pRuntimeObjectSystem = new RuntimeObjectSystem;
    if (!g_pRuntimeObjectSystem->Initialise(&g_Logger, &g_systemTable))
    {
        //delete g_pRuntimeObjectSystem;
        g_pRuntimeObjectSystem = NULL;
        return false;
    }

    // ensure include directories are set - use location of this file as starting point
    FileSystemUtils::Path basePath = g_pRuntimeObjectSystem->FindFile(__FILE__).ParentPath();
    FileSystemUtils::Path imguiIncludeDir = basePath / "imgui";
    g_pRuntimeObjectSystem->AddIncludeDir(imguiIncludeDir.c_str());

    return true;
}

void RCCppCleanup()
{
    delete g_pRuntimeObjectSystem;
}

void RCCppUpdate()
{
    //check status of any compile
    if (g_pRuntimeObjectSystem->GetIsCompiledComplete())
    {
        
        // load module when compile complete
        g_pRuntimeObjectSystem->LoadCompiledModule();

    }

    if (!g_pRuntimeObjectSystem->GetIsCompiling())
    {
        float deltaTime = 1.0f / ImGui::GetIO().Framerate;
        g_pRuntimeObjectSystem->GetFileChangeNotifier()->Update(deltaTime);
    }
}

void Clear()
{
    auto commandList = g_pDeviceResources->GetCommandList();

    // Clear the views.
    auto rtvDescriptor = g_pDeviceResources->GetRenderTargetView();
    auto dsvDescriptor = g_pDeviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = g_pDeviceResources->GetScreenViewport();
    auto scissorRect = g_pDeviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

void Render()
{

    // Don't try to render anything before the first Update.
    if (g_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    g_pDeviceResources->Prepare();
    Clear();
    
    auto commandList = g_pDeviceResources->GetCommandList();
    commandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);


    // TODO: Add your rendering code here.
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);


    // Show the new frame.
    g_pDeviceResources->Present();

}
