R"(
#version 130
#extension GL_ARB_explicit_attrib_location : require

in vec2 pos;
in vec2 uv;

out vec2 fragUV;

void main()
{
    fragUV = uv;
    gl_Position = vec4(pos.xy, 0, 1);
}
)"
