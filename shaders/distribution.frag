#version 300 es
precision highp float;

layout(location = 0) out vec4 FragColor;
in vec2 aXY;

#define TWO_PI 6.283185307179586

struct Gaussian { vec2 mean; vec2 sigma; };
layout(std140) uniform MixtureBlock {
  int uCount;
  float uPeak;
  Gaussian uGaussians[10];
};

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

float gaussian(vec2 pos, vec2 mean, vec2 sigma) {
  vec2 d = pos - mean;
  vec2 s2 = sigma * sigma;
  float expo = -0.5 * (d.x * d.x / s2.x + d.y * d.y / s2.y);
  return exp(expo) / (TWO_PI * sigma.x * sigma.y);
}

float mixture_of_gaussians(vec2 pos) {
  if (uCount <= 0) return 0.0;
  float sum = 0.0;
  for (int i = 0; i < uCount; ++i) {
    sum += gaussian(pos, uGaussians[i].mean, uGaussians[i].sigma);
  }
  return sum / float(uCount);
}

void main() {
  float peak = uPeak;
  float prob = mixture_of_gaussians(aXY);

  // Gamma correction style
  float t = pow(prob / max(peak, 1e-8), 0.5);

  FragColor = vec4(colormap(t), 1.0);
}
