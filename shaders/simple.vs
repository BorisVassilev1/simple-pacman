#version 430

#include <rendering.glsl>

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vVertexNormal;
out vec3 vVertexPos;
out mat3 vTBN;

uniform mat4 worldMatrix;

void main() {
	vec4 vPos = worldMatrix * vec4(position, 1.0);

	gl_PointSize = 5.0;
	gl_Position = projectionMatrix * viewMatrix * vPos;
	
	vColor = color;
	vTexCoord = texCoord;
	vVertexNormal = normalize(worldMatrix * vec4(normal, 0.0)).xyz;
	vVertexPos = vPos.xyz;

	vec3 worldSpaceTangent = normalize(vec3(worldMatrix * vec4(tangent, 0.0)));

	// re-orthogonalize T with respect to N
	// worldSpaceTangent = normalize(worldSpaceTangent - dot(worldSpaceTangent, vVertexNormal) * vVertexNormal);

	vTBN = mat3(worldSpaceTangent, cross(worldSpaceTangent, vVertexNormal), vVertexNormal);
}