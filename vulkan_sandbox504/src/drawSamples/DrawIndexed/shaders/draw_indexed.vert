#version 460
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

void main() {
    gl_Position = viewUbo.projection * viewUbo.view * vec4(inPosition, 1.0);
    fragNormal = inNormal;
}
