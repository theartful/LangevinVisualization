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
};

static Viewport EnforceAspectRatio(Viewport particleViewport,
                                   Viewport pixelViewport) {
  const float width = pixelViewport.Width();
  const float height = pixelViewport.Height();

  Viewport result = particleViewport;

  if (width > height) {
    const float aspect_ratio = width / height;
    result.pmin *= glm::vec2(aspect_ratio, 1.0);
    result.pmax *= glm::vec2(aspect_ratio, 1.0);
  } else {
    const float aspect_ratio = height / width;
    result.pmin *= glm::vec2(1.0, aspect_ratio);
    result.pmax *= glm::vec2(1.0, aspect_ratio);
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
