#version 430

#include <rendering.glsl>

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vVertexNormal;
out vec3 vVertexPos;

uniform mat4 worldMatrix;

void main() {
	vec4 mvPos = worldMatrix * vec4(position, 1.0);
	
	gl_PointSize = 5.0;
	gl_Position = projectionMatrix * viewMatrix * mvPos;
	
	vColor = color;
	vTexCoord = texCoord;
	vVertexNormal = normalize(worldMatrix * vec4(normal, 0.0)).xyz;
    vVertexPos = mvPos.xyz;
}