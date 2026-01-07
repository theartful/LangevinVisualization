#pragma once

#include <cmath>
#include <glm/glm.hpp>

struct alignas(16) Gaussian {
  glm::vec2 mean = {0.0, 0.0};
  glm::vec2 sigma = {0.1, 0.1};

  inline float Evaluate(const glm::vec2 &p) const {
    static constexpr float kTwoPi = 6.283185307179586f;

    const glm::vec2 d = p - mean;
    const float q = glm::dot((d * d) / (sigma * sigma), glm::vec2(1.0f));
    const float denom = kTwoPi * sigma.x * sigma.y; // 2*pi*sx*sy
    return std::exp(-0.5f * q) / denom;
  }
};

struct MixtureOfGaussians {
  int count;
  float peak;
  Gaussian g[10];

  inline void UpdatePeak() {
    float max_val = 0.0f;
    if (count <= 0) {
      peak = 0.0f;
      return;
    }

    for (int i = 0; i < count; ++i) {
      const glm::vec2 mu = g[i].mean;
      float mixture_at_mu = 0.0f;
      for (int j = 0; j < count; ++j) {
        mixture_at_mu += g[j].Evaluate(mu);
      }
      mixture_at_mu /= static_cast<float>(count);
      max_val = std::max(max_val, mixture_at_mu);
    }
    peak = max_val;
  }
};
