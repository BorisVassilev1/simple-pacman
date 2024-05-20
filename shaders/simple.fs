#define FRAG

#include <rendering.glsl>

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vVertexNormal;
in vec3 vVertexPos;
in mat3 vTBN;

out vec4 fragColor;

void main() {
	vec3	 normalizedVertexNormal = normalize(vVertexNormal);
	vec3	 finalNormal;
	Material mat = materials[material_index];

	if (mat.use_normal_map != 0.0) {
		vec3 normal = texture(normalMap, vTexCoord).xyz;

		normal = normalize(normal * 2. - 1.);

		finalNormal = normalize(mix(vVertexNormal, vTBN * normal, mat.use_normal_map));
	} else {
		finalNormal = normalizedVertexNormal;
	}

	vec3 color;

	if (!gl_FrontFacing) finalNormal = -finalNormal;
	color = calcAllLights(vVertexPos, finalNormal, finalNormal, vTexCoord);

	// color = vec3(materials[material_index].albedo);
	// color = vVertexNormal;
	// color = vVertexPos;
	// color = vColor.xyz;

	fragColor = vec4(color, 1.0);
}
