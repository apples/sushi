#version 330

in vec3 VertexPosition;
in vec2 VertexTexCoord;
in vec3 VertexNormal;

uniform mat4 MVP;

out vec2 TexCoord;

void main() {

    TexCoord = VertexTexCoord;

    gl_Position = MVP * vec4(VertexPosition, 1.0);
}
