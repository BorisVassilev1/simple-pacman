layout (location = 0) in vec3 vertexPos;
layout (location = 2) in vec2 texCoord;

out vec2 outTexCoord;

void main() {
	vec4 position = vec4(vertexPos.xy, 0., 1.0);
	gl_Position = position.xyww;
	outTexCoord = texCoord;
}
