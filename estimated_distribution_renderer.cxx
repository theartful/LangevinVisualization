#include "estimated_distribution_renderer.h"

#include "accumulator.frag.h"
#include "accumulator.vert.h"
#include "estimated_distribution.frag.h"
#include "estimated_distribution.vert.h"
#include "mixture.h"
#include "utils.h"

static constexpr size_t kWidth = 200;
static constexpr size_t kHeight = 200;

EstimatedDistributionRenderer::EstimatedDistributionRenderer() {
  CreateAccumulatorProgram();
  CreateRendererProgram();
}

void EstimatedDistributionRenderer::CreateAccumulatorProgram() {
  // Create VAO and VBO
  glGenVertexArrays(1, &m_accumVAO);
  glBindVertexArray(m_accumVAO);

  // Create shaders
  {
    m_accumVertShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *src = (const GLchar *)AccumulatorVert;
    const GLsizei len = AccumulatorVert_len;
    glShaderSource(m_accumVertShader, 1, &src, &len);
    glCompileShader(m_accumVertShader);
    CheckCompilationResult(m_accumVertShader, "accumulator.vert");
  }

  {
    m_accumFragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *src = (const GLchar *)AccumulatorFrag;
    const GLsizei len = AccumulatorFrag_len;
    glShaderSource(m_accumFragShader, 1, &src, &len);
    glCompileShader(m_accumFragShader);
    CheckCompilationResult(m_accumFragShader, "accumulator.frag");
  }

  m_accumProgram = glCreateProgram();
  glAttachShader(m_accumProgram, m_accumVertShader);
  glAttachShader(m_accumProgram, m_accumFragShader);
  glLinkProgram(m_accumProgram);

  m_accumParticlesUniform = glGetUniformLocation(m_accumProgram, "uParticles");
  m_accumParticlesWidthUniform =
      glGetUniformLocation(m_accumProgram, "uParticlesWidth");
  m_accumMinUniform = glGetUniformLocation(m_accumProgram, "uMin");
  m_accumMaxUniform = glGetUniformLocation(m_accumProgram, "uMax");

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Create framebuffers
  glGenFramebuffers(1, &m_fbo);
  glGenTextures(1, &m_color);

  glBindTexture(GL_TEXTURE_2D, m_color);
  // FIXME: Why doesn't this work
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, kWidth, kHeight, 0, GL_R, GL_FLOAT,
  // nullptr);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, kWidth, kHeight, 0, GL_RG, GL_FLOAT,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_color, 0);

  const GLenum bufs[] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, bufs);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("Error creating framebuffer");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EstimatedDistributionRenderer::CreateRendererProgram() {
  // Create VAO and VBO
  glGenVertexArrays(1, &m_renderQuadVAO);
  glBindVertexArray(m_renderQuadVAO);

  glGenBuffers(1, &m_renderQuadVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_renderQuadVBO);
  const float quad[] = {
      -1.0, -1.0, //
      1.0,  1.0,  //
      1.0,  -1.0, //

      -1.0, -1.0, //
      1.0,  1.0,  //
      -1.0, 1.0,  //
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  // Create shaders
  {
    m_renderVertShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *src = (const GLchar *)EstimatedDistributionVert;
    const GLsizei len = EstimatedDistributionVert_len;
    glShaderSource(m_renderVertShader, 1, &src, &len);
    glCompileShader(m_renderVertShader);
    CheckCompilationResult(m_renderVertShader, "estimated_distribution.vert");
  }

  {
    m_renderFragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *src = (const GLchar *)EstimatedDistributionFrag;
    const GLsizei len = EstimatedDistributionFrag_len;
    glShaderSource(m_renderFragShader, 1, &src, &len);
    glCompileShader(m_renderFragShader);
    CheckCompilationResult(m_renderFragShader, "estimated_distribution.frag");
  }

  m_renderProgram = glCreateProgram();
  glAttachShader(m_renderProgram, m_renderVertShader);
  glAttachShader(m_renderProgram, m_renderFragShader);
  glLinkProgram(m_renderProgram);

  m_renderMinUniform = glGetUniformLocation(m_renderProgram, "uMin");
  m_renderMaxUniform = glGetUniformLocation(m_renderProgram, "uMax");
  m_renderAccumUniform = glGetUniformLocation(m_renderProgram, "uAccum");
  m_renderNumParticlesUniform =
      glGetUniformLocation(m_renderProgram, "uNumParticles");
  m_renderAreaUniform = glGetUniformLocation(m_renderProgram, "uArea");
  m_renderPeakUniform = glGetUniformLocation(m_renderProgram, "uPeak");

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void EstimatedDistributionRenderer::Render(Viewport particleViewport,
                                           Viewport pixelViewport,
                                           int particlesWidth,
                                           int particlesHeight,
                                           GLuint particlesTexture) {
  Accumulate(particleViewport, particlesWidth, particlesHeight,
             particlesTexture);

  DoRender(particleViewport, pixelViewport, particlesWidth * particlesHeight);
}

void EstimatedDistributionRenderer::SetMixture(const MixtureOfGaussians &m) {
  glUseProgram(m_renderProgram);
  glUniform1f(m_renderPeakUniform, m.peak);
  glUseProgram(0);
}

void EstimatedDistributionRenderer::Accumulate(Viewport particleViewport,
                                               int particlesWidth,
                                               int particlesHeight,
                                               GLuint particlesTexture) {
  // Accumulate
  glViewport(0, 0, kWidth, kHeight);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  glBindVertexArray(m_accumVAO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glUseProgram(m_accumProgram);

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, particlesTexture);
  glUniform1i(m_accumParticlesUniform, 0);

  glUniform1i(m_accumParticlesWidthUniform, particlesWidth);
  glUniform2f(m_accumMinUniform, particleViewport.pmin.x,
              particleViewport.pmin.y);
  glUniform2f(m_accumMaxUniform, particleViewport.pmax.x,
              particleViewport.pmax.y);

  glDrawArrays(GL_POINTS, 0, particlesWidth * particlesHeight);

  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EstimatedDistributionRenderer::DoRender(Viewport particleViewport,
                                             Viewport pixelViewport,
                                             int numParticles) {
  glViewport(pixelViewport.pmin.x, pixelViewport.pmin.y, pixelViewport.Width(),
             pixelViewport.Height());

  glDisable(GL_BLEND);

  glBindVertexArray(m_renderQuadVAO);
  glUseProgram(m_renderProgram);

  glUniform2f(m_renderMinUniform, particleViewport.pmin.x,
              particleViewport.pmin.y);
  glUniform2f(m_renderMaxUniform, particleViewport.pmax.x,
              particleViewport.pmax.y);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_color);
  glUniform1i(m_renderAccumUniform, 0);

  glUniform1i(m_renderNumParticlesUniform, numParticles);
  glUniform1f(m_renderAreaUniform, (particleViewport.Width() / kWidth) *
                                       (particleViewport.Height() / kHeight));

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}

EstimatedDistributionRenderer::~EstimatedDistributionRenderer() {
  // Accumulator
  glDeleteVertexArrays(1, &m_accumVAO);
  glDeleteProgram(m_accumProgram);
  glDeleteShader(m_accumVertShader);
  glDeleteShader(m_accumFragShader);

  // Renderer
  glDeleteVertexArrays(1, &m_renderQuadVAO);
  glDeleteBuffers(1, &m_renderQuadVBO);
  glDeleteProgram(m_renderProgram);
  glDeleteShader(m_renderVertShader);
  glDeleteShader(m_renderFragShader);
}
