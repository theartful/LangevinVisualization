#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

struct Viewport {
  glm::vec2 pmin;
  glm::vec2 pmax;
};

class ParticleRenderer {
public:
  ParticleRenderer();

  void Render(Viewport viewport, int width, int height, int particlesWidth,
              int particlesHeight, GLuint particlesTexture);

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
