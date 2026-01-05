#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "utils.h"
#include "mixture.h"

class EstimatedDistributionRenderer {
public:
  EstimatedDistributionRenderer();
  ~EstimatedDistributionRenderer();

  void Render(Viewport particleViewport, Viewport pixelViewport,
              int particlesWidth, int particlesHeight, GLuint particlesTexture);
  void SetMixture(const MixtureOfGaussians &m);

private:
  void CreateAccumulatorProgram();
  void CreateRendererProgram();

  void Accumulate(Viewport particleViewport, int particlesWidth,
                  int particlesHeight, GLuint particlesTexture);
  void DoRender(Viewport particleViewport, Viewport pixelViewport,
                int numParticles);

private:
  // Accumulator
  GLuint m_accumVAO;
  GLuint m_accumVertShader;
  GLuint m_accumFragShader;
  GLuint m_accumProgram;
  GLuint m_fbo;
  GLuint m_color;

  GLint m_accumParticlesUniform;
  GLint m_accumParticlesWidthUniform;
  GLint m_accumMinUniform;
  GLint m_accumMaxUniform;

  // Renderer
  GLuint m_renderQuadVAO;
  GLuint m_renderQuadVBO;

  GLuint m_renderVertShader;
  GLuint m_renderFragShader;
  GLuint m_renderProgram;

  GLint m_renderMinUniform;
  GLint m_renderMaxUniform;
  GLint m_renderAccumUniform;
  GLint m_renderNumParticlesUniform;
  GLint m_renderAreaUniform;
  GLint m_renderPeakUniform;
};
