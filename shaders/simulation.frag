#version 300 es
precision highp float;

layout(location = 0) out vec2 ParticlePosition;

uniform sampler2D uParticles;
uniform uint uFrameId;
uniform float uDt;

#define TWO_PI 6.283185307179586

struct Gaussian {
  vec2 mean;
  vec2 sigma;
};
layout(std140) uniform MixtureBlock {
  int uCount;
  float uPeak;
  Gaussian uGaussians[10];
};

uint rng = 0u;

uint murmur_hash3_mix(uint hash, uint k) {
  uint c1 = 0xcc9e2d51u;
  uint c2 = 0x1b873593u;
  uint r1 = 15u;
  uint r2 = 13u;
  uint m = 5u;
  uint n = 0xe6546b64u;

  k *= c1;
  k = (k << r1) | (k >> (32u - r1));
  k *= c2;

  hash ^= k;
  hash = ((hash << r2) | (hash >> (32u - r2))) * m + n;

  return hash;
}

uint murmur_hash3_finalize(uint hash) {
  hash ^= hash >> 16u;
  hash *= 0x85ebca6bu;
  hash ^= hash >> 13u;
  hash *= 0xc2b2ae35u;
  hash ^= hash >> 16u;

  return hash;
}

uint lcg_random() {
  uint m = 1664525u;
  uint n = 1013904223u;
  rng = rng * m + n;
  return rng;
}

float ldexp(float x, int exp) { return x * exp2(float(exp)); }

float lcg_randomf() { return ldexp(float(lcg_random()), -32); }

void seed(uint frame_id) {
  uvec2 pixel = uvec2(gl_FragCoord);
  uvec2 dims = uvec2(1920u, 1080u);

  rng = murmur_hash3_mix(0u, uint(pixel.x + pixel.y * dims.x));
  rng = murmur_hash3_mix(rng, frame_id);
  rng = murmur_hash3_finalize(rng);
}

vec2 sample_gaussian(vec2 u, float mean, float standardDeviation) {
  float a = standardDeviation * sqrt(-2.0 * log(1.0 - u.x));
  float b = TWO_PI * u.y;

  return vec2(cos(b), sin(b)) * a + mean;
}

vec2 gaussian_score(vec2 pos, vec2 mean, vec2 sigma) {
  return (mean - pos) / (sigma * sigma);
}

float gaussian(vec2 pos, vec2 mean, vec2 sigma, float exponent_offset) {
  vec2 d = (pos - mean) / sigma;
  return exp(-0.5 * dot(d, d) + exponent_offset) / (TWO_PI * sigma.x * sigma.y);
}

vec2 mixture_of_gaussian_score(vec2 pos) {
  float wsum = 0.0;
  vec2 num = vec2(0.0);

  float max_e = -1e30;
  for (int i = 0; i < uCount; ++i) {
    vec2 d = (pos - uGaussians[i].mean) / uGaussians[i].sigma;
    float e = -0.5 * dot(d, d);
    max_e = max(max_e, e);
  }

  for (int i = 0; i < uCount; ++i) {
    float w = gaussian(pos, uGaussians[i].mean, uGaussians[i].sigma, -max_e);
    num += w * gaussian_score(pos, uGaussians[i].mean, uGaussians[i].sigma);
    wsum += w;
  }
  return (wsum > 0.0) ? num / wsum : vec2(0.0);
}

void main() {
  seed(uFrameId);

  vec2 pos = texelFetch(uParticles, ivec2(gl_FragCoord.xy), 0).xy;

  float dt = uDt;

  vec2 u = vec2(lcg_randomf(), lcg_randomf());
  vec2 w = sample_gaussian(u, 0.0, 1.0);

  ParticlePosition =
      pos + dt * mixture_of_gaussian_score(pos) + sqrt(2.0 * dt) * w;
}
