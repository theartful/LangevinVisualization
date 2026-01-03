#version 330

layout(location = 0) out vec2 ParticlePosition;

uniform sampler2D uParticles;
uniform uint uFrameId;

#define TWO_PI 6.283185307179586
#define PI

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
  float a = standardDeviation * sqrt(-2.0f * log(1.0f - u.x));
  float b = TWO_PI * u.y;

  return vec2(cos(b), sin(b)) * a + mean;
}

vec2 gaussian_score(vec2 pos, vec2 mean, float sigma) {
  return (mean - pos) / (sigma * sigma);
}

float gaussian(vec2 pos, vec2 mean, float sigma) {
  return (1.0 / (TWO_PI * sigma * sigma)) *
         exp(-dot(pos - mean, pos - mean) / (2 * sigma * sigma));
}

vec2 mixture_of_gaussian_score(vec2 pos) {
  vec2 means[4] = vec2[](
      vec2(-0.5, -0.5),
      vec2(0.5, 0.5),
      vec2(-0.5, 0.5),
      vec2(0.5, -0.5)
  );
  float sigmas[4] = float[](0.1, 0.1, 0.1, 0.1);

  float probs[4] = float[](
      gaussian(pos, means[0], sigmas[0]),
      gaussian(pos, means[1], sigmas[1]),
      gaussian(pos, means[2], sigmas[2]),
      gaussian(pos, means[3], sigmas[3])
  );

  float sum_probs = probs[0] + probs[1] + probs[2] + probs[3];
  return (gaussian_score(pos, means[0], sigmas[0]) * probs[0] +
          gaussian_score(pos, means[1], sigmas[1]) * probs[1] +
          gaussian_score(pos, means[2], sigmas[2]) * probs[2] +
          gaussian_score(pos, means[3], sigmas[3]) * probs[3]) /
         sum_probs;
}

void main() {
  seed(uFrameId);

  vec2 pos = texelFetch(uParticles, ivec2(gl_FragCoord.xy), 0).xy;

  float dt = 0.00004;

  vec2 u = vec2(lcg_randomf(), lcg_randomf());
  vec2 w = vec2(sample_gaussian(u, 0, 1.0));

  ParticlePosition = pos + dt * mixture_of_gaussian_score(pos) + sqrt(2 * dt) * w;
}
