#version 460
#extension GL_EXT_nonuniform_qualifier : require


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
    mat4 transform;
    vec4 min;
    vec4 max;
};
layout(std430, set = 1, binding = 0) buffer MatrixBuffer{
    Brick brick[];
};
layout(set=1, binding=1) readonly buffer VisibleIDs  { uint ids[]; };

void main() {
    
     uint id = ids[gl_InstanceIndex];
    mat4 model = brick[id].transform;
    
    gl_Position = viewUbo.projection * viewUbo.view * model * vec4(inPosition, 1.0);
    fragNormal = inNormal;
}
