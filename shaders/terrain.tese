#version 430
// Triangles
layout(triangles, fractional_even_spacing, ccw) in;

layout(std140, binding=0) uniform Matrices {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 cameraWorldMatrix;
};

in vec4 tcColor[];
in vec2 tcTexCoord[];
in vec3 tcVertexNormal[]; // model-space
in mat3 tcTBN[];
// in gl_Position // in model-space

out vec4 teColor;
out vec2 teTexCoord;
out vec3 teVertexNormal;
out vec3 teVertexPos;
out mat3 teTBN;

layout(binding=1) uniform sampler2D albedoMap;
layout(binding=2) uniform sampler2D normalMap;
layout(binding=3) uniform sampler2D heightMap;

uniform mat4 worldMatrix;
uniform float displacementPower;
uniform float displacementOffset;

void main() {
	// barycentric coordinates
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	float w = gl_TessCoord.z;
	// barycentric interpolation
	vec2 texCoord = u * tcTexCoord[0] + v * tcTexCoord[1] + w * tcTexCoord[2];
    vec3 normal = u * tcVertexNormal[0] + v * tcVertexNormal[1] + w * tcVertexNormal[2];
    vec4 color = u * tcColor[0] + v * tcColor[1] + w * tcColor[2];
	mat3 TBN = u * tcTBN[0] + v * tcTBN[1] + w * tcTBN[2];

	vec4 pos0 = gl_in[0].gl_Position;
	vec4 pos1 = gl_in[1].gl_Position;
	vec4 pos2 = gl_in[2].gl_Position;
	// barycentric interpolation
	vec4 pos = u * pos0 + v * pos1 + w * pos2;

	// still in model space for appropriate scaling of displacement
    pos.xyz += (texture(heightMap, texCoord).x + displacementOffset) * displacementPower  * normal;

	// vertex position goes to world space
    pos = worldMatrix * pos;
	// gl_Position is now in view-space
	gl_Position = projectionMatrix * viewMatrix * pos;

	teColor = color;
	teTexCoord	= texCoord;
	
	// to the fragment shader stil goes the world space position
    teVertexPos = pos.xyz;

	// model space normals
    teVertexNormal = normal;
    
	teTBN = TBN;
}