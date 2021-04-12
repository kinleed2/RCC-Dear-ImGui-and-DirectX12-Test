#include "Scene.h"
#include "../SystemTable.h"

#include "../DirectXTK12/Inc/GeometricPrimitive.h"
#include "../DirectXTK12/Inc/DDSTextureLoader.h"
#include "../DirectXTK12/Inc/EffectPipelineStateDescription.h"
#include "../DirectXTK12/Inc/CommonStates.h"



using namespace DirectX;
using namespace DX;

void SceneMain::Initialize()
{
    // Estimate the scene bounding sphere manually since we know how the scene was constructed.
    // The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
    // the world space origin.  In general, you need to loop over every world space vertex
    // position and compute the bounding sphere.
    mSceneBounds.Center = Vector3(0.0f, 0.0f, 0.0f);
    mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

    // Set camera
    mCamera.SetPosition({ 0.0f, 2.0f, -15.0f });

    auto devRes = g_pSys->pDeviceResources.get();

    // Set Mesh
    std::vector<VertexPositionNormalTexture> vertices;
    std::vector<uint16_t> indices;

    GeometricPrimitive::CreateBox(vertices, indices,
        (Vector3(2,2,2)));

    auto boxMesh = make_unique<MeshGeometry>();
    boxMesh->Set(devRes, "box", vertices, indices);
    mGeometries["box"] = std::move(boxMesh);
    

    // Load textures
    {
        auto texture = make_unique<Texture>();

        ResourceUploadBatch upload(devRes->GetD3DDevice());

        upload.Begin();

        DX::ThrowIfFailed((CreateDDSTextureFromFile(devRes->GetD3DDevice(),
            upload, L"Assets\\Textures\\water1.dds", texture->Resource.ReleaseAndGetAddressOf())));

        mTextures["water"] = std::move(texture);

        // Upload the resources to the GPU.
        auto cmdQueue = devRes->GetCommandQueue();
        auto finish = upload.End(cmdQueue);

        // Wait for the upload thread to terminate
        finish.wait();
    }
    // Create root signatures
    {
        CD3DX12_DESCRIPTOR_RANGE cubeAndShadow(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 + 1 /* cube map + shadow map */, 0);
        CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mTextures.size(), 2);
        CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 7, 0);

        // Root parameter can be a table, root descriptor or root constants.
        CD3DX12_ROOT_PARAMETER rootParameters[5];

        // Perfomance TIP: Order from most frequent to least frequent.
        rootParameters[0].InitAsConstantBufferView(0);
        rootParameters[1].InitAsConstantBufferView(1);
        rootParameters[2].InitAsShaderResourceView(0, 1);
        rootParameters[3].InitAsDescriptorTable(1, &cubeAndShadow, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[3].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[4].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);

        // A root signature is an array of root parameters.
        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(std::size(rootParameters), rootParameters,
            0, nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
        ComPtr<ID3DBlob> serializedRootSig = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

        if (errorBlob != nullptr)
        {
            ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        ThrowIfFailed(devRes->GetD3DDevice()->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(mRootSignature.GetAddressOf())));
    }
    // Create PSO
    {
        const D3D_SHADER_MACRO alphaTestDefines[] =
        {
            "ALPHA_TEST", "1",
            NULL, NULL
        };

        mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
        mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

        mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
        mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
        mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");

        mShaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
        mShaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

        mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
        mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

        mInputLayout =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

        //
        // PSO for opaque objects.
        //
        RenderTargetState rtState(devRes->GetBackBufferFormat(),
            devRes->GetDepthBufferFormat());

        EffectPipelineStateDescription pd(
            &VertexPositionNormalTexture::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState);

        
        pd.CreatePipelineState(devRes->GetD3DDevice(), 
            mRootSignature.Get(), 
            { reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize() },
            { reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize() },
            mPSOs["opaque"].GetAddressOf());

        //ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        //opaquePsoDesc.InputLayout = VertexPositionNormalTexture::InputLayout;
        //opaquePsoDesc.pRootSignature = mRootSignature.Get();
        //opaquePsoDesc.VS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
        //    mShaders["standardVS"]->GetBufferSize()
        //};
        //opaquePsoDesc.PS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
        //    mShaders["opaquePS"]->GetBufferSize()
        //};
        //opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        //opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        //opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        //opaquePsoDesc.SampleMask = UINT_MAX; 
        //opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        //opaquePsoDesc.NumRenderTargets = 1;
        //opaquePsoDesc.RTVFormats[0] = devRes->GetBackBufferFormat();
        //opaquePsoDesc.SampleDesc.Count = 1;
        //opaquePsoDesc.SampleDesc.Quality = 0;
        //opaquePsoDesc.DSVFormat = devRes->GetDepthBufferFormat();
        //ThrowIfFailed(devRes->GetD3DDevice()->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

        ////
        //// PSO for shadow map pass.
        ////
        //D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
        //smapPsoDesc.RasterizerState.DepthBias = 100000;
        //smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        //smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        //smapPsoDesc.pRootSignature = mRootSignature.Get();
        //smapPsoDesc.VS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
        //    mShaders["shadowVS"]->GetBufferSize()
        //};
        //smapPsoDesc.PS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["shadowOpaquePS"]->GetBufferPointer()),
        //    mShaders["shadowOpaquePS"]->GetBufferSize()
        //};

        //// Shadow map pass does not have a render target.
        //smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        //smapPsoDesc.NumRenderTargets = 0;
        //ThrowIfFailed(devRes->GetD3DDevice()->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));

        ////
        //// PSO for debug layer.
        ////
        //D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = opaquePsoDesc;
        //debugPsoDesc.pRootSignature = mRootSignature.Get();
        //debugPsoDesc.VS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
        //    mShaders["debugVS"]->GetBufferSize()
        //};
        //debugPsoDesc.PS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
        //    mShaders["debugPS"]->GetBufferSize()
        //};
        //ThrowIfFailed(devRes->GetD3DDevice()->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["debug"])));

        ////
        //// PSO for sky.
        ////
        //D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

        //// The camera is inside the sky sphere, so just turn off culling.
        //skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

        //// Make sure the depth function is LESS_EQUAL and not just LESS.  
        //// Otherwise, the normalized depth values at z = 1 (NDC) will 
        //// fail the depth test if the depth buffer was cleared to 1.
        //skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        //skyPsoDesc.pRootSignature = mRootSignature.Get();
        //skyPsoDesc.VS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
        //    mShaders["skyVS"]->GetBufferSize()
        //};
        //skyPsoDesc.PS =
        //{
        //    reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
        //    mShaders["skyPS"]->GetBufferSize()
        //};
        //ThrowIfFailed(devRes->GetD3DDevice()->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));
    }

    // Create resource descriptors 
    {   
        m_resourceDescriptors = std::make_unique<DescriptorHeap>(devRes->GetD3DDevice(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            mTextures.size());
    }
    // Create SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        //srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MipLevels = mTextures["water"]->Resource->GetDesc().MipLevels;
        srvDesc.Format = mTextures["water"]->Resource->GetDesc().Format;

        devRes->GetD3DDevice()->CreateShaderResourceView(
            mTextures["water"]->Resource.Get(), 
            &srvDesc,
            m_resourceDescriptors->GetCpuHandle(mTexturesHeapIndex.size()));

        mTexturesHeapIndex["water"] = mTexturesHeapIndex.size();
    }
    // Create material
    {
        auto material = make_unique<Material>();
        material->Set("water", mMaterials.size(), mTexturesHeapIndex["water"]);
        mMaterials["water"] = std::move(material);  
    }
    // Set render item
    {
        auto boxRenderItem = make_unique<RenderItem>();
        boxRenderItem->Set(mAllRitems.size(), mGeometries["box"].get(), mMaterials["water"].get());
        mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRenderItem.get());
        mAllRitems.push_back(std::move(boxRenderItem));
    }
    // Create frame resources
    for (int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(devRes,
            1,  // main pass 
            mAllRitems.size(),  // objects
            mMaterials.size()   // materials
            ));
    }
}

