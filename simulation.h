#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

#include "mixture.h"

class Simulation {
public:
  Simulation();
  ~Simulation();

  void Update();
  void SetMixture(const MixtureOfGaussians &m);

  size_t Width();
  size_t Height();
  size_t NumParticles();
  GLuint ParticlesTexture();

private:
  void InitializeParticles();

private:
  GLuint m_quadVAO;
  GLuint m_quadVBO;

  GLuint m_vertShader;
  GLuint m_fragShader;
  GLuint m_program;

  GLuint m_fbos[2];
  GLuint m_colors[2];

  int m_frameIdUniform;
  int m_particlesUniform;
  int m_step;
  std::vector<glm::vec2> m_particles;

  GLuint m_mogUBO;
};
