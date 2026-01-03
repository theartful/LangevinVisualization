#include "simulation.h"

#include "simulation.frag.h"
#include "simulation.vert.h"
#include "utils.h"

#include <cstdlib>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

static constexpr size_t kWidth = 1920;
static constexpr size_t kHeight = 1080;
static constexpr size_t kNumParticles = kWidth * kHeight;

Simulation::Simulation() : m_step(0) {
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
    const GLchar *src = (const GLchar *)SimulationVert;
    const GLsizei len = SimulationVert_len;
    glShaderSource(m_vertShader, 1, &src, &len);
    glCompileShader(m_vertShader);
    CheckCompilationResult(m_vertShader);
  }

  {
    m_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *src = (const GLchar *)SimulationFrag;
    const GLsizei len = SimulationFrag_len;
    glShaderSource(m_fragShader, 1, &src, &len);
    glCompileShader(m_fragShader);
    CheckCompilationResult(m_fragShader);
  }

  m_program = glCreateProgram();
  glAttachShader(m_program, m_vertShader);
  glAttachShader(m_program, m_fragShader);
  glLinkProgram(m_program);

  m_frameIdUniform = glGetUniformLocation(m_program, "uFrameId");
  m_particlesUniform = glGetUniformLocation(m_program, "uParticles");

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  InitializeParticles();

  // Create framebuffers
  glGenFramebuffers(2, m_fbos);
  glGenTextures(2, m_colors);

  for (int i = 0; i < 2; i++) {
    glBindTexture(GL_TEXTURE_2D, m_colors[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, kWidth, kHeight, 0, GL_RG,
                 GL_FLOAT, glm::value_ptr(m_particles[0]));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbos[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_colors[i], 0);

    const GLenum bufs[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, bufs);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("Error creating framebuffer");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void Simulation::InitializeParticles() {
  m_particles.reserve(kNumParticles);

  for (int i = 0; i < kWidth; i++) {
    const float x = 2.0 * (static_cast<float>(i) / (kWidth - 1.0)) - 1.0;
    for (int j = 0; j < kHeight; j++) {
      const float y = 2.0 * (static_cast<float>(j) / (kHeight - 1.0)) - 1.0;
      m_particles.push_back({x, y});
    }
  }
}

void Simulation::Update() {
  m_step++;

  const int bing = m_step % 2;
  const int bong = 1 - bing;

  glViewport(0, 0, kWidth, kHeight);
  glDisable(GL_BLEND);
  glBindVertexArray(m_quadVAO);
  glUseProgram(m_program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_colors[bing]);
  glUniform1i(m_particlesUniform, 0);

  glUniform1ui(m_frameIdUniform, m_step);

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbos[bong]);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Simulation::~Simulation() {
  glDeleteVertexArrays(1, &m_quadVAO);
  glDeleteBuffers(1, &m_quadVBO);
  glDeleteProgram(m_program);
  glDeleteShader(m_vertShader);
  glDeleteShader(m_fragShader);
}

size_t Simulation::Width() { return kWidth; }
size_t Simulation::Height() { return kHeight; }
size_t Simulation::NumParticles() { return kNumParticles; }
GLuint Simulation::ParticlesTexture() {
  const int bing = m_step % 2;
  const int bong = 1 - bing;

  return m_colors[bong];
}
