#pragma once

#include <glm/glm.hpp>
#include <cmath>

struct alignas(16) Gaussian {
  glm::vec2 mean;
  glm::vec2 sigma;

  inline float Evaluate(const glm::vec2 &p) const {
    // Anisotropic 2D Gaussian PDF with diagonal covariance (sigma.x, sigma.y)
    const glm::vec2 d = p - mean;
    const glm::vec2 s2 = sigma * sigma;
    const float q = glm::dot((d * d) / s2, glm::vec2(1.0f)); // sum of components
    const float denom = 6.283185307179586f * sigma.x * sigma.y; // 2*pi*sx*sy
    return std::exp(-0.5f * q) / denom;
  }
};

struct MixtureOfGaussians {
  int count;
  float peak;
  Gaussian g[10];

  inline void UpdatePeak() {
    float max_val = 0.0f;
    if (count <= 0) { peak = 0.0f; return; }

    for (int i = 0; i < count; ++i) {
      const glm::vec2 mu = g[i].mean;
      float mixture_at_mu = 0.0f;
      for (int j = 0; j < count; ++j) {
        mixture_at_mu += g[j].Evaluate(mu);
      }
      mixture_at_mu /= static_cast<float>(count);
      if (mixture_at_mu > max_val) max_val = mixture_at_mu;
    }
    peak = max_val;
  }
};
