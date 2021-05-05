sampler texSampler;

float amount;

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    float4 color = tex2D(texSampler, uv);
    float gray = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    return float4(lerp(color.rgb, float3(gray, gray, gray), amount), 1.0f);
}
