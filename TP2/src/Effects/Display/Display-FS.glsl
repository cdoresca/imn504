#version 460

uniform sampler2D myFBO;

in vec2 uv;

layout(location = 0) out vec4 Color;

void main() {
    vec3 c = texture(myFBO, uv).rgb;

    Color = vec4(c.rgb, 1.0);
}