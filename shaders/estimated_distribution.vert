#version 330

layout(location = 0) in vec2 aPos;
out vec2 aUV;

uniform vec2 uMin;
uniform vec2 uMax;

void main() {
  aUV = (aPos + 1.0) / 2.0;
  gl_Position = vec4(aPos, 0.0, 1.0);
}
