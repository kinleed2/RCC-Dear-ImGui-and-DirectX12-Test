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

#pragma comment(lib, "windowsapp")


#include "RuntimeObjectSystem.h"

// headers from our example 
#include "StdioLogSystem.h"
#include "SystemTable.h"
#include "RCCppMainLoop.h"
#include "Scene/Scene.h"
#include "GraphicsMemory.h"


// RCC++ Data
static StdioLogSystem        g_Logger;
static SystemTable           g_systemTable;

// Graphics
std::unique_ptr<DirectX::GraphicsMemory> g_graphicsMemory;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// forward declaractions of RCC++
bool RCCppInit();
void RCCppUpdate();
void RCCppCleanup();

#include "ModelLoader/PMDLoader.h"
// Main code
int main(int, char**)
{
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize))
        return 1;

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);

    int width = 1280, height = 720;

    HWND hwnd = ::CreateWindow(
        wc.lpszClassName, 
        _T("RCC++ Dear ImGui and DirectX12 Test"), 
        WS_OVERLAPPEDWINDOW, 
        100, 100, 
        width, height,
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd)
        return 1;

    // Initialize RCC++
    RCCppInit();

    // Initialize Direct3D
    if (!g_systemTable.pRCCppMainLoopI->CreateDeviceD3D(hwnd, width, height))
    {
        //CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }


    g_graphicsMemory = std::make_unique<DirectX::GraphicsMemory>(g_systemTable.pDeviceResources->GetD3DDevice());
    g_systemTable.pSceneManager = std::make_unique<SceneManager>();
    g_systemTable.pSceneManager->ChangeScene(new SceneTitle());

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(g_systemTable.pDeviceResources->GetD3DDevice(), 2,
        g_systemTable.pDeviceResources->GetBackBufferFormat(), g_systemTable.pd3dSrvDescHeap->Heap(),
        g_systemTable.pd3dSrvDescHeap->GetFirstCpuHandle(),
        g_systemTable.pd3dSrvDescHeap->GetFirstGpuHandle());

    g_systemTable.pImContext = ImGui::GetCurrentContext();
    g_systemTable.ImGui_ImplDX12_RenderDrawData = ImGui_ImplDX12_RenderDrawData;

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

        g_graphicsMemory->Commit(g_systemTable.pDeviceResources->GetCommandQueue());
    }

    // Cleanup
    RCCppCleanup();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_systemTable.pRCCppMainLoopI->CleanupDeviceD3D();
    g_graphicsMemory.reset();

    #ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = NULL;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
    #endif

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
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
        if (g_systemTable.pDeviceResources != NULL && wParam != SIZE_MINIMIZED)
        {
            ImGui_ImplDX12_InvalidateDeviceObjects();
            g_systemTable.pDeviceResources->WindowSizeChanged((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
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
    g_systemTable.pRuntimeObjectSystem = new RuntimeObjectSystem;
    g_systemTable.pLogger = &g_Logger;
    if (!g_systemTable.pRuntimeObjectSystem->Initialise(&g_Logger, &g_systemTable))
    {
        delete g_systemTable.pRuntimeObjectSystem;
        g_systemTable.pRuntimeObjectSystem = NULL;
        return false;
    }

    // ensure include directories are set - use location of this file as starting point
    FileSystemUtils::Path basePath = g_systemTable.pRuntimeObjectSystem->FindFile(__FILE__).ParentPath();
    FileSystemUtils::Path imguiIncludeDir = basePath / "imgui";
    g_systemTable.pRuntimeObjectSystem->AddIncludeDir(imguiIncludeDir.c_str());

    return true;
}

void RCCppCleanup()
{
    delete g_systemTable.pRuntimeObjectSystem;
}


void RCCppUpdate()
{
    //check status of any compile
    if (g_systemTable.pRuntimeObjectSystem->GetIsCompiledComplete())
    {
        
        // load module when compile complete
        g_systemTable.pRuntimeObjectSystem->LoadCompiledModule();

    }

    if (!g_systemTable.pRuntimeObjectSystem->GetIsCompiling())
    {
        float deltaTime = 1.0f / ImGui::GetIO().Framerate;
        g_systemTable.pRuntimeObjectSystem->GetFileChangeNotifier()->Update(deltaTime);
    }
}

