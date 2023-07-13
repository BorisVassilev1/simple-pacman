#version 430

#include <rendering.glsl>

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vVertexNormal;
in vec3 vVertexPos;

out vec4 fragColor;

uniform sampler2D texture_sampler;

void main() {
	Material mat = materials[material_index];

	vec4  diffuse	= texture(albedoMap, vTexCoord).xyzw;
	vec4  calcAlbedo	= mix(vec4(mat.albedo, 1.), diffuse, mat.use_albedo_map);
	
	fragColor = calcAlbedo;
}