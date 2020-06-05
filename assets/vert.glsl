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

void main() {
    TexCoord = VertexTexCoord;

    vec4 pos = vec4(0,0,0,0);

	if (Animated) {
		for (int i=0; i<4; ++i) {
			int bone_i = int(VertexBlendIndices[i]);
			float weight = VertexBlendWeights[i];
			mat4 fmat = Bones[bone_i];
			vec4 p = fmat * vec4(VertexPosition, 1.0);
			p = p * weight;
			pos += p;
		}
    } else {
        pos = vec4(VertexPosition, 1.0);
    }

    gl_Position = MVP * pos;
}
