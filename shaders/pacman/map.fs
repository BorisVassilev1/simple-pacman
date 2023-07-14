#version 430

#include <rendering.glsl>

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vVertexNormal;
in vec3 vVertexPos;

out vec4 fragColor;

uniform sampler2D texture_sampler;
uniform ivec2 resolution;

void main() {
	Material mat = materials[material_index];
	vec4  mapData = texture(albedoMap, vTexCoord).xyzw;

	vec2 position = vTexCoord * resolution - vec2(0.5);
	vec2 center = round(position);
	vec2 distanceFromCenter = position - center;

	float dotRadius = 0.03 * mapData.y;
	dotRadius += 0.1 * mapData.x;

	vec3 dotColor = vec3(step(dot(distanceFromCenter, distanceFromCenter), dotRadius));
	
	vec3 finalColor = dotColor;
	finalColor.z += mapData.z;

	fragColor = vec4(finalColor, 1.0);
}