void SceneMain::Update(DX::StepTimer const& timer)
{



    auto fence = g_pSys->pDeviceResources->GetFence();

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (mCurrFrameResource->Fence != 0 && fence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        DX::ThrowIfFailed(fence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }


    // Update camera
    mCamera.UpdateViewMatrix();

    static float angle = 0.0f;
    angle += timer.GetElapsedSeconds();
    Matrix rot = Matrix::CreateFromYawPitchRoll(DirectX::XMConvertToRadians(angle * 10.0f), 
        DirectX::XMConvertToRadians(angle * 10.0f),
        0.0f);

    // Update objects
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto& e : mAllRitems)
    {
        // Only update the cbuffer data if the constants have changed.  
        // This needs to be tracked per frame resource.
        if (e->NumFramesDirty > 0)
        {
            e->world = rot;

            ObjectConstants objConstants;
            objConstants.world = e->world;
            objConstants.texTransform = e->TexTransform;
            objConstants.materialIndex = e->Mat->MatCBIndex;

            currObjectCB->CopyData(e->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            //e->NumFramesDirty--;
        }
    }

    // Update materails
    auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
    for (auto& e : mMaterials)
    {
        // Only update the cbuffer data if the constants have changed.  If the cbuffer
        // data changes, it needs to be updated for each FrameResource.
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            MaterialData matData;
            matData.DiffuseAlbedo = mat->DiffuseAlbedo;
            matData.FresnelR0 = mat->FresnelR0;
            matData.Roughness = mat->Roughness;
            matData.MatTransform = mat->MatTransform;
            matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
            matData.NormalMapIndex = mat->NormalSrvHeapIndex;

            currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }

    // Update main pass
    Matrix shadowTransform = XMLoadFloat4x4(&mShadowTransform);

    mMainPassCB.View = mCamera.GetView();
    mMainPassCB.InvView = mCamera.GetView().Invert();
    mMainPassCB.Proj = mCamera.GetProj();
    mMainPassCB.InvProj = mCamera.GetProj().Invert();

    Matrix viewProj = mCamera.GetView() * mCamera.GetProj();

    mMainPassCB.ViewProj = viewProj;
    mMainPassCB.InvViewProj = viewProj.Invert();
    mMainPassCB.ShadowTransform = shadowTransform;
    mMainPassCB.EyePosW = mCamera.GetPosition();

    RECT rect = g_pSys->pDeviceResources->GetOutputSize();
    float width = rect.right;
    float height = rect.bottom;
    mMainPassCB.RenderTargetSize = { width, height };
    mMainPassCB.InvRenderTargetSize = { 1.0f / width, 1.0f / height };
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = timer.GetTotalSeconds();
    mMainPassCB.DeltaTime = timer.GetElapsedSeconds();
    mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
    mMainPassCB.Lights[0].Direction = mRotatedLightDirections[0];
    mMainPassCB.Lights[0].Strength = { 0.9f, 0.8f, 0.7f };
    mMainPassCB.Lights[1].Direction = mRotatedLightDirections[1];
    mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
    mMainPassCB.Lights[2].Direction = mRotatedLightDirections[2];
    mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
    //

    ImGui::Begin("ecc");
    ImGui::End();

    // 
    ImVec2 sizeAppWindow = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(sizeAppWindow.x * 0.3f , 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(sizeAppWindow.x * 0.6f, sizeAppWindow.y * 0.6f), ImGuiCond_Always);
    
    //ImGui::SetNextWindowFocus();
    ImGui::Begin("SceneMain");

    if (ImGui::Button("Change Scene"))
    {
        g_pSys->pDeviceResources->WaitForGpu();
        g_pSys->pSceneManager->ChangeScene(new SceneLoad(new SceneTitle()));
    }
    renderWindowPos = ImGui::GetWindowPos();
    renderWindowSize = ImGui::GetWindowSize();
    ImGui::Text("Window size :%.0f %.0f", renderWindowSize.x, renderWindowSize.y);
    ImGui::End();
    
    mCamera.SetLens(0.25f * 3.1415926535f, renderWindowSize.x / renderWindowSize.y, 1.0f, 1000.0f);
}

void SceneMain::Render()
{

    auto cmdList = g_pSys->pDeviceResources->GetCommandList();

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_resourceDescriptors->Heap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Set render state
    cmdList->SetGraphicsRootSignature(mRootSignature.Get());

    auto passCB = mCurrFrameResource->PassCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
    cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(3, m_resourceDescriptors->GetFirstGpuHandle());

    // Set the viewport and scissor rect.
    D3D12_VIEWPORT renderWindowView;
    renderWindowView.TopLeftX = renderWindowPos.x;
    renderWindowView.TopLeftY = renderWindowPos.y;
    renderWindowView.Width = renderWindowSize.x;
    renderWindowView.Height = renderWindowSize.y;
    renderWindowView.MinDepth = 0.1f;
    renderWindowView.MaxDepth = 1.0f;

    RECT renderWindowRect;
    renderWindowRect.left = renderWindowPos.x;
    renderWindowRect.top = renderWindowPos.y;
    renderWindowRect.right = renderWindowPos.x + renderWindowSize.x;
    renderWindowRect.bottom = renderWindowPos.y + +renderWindowSize.y;

    cmdList->RSSetViewports(1, &renderWindowView);
    cmdList->RSSetScissorRects(1, &renderWindowRect);

    // Render opaque objects
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    auto objectCB = mCurrFrameResource->ObjectCB->Resource();

    cmdList->SetPipelineState(mPSOs["opaque"].Get());

    // For each render item...
    for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::Opaque].size(); ++i)
    {
        auto ri = mRitemLayer[(int)RenderLayer::Opaque][i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;

        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }


}
