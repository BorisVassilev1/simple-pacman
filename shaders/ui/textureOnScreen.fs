#version 460

in vec3 outVertexPos;
in vec2 outTexCoord;

out vec4 fragColor;

layout(binding = 7) uniform sampler2D  sampler_color;
layout(binding = 8) uniform sampler2D  sampler_depth;
layout(binding = 9) uniform usampler2D sampler_stencil;

uniform int viewMode = 0;

void main() {
	if (viewMode == 0) {
		fragColor = texture(sampler_color, outTexCoord);
	} else if (viewMode == 1) {
		fragColor = vec4(vec3(texture(sampler_depth, outTexCoord).x), 1.0);
	} else if (viewMode == 2) {
		fragColor = vec4(vec3(texture(sampler_stencil, outTexCoord).x), 1.0);
	}
}