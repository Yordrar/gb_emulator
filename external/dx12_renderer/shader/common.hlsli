struct CameraData
{
    float4x4 viewProjMatrix;
    float4x4 inverseViewProjMatrix;
    float4 cameraPosition;
};

struct GeometryData
{
    float4x4 modelMatrix;
    float4x4 inverseModelMatrix;
};

struct DrawConstants
{
    uint passBufferIndex;
    uint cameraBufferIndex;
    uint materialBufferIndex;
    uint geometryBufferIndex;
};
ConstantBuffer<DrawConstants> drawConstants : register(b0, space0);

template<typename buffer_type>
buffer_type getBuffer(uint index)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[index];
    return buffer.Load<buffer_type>(0);
}

template<typename buffer_type>
buffer_type getPassBuffer()
{
    return getBuffer<buffer_type>(drawConstants.passBufferIndex);
}

CameraData getCameraData()
{
    return getBuffer<CameraData>(drawConstants.cameraBufferIndex);
}

template<typename buffer_type>
buffer_type getMaterialBuffer()
{
    return getBuffer<buffer_type>(drawConstants.materialBufferIndex);
}