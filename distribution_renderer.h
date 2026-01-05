#pragma once

#include <GL/glew.h>

#include "utils.h"
#include "mixture.h"

class DistributionRenderer {
public:
  DistributionRenderer();
  ~DistributionRenderer();

  void Render(Viewport particleViewport, Viewport pixelViewport);
  void SetMixture(const MixtureOfGaussians &m);

private:
  GLuint m_quadVAO;
  GLuint m_quadVBO;

  GLuint m_vertShader;
  GLuint m_fragShader;
  GLuint m_program;

  GLint m_minUniform;
  GLint m_maxUniform;

  GLuint m_mogUBO;
};
