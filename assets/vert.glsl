#version 410

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 VertexTexCoord;
layout(location = 2) in vec3 VertexNormal;
layout(location = 3) in vec3 VertexIndices;
layout(location = 4) in vec3 VertexWeights;

uniform mat4 MVP;
uniform bool Animated;
uniform mat4 Bones[32];

out vec2 TexCoord;

void main() {
    TexCoord = VertexTexCoord;

    vec4 pos = vec4(0,0,0,0);

	if (Animated) {
		for (int i=0; i<1; ++i) {
			float weight = VertexWeights[i];
			int bone_i = int(VertexIndices[i]);
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
