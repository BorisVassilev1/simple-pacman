#version 430
#include <rendering.glsl>

out vec3 vTexCoord;
out vec3 vVertexPos;

uniform mat4 worldMatrix;

void main() {
    // правим въртене защото z е нагоре
    vec3 direction = vec3(normalize(position));
    // правим само въртенето от матрицата на изгледа
    vec4 vertexPos = viewMatrix * vec4(direction, 0.0);
    
    // // и проектираме нормално
    vertexPos.w = 1.0;
    vertexPos = projectionMatrix * vertexPos;
    gl_Position = vec4(vertexPos.xyww); // z = 1.
    vTexCoord = position;
    vVertexPos = vertexPos.xyz;
}