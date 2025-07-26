struct VertexInput
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

cbuffer CameraBuffer : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 proj;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    float4 worldPos = mul(model, float4(input.position, 1.0));
    float4 viewPos  = mul(view, worldPos);
    output.position = mul(proj, viewPos);

    output.uv     = input.uv;
    output.normal = normalize(mul((float3x3)model, input.normal));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float lighting = max(dot(normalize(input.normal), lightDir), 0.0);

    float3 baseColor = float3(1.0, 0.6, 0.3);
    return float4(baseColor, 1.0);
}