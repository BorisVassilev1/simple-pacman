#version 430
// Triangles
layout(vertices = 3) out;

layout(std140, binding = 0) uniform Matrices {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 cameraWorldMatrix;
};

in vec4 vColor[];
in vec2 vTexCoord[];
in vec3 vVertexNormal[];
in vec4 vVertexPos[];
in mat3 vTBN[];

out vec4 tcColor[];
out vec2 tcTexCoord[];
out vec3 tcVertexNormal[];
out mat3 tcTBN[];

const int	MIN_TES	 = 2;
const int	MAX_TES	 = 32;
const float MIN_DIST = 1;
const float MAX_DIST = 5;

void main() {
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	tcColor[gl_InvocationID]		= vColor[gl_InvocationID];
	tcTexCoord[gl_InvocationID]		= vTexCoord[gl_InvocationID];
	tcVertexNormal[gl_InvocationID] = vVertexNormal[gl_InvocationID];
	tcTBN[gl_InvocationID]			= vTBN[gl_InvocationID];

	if (gl_InvocationID == 0) {
		// use the world space vertex position
		vec4 center0 = vVertexPos[0] + (vVertexPos[2] - vVertexPos[0]) / 2.0;
		vec4 center1 = vVertexPos[1] + (vVertexPos[0] - vVertexPos[1]) / 2.0;
		vec4 center2 = vVertexPos[2] + (vVertexPos[1] - vVertexPos[2]) / 2.0;

		vec4 cameraPos = cameraWorldMatrix[3];

		float dist0 = length(cameraPos - center0);
		float dist1 = length(cameraPos - center1);
		float dist2 = length(cameraPos - center2);

		float tes0 = mix(MAX_TES, MIN_TES, clamp(dist0 / MAX_DIST, 0.0, 1.0));
		float tes1 = mix(MAX_TES, MIN_TES, clamp(dist1 / MAX_DIST, 0.0, 1.0));
		float tes2 = mix(MAX_TES, MIN_TES, clamp(dist2 / MAX_DIST, 0.0, 1.0));

		gl_TessLevelOuter[0] = tes2;	 // left for triangles
		gl_TessLevelOuter[1] = tes0;	 // bot for triangles
		gl_TessLevelOuter[2] = tes1;	 // right for triangles

		gl_TessLevelInner[0] = max(tes0, max(tes1, tes2));	   // all inner sides for triangles
	}
}
