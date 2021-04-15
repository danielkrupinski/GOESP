R"(
#version 130
#extension GL_ARB_explicit_attrib_location : require
#extension GL_ARB_explicit_uniform_location : require

in vec2 uv;
out vec4 color;

layout(location = 0) uniform sampler2D texSampler;
layout(location = 1) uniform float amount;

void main()
{
    color.r = texture(texSampler, uv - amount).r;
    color.g = texture(texSampler).g;
    color.b = texture(texSampler, uv + amount).b;
    color.a = 1.0f;
}
)"
