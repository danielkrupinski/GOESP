R"(
#version 130

in vec2 fragUV;
out vec4 color;

uniform sampler2D texSampler;
uniform float texelHeight;

float offsets[5] = float[](0.0f, 1.0f, 2.0f, 3.0f, 4.0f);
float weights[5] = float[](0.2270270270f, 0.1945945946f, 0.1216216216f, 0.0540540541f, 0.0162162162f);

void main()
{
    color = texture(texSampler, fragUV);
    color.rgb *= weights[0];
    for (int i = 1; i < 5; ++i) {
        color.rgb += texture(texSampler, fragUV - vec2(0.0f, texelHeight * offsets[i])).rgb * weights[i];
        color.rgb += texture(texSampler, fragUV + vec2(0.0f, texelHeight * offsets[i])).rgb * weights[i];
    }
}
)"
