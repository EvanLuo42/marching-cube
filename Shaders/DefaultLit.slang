struct VertexInput
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POS;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3 cameraPos;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    float4 worldPos = mul(float4(input.position, 1.0), model);
    float4 viewPos  = mul(worldPos, view);
    output.position = mul(viewPos, proj);
    output.worldPos = worldPos.xyz;

    output.uv     = input.uv;
    output.normal = normalize(mul((float3x3)model, input.normal));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 lightColor = float3(1.0, 1.0, 1.0);
    return float4(lightColor, 1.0);
}