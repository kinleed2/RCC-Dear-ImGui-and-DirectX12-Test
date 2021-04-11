#include "FrameResource.h"

const int gNumFrameResources = 3;

FrameResource::FrameResource(DX::DeviceResources* devRes, UINT passCount, UINT objectCount, UINT materialCount)
{
    auto dev = devRes->GetD3DDevice();

    DX::ThrowIfFailed(dev->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));
    if (passCount > 0)
        PassCB = std::make_unique<UploadBuffer<PassConstants>>(dev, passCount, true);
    if (materialCount > 0)
        MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(dev, materialCount, false);
    if (objectCount > 0)
        ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(dev, objectCount, true);
}

FrameResource::~FrameResource()
{
}
