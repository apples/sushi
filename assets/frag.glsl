#version 330

in vec2 TexCoord;

uniform sampler2D DiffuseTexture;

out vec4 FragColor;

void main() {
    FragColor = texture(DiffuseTexture, TexCoord);
}
