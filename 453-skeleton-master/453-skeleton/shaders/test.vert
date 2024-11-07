#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;

out vec2 tc;
uniform mat4 Matrx;

void main() {
	tc = texCoord;
	gl_Position = Matrx * vec4(pos, 1.0);
}
