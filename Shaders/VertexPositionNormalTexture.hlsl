//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
    float3 PosL    : SV_Position;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION1;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;

    // Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];

    // Transform to world space.
    float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul((float3x3)gWorld, vin.NormalL);
    
    // Transform to homogeneous clip space.
    vout.PosH = mul(gViewProj, posW );

    // Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);

    vout.TexC = mul(texC, matData.MatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float  roughness = matData.Roughness;
    uint diffuseMapIndex = matData.DiffuseMapIndex;

    // Dynamically look up the texture in the array.
    diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

//#ifdef ALPHA_TEST
//    // Discard pixel if texture alpha < 0.1.  We do this test as soon 
//    // as possible in the shader so that we can potentially exit the
//    // shader early, thereby skipping the rest of the shader code.
//    clip(diffuseAlbedo.a - 0.1f);
//#endif
//
//    // Uncomment to turn off normal mapping.
//    bumpedNormalW = pin.NormalW;
//
//    // Vector from point being lit to eye. 
//    float3 toEyeW = normalize(gEyePosW - pin.PosW);
//
//    // Light terms.
//    float4 ambient = gAmbientLight * diffuseAlbedo;
//
//    const float shininess = (1.0f - roughness) * normalMapSample.a;
//    Material mat = { diffuseAlbedo, fresnelR0, shininess };
//    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
//        bumpedNormalW, toEyeW, shadowFactor);
//
//    float4 litColor = ambient + directLight;
//
//    // Add in specular reflections.
//    float3 r = reflect(-toEyeW, bumpedNormalW);
//    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
//    litColor.rgb += shininess * fresnelFactor;
//
//    // Common convention to take alpha from diffuse albedo.
//    litColor.a = diffuseAlbedo.a;

    return diffuseAlbedo;
}
