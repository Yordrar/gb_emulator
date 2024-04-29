#include "common.hlsli"

struct VSIn
{
    float3 m_position : POSITION;
    float2 m_uvs : TEXCOORDS;
};

struct VSOut
{
    float4 m_position : SV_POSITION;
    float2 m_uvs : TEXCOORDS;
};

VSOut commonVertexProcessing(VSIn vertexData)
{
    VSOut vsOut;
    vsOut.m_position = float4(vertexData.m_position, 1.0f);
    vsOut.m_uvs = vertexData.m_uvs;
    return vsOut;
}

VSOut main_vs( VSIn vertexData )
{
    return commonVertexProcessing( vertexData );
}

struct ResourceIndices
{
    uint textureIdx;
};

[earlydepthstencil]
float4 main_ps(VSOut vsOut) : SV_Target
{
    ResourceIndices indices = getMaterialBuffer<ResourceIndices>();
    Texture2D tex = ResourceDescriptorHeap[indices.textureIdx];
    SamplerState samp = SamplerDescriptorHeap[0];
    float4 col = tex.Sample(samp, vsOut.m_uvs);
    return col;
}