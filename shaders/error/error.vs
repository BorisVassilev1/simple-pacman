precision highp float;

layout(location = 0) in vec3 position;

layout(std140) uniform Matrices {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 cameraWorldMatrix;
};

out vec3 vVertexPos;

uniform mat4 worldMatrix;

void main() {
	vec4 mvPos = worldMatrix * vec4(position, 1.0);
	
	gl_PointSize = 5.0;
	gl_Position = projectionMatrix * viewMatrix * mvPos;
	
    vVertexPos = mvPos.xyz;
}
