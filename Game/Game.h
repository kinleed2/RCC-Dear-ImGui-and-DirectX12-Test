//#pragma once
//
//#include "DeviceResources.h"
//#include "StepTimer.h"
//#include "GraphicsMemory.h"
//
//// A basic game implementation that creates a D3D12 device and
//// provides a game loop.
//class Game final : public DX::IDeviceNotify
//{
//public:
//
//    Game() noexcept(false);
//    ~Game();
//
//    Game(Game&&) = default;
//    Game& operator= (Game&&) = default;
//
//    Game(Game const&) = delete;
//    Game& operator= (Game const&) = delete;
//
//    // Initialization and management
//    void Initialize(HWND window, int width, int height);
//
//    // Basic game loop
//    void Tick();
//
//    // IDeviceNotify
//    void OnDeviceLost() override;
//    void OnDeviceRestored() override;
//
//    // Messages
//    void OnActivated();
//    void OnDeactivated();
//    void OnSuspending();
//    void OnResuming();
//    void OnWindowMoved();
//    void OnWindowSizeChanged(int width, int height);
//
//    // Properties
//    void GetDefaultSize(int& width, int& height) const noexcept;
//
//public:
//    static const UINT FrameCount = 3;
//
//private:
//
//    void Update(DX::StepTimer const& timer);
//    void Render();
//
//    void Clear();
//
//    void CreateDeviceDependentResources();
//
//    void CreateWindowSizeDependentResources();
//
//    // Device resources.
//    DX::DeviceResources* m_deviceResources;
//
//    // Rendering loop timer.
//    DX::StepTimer m_timer;
//
//    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
//
//    ID3D12DebugDevice* debugInterface;
//
//};