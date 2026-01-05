#pragma once

#include <GL/glew.h>

#include "utils.h"

class DistributionRenderer {
public:
  DistributionRenderer();
  ~DistributionRenderer();

  void Render(Viewport particleViewport, Viewport pixelViewport);

private:
  GLuint m_quadVAO;
  GLuint m_quadVBO;

  GLuint m_vertShader;
  GLuint m_fragShader;
  GLuint m_program;

  GLint m_minUniform;
  GLint m_maxUniform;
};
