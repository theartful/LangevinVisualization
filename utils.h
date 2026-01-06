#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <stdexcept>
#include <string>

struct Viewport {
  glm::vec2 pmin;
  glm::vec2 pmax;

  float Width() { return pmax.x - pmin.x; }
  float Height() { return pmax.y - pmin.y; }

  glm::vec2 Center() { return 0.5f * (pmin + pmax); }
};

static Viewport EnforceAspectRatio(Viewport particleViewport,
                                   Viewport pixelViewport) {
  const float width = pixelViewport.Width();
  const float height = pixelViewport.Height();

  Viewport result = particleViewport;

  const glm::vec2 center = particleViewport.Center();

  if (width > height) {
    const float aspect_ratio = width / height;
    const float correct_width = particleViewport.Height() * aspect_ratio;
    result.pmin.x = center.x - correct_width / 2.0;
    result.pmax.x = center.x + correct_width / 2.0;
  } else {
    const float aspect_ratio = height / width;
    const float correct_height = particleViewport.Width() * aspect_ratio;
    result.pmin.y = center.y - correct_height / 2.0;
    result.pmax.y = center.y + correct_height / 2.0;
  }

  return result;
}

static void CheckCompilationResult(GLuint shader) {
  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success != GL_TRUE) {
    int bufflen;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &bufflen);
    if (bufflen > 1) {
      std::string log;
      log.resize(bufflen + 1);
      glGetShaderInfoLog(shader, bufflen, 0, log.data());
      throw std::runtime_error(log);
    }
  }
}
