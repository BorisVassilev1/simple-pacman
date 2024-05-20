#include <rendering.glsl>

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vVertexNormal;
in vec3 vVertexPos;

out vec4 fragColor;

uniform sampler2D texture_sampler;

void main() {
	Material mat = materials[material_index];

	vec4  mask	= texture(albedoMap, vTexCoord).xyzw;
	mask *= mat.use_albedo_map; // if use_albedo_map is 0, then the ghost is dead

	vec3  albedo = mat.albedo;
	vec4  eyes	= texture(aoMap, vTexCoord).xyzw;
	
	if(mat.use_ao_map == 0.0) { // if the ghost is weak and scared
		eyes = vec4(eyes.w); // white eyes
		albedo = vec3(0.125, 0.125, 0.96); // some blue body
	}
	
	mask.xyz *= albedo;

	fragColor = vec4(mask + eyes);
}
