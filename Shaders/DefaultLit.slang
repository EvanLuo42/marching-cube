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

struct BlinnPhongVariables
{
    float3 lightPos;
    float shininess;
};

[[vk::push_constant]]
BlinnPhongVariables blinnPhongVars;

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
    float3 color = float3(1.0, 1.0, 1.0);

    // Ambient
    float3 ambient = 0.05 * color;

    // Diffuse
    float3 lightDir = normalize(blinnPhongVars.lightPos - input.worldPos);
    float3 normal = normalize(input.normal);
    float diff = max(dot(lightDir, normal), 0.0);
    float3 diffuse = diff * color;

    // Specular
    float3 viewDir = normalize(cameraPos - input.worldPos);
    float spec = 0.0;

    float3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), blinnPhongVars.shininess);

    float3 specular = 0.3 * spec;

    float3 result = ambient + diffuse + specular;
    return float4(result, 1.0);
}