#version 410

in vec3 VertexPosition;
in vec2 VertexTexCoord;
in vec3 VertexNormal;
in vec4 VertexBlendIndices;
in vec4 VertexBlendWeights;

uniform mat4 MVP;
uniform bool Animated;
uniform mat4 Bones[32];

out vec2 TexCoord;
out vec3 Normal;

void main() {
    mat4 transform = MVP;

	if (Animated) {
        transform =
            MVP * (
                Bones[int(VertexBlendIndices[0])] * VertexBlendWeights[0] +
                Bones[int(VertexBlendIndices[1])] * VertexBlendWeights[1] +
                Bones[int(VertexBlendIndices[2])] * VertexBlendWeights[2] +
                Bones[int(VertexBlendIndices[3])] * VertexBlendWeights[3]);
    }

    TexCoord = VertexTexCoord;
    Normal = vec3(transpose(inverse(transform)) * vec4(VertexNormal, 0.0));
    gl_Position = transform * vec4(VertexPosition, 1.0);
}
