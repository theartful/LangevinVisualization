#include "particle_renderer.h"

#include "particle.frag.h"
#include "particle.vert.h"
#include "utils.h"
#include <cstdio>

ParticleRenderer::ParticleRenderer() {
  // Create VAO and VBO
  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  // Create shaders
  {
    m_vertShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *src = (const GLchar *)ParticleVert;
    const GLsizei len = ParticleVert_len;
    glShaderSource(m_vertShader, 1, &src, &len);
    glCompileShader(m_vertShader);
    CheckCompilationResult(m_vertShader);
  }

  {
    m_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *src = (const GLchar *)ParticleFrag;
    const GLsizei len = ParticleFrag_len;
    glShaderSource(m_fragShader, 1, &src, &len);
    glCompileShader(m_fragShader);
    CheckCompilationResult(m_fragShader);
  }

  m_program = glCreateProgram();
  glAttachShader(m_program, m_vertShader);
  glAttachShader(m_program, m_fragShader);
  glLinkProgram(m_program);

  m_particlesUniform = glGetUniformLocation(m_program, "uParticles");
  m_particlesWidthUniform = glGetUniformLocation(m_program, "uParticlesWidth");
  m_minUniform = glGetUniformLocation(m_program, "uMin");
  m_maxUniform = glGetUniformLocation(m_program, "uMax");

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleRenderer::Render(Viewport viewport, int width, int height,
                              int particlesWidth, int particlesHeight,
                              GLuint particlesTexture) {
  glViewport(0, 0, width, height);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(m_vao);
  glUseProgram(m_program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, particlesTexture);
  glUniform1i(m_particlesUniform, 0);

  glUniform1i(m_particlesWidthUniform, particlesWidth);
  glUniform2f(m_minUniform, viewport.pmin.x, viewport.pmin.y);
  glUniform2f(m_maxUniform, viewport.pmax.x, viewport.pmax.y);

  glDrawArrays(GL_POINTS, 0, particlesWidth * particlesHeight);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}
