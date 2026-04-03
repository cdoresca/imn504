#version 460
layout(location = 0) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diffuse = max(dot(normalize(fragNormal), lightDir), 0.0);
    vec3 color = vec3(0.8, 0.5, 0.3) * (0.2 + 0.8 * diffuse); // ambient + diffuse
    outColor = vec4(color, 1.0);
}
