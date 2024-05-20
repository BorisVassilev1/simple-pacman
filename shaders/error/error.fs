precision highp float;

in vec3 vVertexPos;

out vec4 fragColor;

uniform sampler2D texture_sampler;

void main() {
	fragColor = vec4(1, 0, 1, 1.0);
}
