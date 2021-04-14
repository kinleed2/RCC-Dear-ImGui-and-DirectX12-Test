#pragma once

#include <unordered_map>
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

#include "imgui.h"
#include "../Common/d3dUtil.h"
#include "../Common/Camera.h"
#include "../Common/StepTimer.h"
#include "../FrameResource/FrameResource.h"
#include "../DirectXTK12/Inc/DescriptorHeap.h"
#include "../TextureRender/ShadowMap.h"


using Microsoft::WRL::ComPtr;

class Scene
{

public:
	Scene() {}
	virtual ~Scene() {}
	virtual void Initialize() = 0;
	virtual void Update(DX::StepTimer const& timer) = 0;
	virtual void Render() = 0;

private:
	friend class SceneLoad;
	friend class SceneManager;
	bool initialized = false;



};

class SceneManager
{
private:
    std::unique_ptr<Scene> currentScene;

public:
    SceneManager() {};
    ~SceneManager() {};
    SceneManager(SceneManager&&) = default;
    SceneManager& operator= (SceneManager&&) = default;
    SceneManager(SceneManager const&) = delete;
    SceneManager& operator= (SceneManager const&) = delete;

    void Update(DX::StepTimer const& timer);
    void Render();
    void ChangeScene(Scene* new_scene);
};

class SceneLoad : public Scene
{
private:

    std::unique_ptr<Scene> nextScene;

public:
    SceneLoad(Scene* nextScene)
    {
        this->nextScene.reset(nextScene);
    }
    virtual ~SceneLoad() {}
    void Initialize() override;

    void Update(DX::StepTimer const& timer) override;
    void Render() override;
private:
    static void LoadingThread(SceneLoad* scene);

    int count = 0;

};

class SceneTitle : public Scene
{
public:
    SceneTitle() {}
    ~SceneTitle() {}
    void Initialize();
    void Update(DX::StepTimer const& timer);
    void Render();

private:
    // UI state
    bool show_demo_window = true;
    bool show_another_window = false;

};

class SceneMain : public Scene
{
public:
    SceneMain() {}
    ~SceneMain() {}
    void Initialize();
    void Update(DX::StepTimer const& timer);
    void Render();

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unique_ptr<DescriptorHeap> m_resourceDescriptors;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, UINT> mTexturesHeapIndex;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;

    // Render items divided by PSO.
    std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

    UINT mSkyTexHeapIndex = 0;
    UINT mShadowMapHeapIndex = 0;

    UINT mNullCubeSrvIndex = 0;
    UINT mNullTexSrvIndex = 0;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

    PassConstants mMainPassCB;  // index 0 of pass cbuffer.
    PassConstants mShadowPassCB;// index 1 of pass cbuffer.

    Camera mCamera;

    std::unique_ptr<ShadowMap> mShadowMap;

    DirectX::BoundingSphere mSceneBounds;

    float mLightNearZ = 0.0f;
    float mLightFarZ = 0.0f;
    Vector3 mLightPosW;
    Matrix mLightView = Matrix::Identity;
    Matrix mLightProj = Matrix::Identity;
    Matrix mShadowTransform = Matrix::Identity;

    float mLightRotationAngle = 0.0f;
    Vector3 mBaseLightDirections[3] = {
        Vector3(0.57735f, -0.57735f, 0.57735f),
        Vector3(-0.57735f, -0.57735f, 0.57735f),
        Vector3(0.0f, -0.707f, -0.707f)
    };
    Vector3 mRotatedLightDirections[3];


};