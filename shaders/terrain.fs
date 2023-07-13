#version 430

#include <rendering.glsl>

in vec4 teColor;
in vec2 teTexCoord;
in vec3 teVertexNormal;
in vec3 teVertexPos;
in mat3 teTBN;

out vec4	 fragColor;
uniform mat4 worldMatrix;

void main() {
	Material mat = materials[material_index];

	vec3 finalNormal;
	if (mat.use_normal_map != 0.0) {
		vec3 normal = texture(normalMap, teTexCoord).xyz;

		normal = normalize(normal * 2. - 1.);

		finalNormal = normalize(teTBN * normal);
	} else {
		finalNormal = teVertexNormal;
	}

	if(!gl_FrontFacing) finalNormal = -finalNormal;
	vec3 light = calcAllLights(teVertexPos, finalNormal, finalNormal, teTexCoord);

	// vec3 light = vec3(texture(normalMap, teTexCoord));
	// light = finalNormal;

	fragColor = vec4(light, 1.0);
}