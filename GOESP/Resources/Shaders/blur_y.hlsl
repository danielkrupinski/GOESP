sampler texSampler;

float texelHeight;

static const float offsets[3] = { 0.0, 1.3846153846, 3.2307692308 };
static const float weights[3] = { 0.2270270270, 0.3162162162, 0.0702702703 };

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    float4 color = tex2D(texSampler, uv);
    (float3)color *= weights[0];

    for (int i = 1; i < 3; ++i) {
        (float3)color += (float3)tex2D(texSampler, uv - float2(0.0f, texelHeight * offsets[i])) * weights[i];
        (float3)color += (float3)tex2D(texSampler, uv + float2(0.0f, texelHeight * offsets[i])) * weights[i];
    }

    return color;
}
