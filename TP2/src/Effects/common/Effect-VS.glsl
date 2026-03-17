#version 460

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

layout(location = 0) in vec3 Position;
layout(location = 3) in vec3 texCoord;

out vec2 uv;

void main() {
    gl_Position = vec4(Position, 1.0);

    uv = texCoord.xy;
    // uv = Position.xy*0.5+0.5;
}
