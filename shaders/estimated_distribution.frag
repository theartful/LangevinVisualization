#version 330

layout(location = 0) out vec4 FragColor;
in vec2 aUV;

uniform sampler2D uAccum;
uniform int uNumParticles;
uniform float uArea;

#define TWO_PI 6.283185307179586

vec3 colormap(float x) {
  vec4 kRedVec4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
  vec4 kGreenVec4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
  vec4 kBlueVec4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
  vec2 kRedVec2 = vec2(-152.94239396, 59.28637943);
  vec2 kGreenVec2 = vec2(4.27729857, 2.82956604);
  vec2 kBlueVec2 = vec2(-89.90310912, 27.34824973);
  
  x = clamp(x, 0.0, 1.0);
  vec4 v4 = vec4( 1.0, x, x * x, x * x * x);
  vec2 v2 = v4.zw * v4.z;
  return vec3(
    dot(v4, kRedVec4)   + dot(v2, kRedVec2),
    dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
    dot(v4, kBlueVec4)  + dot(v2, kBlueVec2)
  );
}

float gaussian(vec2 pos, vec2 mean, float sigma) {
  return (1.0 / (TWO_PI * sigma * sigma)) *
         exp(-dot(pos - mean, pos - mean) / (2 * sigma * sigma));
}

float mixture_of_gaussians(vec2 pos) {
  vec2 means[4] = vec2[](vec2(-0.5, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5),
                         vec2(0.5, -0.5));

  float sigmas[4] = float[](0.1, 0.1, 0.1, 0.1);

  return 0.25 * (gaussian(pos, means[0], sigmas[0]) +
                 gaussian(pos, means[1], sigmas[1]) +
                 gaussian(pos, means[2], sigmas[2]) +
                 gaussian(pos, means[3], sigmas[3]));
}

void main() {
  float peak = mixture_of_gaussians(vec2(0.5, 0.5));
  float numParticles = texture(uAccum, aUV).r;
  float prob = numParticles / (uNumParticles * uArea);

  // Gamma correction style
  float t = pow(prob / peak, 0.5);

  FragColor = vec4(colormap(t), 1.0);
}
