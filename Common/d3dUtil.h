//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include "../DirectXTK12/Inc/DDSTextureLoader.h"
#include "../DirectXTK12/Inc/SimpleMath.h"
#include "../DirectXTK12/Inc/ResourceUploadBatch.h"
#include "../DirectXTK12/Inc/VertexTypes.h"
#include "DeviceResources.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace std;

extern const int gNumFrameResources;

namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }
}

inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

/*
#if defined(_DEBUG)
    #ifndef Assert
    #define Assert(x, description)                                  \
    {                                                               \
        static bool ignoreAssert = false;                           \
        if(!ignoreAssert && !(x))                                   \
        {                                                           \
            Debug::AssertResult result = Debug::ShowAssertDialog(   \
            (L#x), description, AnsiToWString(__FILE__), __LINE__); \
        if(result == Debug::AssertIgnore)                           \
        {                                                           \
            ignoreAssert = true;                                    \
        }                                                           \
                    else if(result == Debug::AssertBreak)           \
        {                                                           \
            __debugbreak();                                         \
        }                                                           \
        }                                                           \
    }
    #endif
#else
    #ifndef Assert
    #define Assert(x, description) 
    #endif
#endif 		
    */

class d3dUtil
{
public:

    static bool IsKeyDown(int vkeyCode);

    static std::string ToString(HRESULT hr);

    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return (byteSize + 255) & ~255;
    }

    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
};

struct Light
{
    Vector3 Strength = { 0.5f, 0.5f, 0.5f };
    float FalloffStart = 1.0f;                          // point/spot light only
    Vector3 Direction = { 0.0f, -1.0f, 0.0f };          // directional/spot light only
    float FalloffEnd = 10.0f;                           // point/spot light only
    Vector3 Position = { 0.0f, 0.0f, 0.0f };            // point/spot light only
    float SpotPower = 64.0f;                            // spot light only
};

#define MaxLights 16

// Defines a subrange of geometry in a MeshGeometry.  This is for when multiple
// geometries are stored in one vertex and index buffer.  It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index 
// buffers so that we can implement the technique described by Figure 6.3.
struct SubmeshGeometry
{
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    INT BaseVertexLocation = 0;

    // Bounding box of the geometry defined by this submesh. 
    // This is used in later chapters of the book.
    DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
    // Give it a name so we can look it up by name.
    std::string Name;

    // System memory copies.  Use Blobs because the vertex/index format can be generic.
    // It is up to the client to cast appropriately.  
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer = nullptr;

    // Data about the buffers.
    UINT VertexByteStride = 0;
    UINT VertexBufferByteSize = 0;
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
    UINT IndexBufferByteSize = 0;

    UINT TotalIndexCount = 0;

    // A MeshGeometry may store multiple geometries in one vertex/index buffer.
    // Use this container to define the Submesh geometries so we can draw
    // the Submeshes individually.
    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }
    template <typename VertexTypes>
    void Set(DX::DeviceResources* devRes, const std::string name, const vector<VertexTypes>& vertices,
        const vector<uint16_t>& indices)
    {
        Name = name;

        auto dev = devRes->GetD3DDevice();

        ResourceUploadBatch upload(dev);

        upload.Begin();

        TotalIndexCount = indices.size();

        const UINT vbByteSize = (UINT)vertices.size() * sizeof(VertexTypes);
        const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

        // Copy data into the upload heap
        SharedGraphicsResource vertexUploadBuffer = GraphicsMemory::Get(dev).Allocate(vbByteSize);
        SharedGraphicsResource indexUploadBuffer = GraphicsMemory::Get(dev).Allocate(ibByteSize);

        memcpy(vertexUploadBuffer.Memory(), vertices.data(), vbByteSize);
        memcpy(indexUploadBuffer.Memory(), indices.data(), ibByteSize);

        auto vbdesc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);
        auto ibdesc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(dev->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &vbdesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(VertexBuffer.GetAddressOf())
        ));
        DX::ThrowIfFailed(dev->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &ibdesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(IndexBuffer.GetAddressOf())
        ));

        upload.Upload(VertexBuffer.Get(), vertexUploadBuffer);
        upload.Upload(IndexBuffer.Get(), indexUploadBuffer);

        upload.Transition(VertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        upload.Transition(IndexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        auto cmdQueue = devRes->GetCommandQueue();

        // Upload the resources to the GPU.
        auto finish = upload.End(cmdQueue);

        // Wait for the upload thread to terminate
        finish.wait();

        VertexByteStride = sizeof(VertexTypes);
        VertexBufferByteSize = vbByteSize;
        IndexFormat = DXGI_FORMAT_R16_UINT;
        IndexBufferByteSize = ibByteSize;
    }
};

// Simple struct to represent a material for our demos.  A production 3D engine
// would likely create a class hierarchy of Materials.
struct Material
{
    // Unique material name for lookup.
    std::string Name;

    // Index into constant buffer corresponding to this material.
    int MatCBIndex = -1;

    // Index into SRV heap for diffuse texture.
    int DiffuseSrvHeapIndex = -1;

    // Index into SRV heap for normal texture.
    int NormalSrvHeapIndex = -1;

    // Dirty flag indicating the material has changed and we need to update the constant buffer.
    // Because we have a material constant buffer for each FrameResource, we have to apply the
    // update to each FrameResource.  Thus, when we modify a material we should set 
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    int NumFramesDirty = gNumFrameResources;

    // Material constant buffer data used for shading.
    Vector4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = .25f;
    Matrix MatTransform = Matrix::Identity;

    void Set(const string& name, int matCBIndex, int diffuseSrvHeapIndex)
    {
        this->Name = name;
        this->MatCBIndex = matCBIndex;
        this->DiffuseSrvHeapIndex = diffuseSrvHeapIndex;
    }
};

struct Texture
{
    // Unique material name for lookup.
    std::string Name;

    std::wstring Filename;

    Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
    RenderItem() = default;
    RenderItem(const RenderItem& rhs) = delete;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    Matrix world = Matrix::Identity;

    Matrix TexTransform = Matrix::Identity;

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource.  Thus, when we modify obect data we should set 
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    int NumFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    UINT ObjCBIndex = -1;

    Material* Mat = nullptr;
    MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;

    void Set(UINT objectindex, MeshGeometry* geo, Material* mat)
    {
        this->ObjCBIndex = objectindex;
        this->Geo = geo;
        this->Mat = mat;

        IndexCount = Geo->TotalIndexCount;
    }

};

enum class RenderLayer : int
{
    Opaque = 0,
    Debug,
    Sky,
    Count
};

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif