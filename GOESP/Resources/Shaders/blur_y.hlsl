sampler texSampler;

float texelHeight;

static const float offsets[5] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
static const float weights[5] = { 0.2270270270f, 0.1945945946f, 0.1216216216f, 0.0540540541f, 0.0162162162f };

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    float4 color = tex2D(texSampler, uv);
    (float3)color *= weights[0];

    for (int i = 1; i < 5; ++i) {
        (float3)color += (float3)tex2D(texSampler, uv + float2(0.0f, -texelHeight * offsets[i])) * weights[i];
        (float3)color += (float3)tex2D(texSampler, uv + float2(0.0f,  texelHeight * offsets[i])) * weights[i];
    }

    return color;
}
