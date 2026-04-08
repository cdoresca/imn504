#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_shader_draw_paramaters : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragNormal;

layout(binding = 0, set = 0) uniform ViewUBO {
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
    float near;
    float far;
} viewUbo;

struct Brick{
    uint type;
    mat4 model;
};

layout(std430, set = 1, binding = 0) buffer MatrixBuffer{
    Brick transform[];
};


void main() {
    int id = gl_DrawId;
    
    gl_Position = viewUbo.projection * viewUbo.view * transform[id].model * vec4(inPosition, 1.0);
    fragNormal = inNormal;
}
