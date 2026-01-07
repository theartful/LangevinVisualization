#version 300 es
precision highp float;

uniform sampler2D uParticles;
uniform int uParticlesWidth;

// Viewport
uniform vec2 uMin;
uniform vec2 uMax;

void main() {
  ivec2 pixel =
      ivec2(gl_VertexID % uParticlesWidth, gl_VertexID / uParticlesWidth);

  vec2 pos = texelFetch(uParticles, pixel, 0).xy;
  pos = 2.0 * (pos - uMin) / (uMax - uMin) - 1.0;

  gl_PointSize = 1.0;
  gl_Position = vec4(pos, 0.0, 1.0);
}
