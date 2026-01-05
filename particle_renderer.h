#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "utils.h"

class ParticleRenderer {
public:
  ParticleRenderer();

  void Render(Viewport particleViewport, Viewport pixelViewport,
              int particlesWidth, int particlesHeight, GLuint particlesTexture);

private:
  GLuint m_vao;
  GLuint m_vertShader;
  GLuint m_fragShader;
  GLuint m_program;

  GLint m_particlesUniform;
  GLint m_particlesWidthUniform;
  GLint m_minUniform;
  GLint m_maxUniform;
};
