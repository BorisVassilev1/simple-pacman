#version 430

#include <rendering.glsl>

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vVertexNormal;		// vertex normal in model space
out vec4 vVertexPos;		// vertex position in world space
// gl_Position is in model space

out mat3 vTBN;

uniform mat4 worldMatrix;

void main() {
	vec4 mvPos = vec4(position, 1.0);

	gl_PointSize = 5.0;
	gl_Position	 = mvPos;

	vColor		  = color;
	vTexCoord	  = texCoord;
	vVertexNormal = normal;
	// this goes to world space
	vVertexPos = worldMatrix * mvPos;

	vec3 worldSpaceNormal  = normalize(vec3(worldMatrix * vec4(normal, 0.0)));
	vec3 worldSpaceTangent = normalize(vec3(worldMatrix * vec4(tangent, 0.0)));

	// worldSpaceTangent = normalize(worldSpaceTangent - dot(worldSpaceTangent, vVertexNormal) * vVertexNormal);

	vTBN = mat3(worldSpaceTangent, cross(worldSpaceTangent, worldSpaceNormal), worldSpaceNormal);
}