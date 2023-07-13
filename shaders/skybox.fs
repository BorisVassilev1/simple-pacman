#version 430
layout(binding = 1) uniform samplerCube tex;

in vec3 vTexCoord;
out vec4 fragColor;

void main() {
    fragColor = texture(tex, vTexCoord);
    fragColor.w = 1.0;
}