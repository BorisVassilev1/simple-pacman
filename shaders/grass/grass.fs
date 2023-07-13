#version 430

#include <rendering.glsl>

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vVertexNormal;
in vec3 vVertexPos;

out vec4 fragColor;

uniform vec3 color0 = pow(vec3(0.1, 0.5, 0.1), vec3(2.2));
uniform vec3 color1 = pow(vec3(0.9, 1, 0.7), vec3(2.2));

void main() {
	vec3 albedo = mix(color0, color1, vColor.x * vColor.x);

	vec3 normalizedVertexNormal = normalize(vVertexNormal);

	// if(!gl_FrontFacing) normalizedVertexNormal = -normalizedVertexNormal;
	vec3 color = calcAllLights(vVertexPos, normalizedVertexNormal, normalizedVertexNormal, vTexCoord);

	// color = vec3(materials[material_index].albedo);
	// color = vVertexNormal;

	fragColor = vec4(color, 1.0);
}