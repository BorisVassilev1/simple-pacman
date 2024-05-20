#define VERT
// askdjaskdjkasjdkasjd

#include <rendering.glsl>

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vVertexNormal;
out vec3 vVertexPos;
out mat3 vTBN;

uniform mat4 worldMatrix;

const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;
layout(std140, binding=3) uniform mats{
	mat4 finalBonesMatrices[MAX_BONES];
};
uniform bool animate = false;

void main() {

	vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0f);
	if(animate) {
		for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    	{
    	    if(boneIds[i] == -1) 
    	        continue;
    	    if(boneIds[i] >=MAX_BONES) 
    	    {
    	        totalPosition = vec4(position,1.0f);
    	        break;
    	    }
    	    vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(position,1.0f);
    	    totalPosition += localPosition * weights[i];
    	    vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * normal;
			totalNormal += localNormal * weights[i];
    	}
	} else {
		totalPosition = vec4(position, 1.0);
		totalNormal = normal;
	}

	vec4 vPos = worldMatrix * totalPosition;

	gl_PointSize = 5.0;
	gl_Position = projectionMatrix * viewMatrix * vPos;
	
	vColor = color;
	vTexCoord = texCoord;
	vVertexNormal = normalize(worldMatrix * vec4(totalNormal, 0.0)).xyz;
	vVertexPos = vPos.xyz;

	vec3 worldSpaceTangent = normalize(vec3(worldMatrix * vec4(tangent, 0.0)));

	// re-orthogonalize T with respect to N
	worldSpaceTangent = normalize(worldSpaceTangent - dot(worldSpaceTangent, vVertexNormal) * vVertexNormal);

	vTBN = mat3(worldSpaceTangent, cross(worldSpaceTangent, vVertexNormal), vVertexNormal);
}
