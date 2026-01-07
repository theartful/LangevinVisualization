#include "distribution_renderer.h"

#include "distribution.frag.h"
#include "distribution.vert.h"
#include "mixture.h"

DistributionRenderer::DistributionRenderer() {
  // Create VAO and VBO
  glGenVertexArrays(1, &m_quadVAO);
  glBindVertexArray(m_quadVAO);

  glGenBuffers(1, &m_quadVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
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
    m_vertShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *src = (const GLchar *)DistributionVert;
    const GLsizei len = DistributionVert_len;
    glShaderSource(m_vertShader, 1, &src, &len);
    glCompileShader(m_vertShader);
    CheckCompilationResult(m_vertShader, "distribution.vert");
  }

  {
    m_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *src = (const GLchar *)DistributionFrag;
    const GLsizei len = DistributionFrag_len;
    glShaderSource(m_fragShader, 1, &src, &len);
    glCompileShader(m_fragShader);
    CheckCompilationResult(m_fragShader, "distribution.frag");
  }

  m_program = glCreateProgram();
  glAttachShader(m_program, m_vertShader);
  glAttachShader(m_program, m_fragShader);
  glLinkProgram(m_program);

  m_minUniform = glGetUniformLocation(m_program, "uMin");
  m_maxUniform = glGetUniformLocation(m_program, "uMax");

  // Bind uniform block index to binding 0
  GLuint blockIdx = glGetUniformBlockIndex(m_program, "MixtureBlock");
  if (blockIdx != GL_INVALID_INDEX) {
    glUniformBlockBinding(m_program, blockIdx, 0);
  }

  // Create Mixture UBO
  glGenBuffers(1, &m_mogUBO);
  glBindBuffer(GL_UNIFORM_BUFFER, m_mogUBO);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(MixtureOfGaussians), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_mogUBO);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DistributionRenderer::SetMixture(const MixtureOfGaussians &m) {
  glBindBuffer(GL_UNIFORM_BUFFER, m_mogUBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MixtureOfGaussians), &m);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void DistributionRenderer::Render(Viewport particleViewport,
                                  Viewport pixelViewport) {
  glViewport(pixelViewport.pmin.x, pixelViewport.pmin.y, pixelViewport.Width(),
             pixelViewport.Height());

#ifndef EMSCRIPTEN
  glEnable(GL_PROGRAM_POINT_SIZE);
#endif
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(m_quadVAO);
  glUseProgram(m_program);

  glUniform2f(m_minUniform, particleViewport.pmin.x, particleViewport.pmin.y);
  glUniform2f(m_maxUniform, particleViewport.pmax.x, particleViewport.pmax.y);

  // Bind mixture UBO at binding=0
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_mogUBO);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
  glUseProgram(0);
}

DistributionRenderer::~DistributionRenderer() {
  glDeleteVertexArrays(1, &m_quadVAO);
  glDeleteBuffers(1, &m_quadVBO);
  glDeleteProgram(m_program);
  glDeleteShader(m_vertShader);
  glDeleteShader(m_fragShader);
  glDeleteBuffers(1, &m_mogUBO);
}
