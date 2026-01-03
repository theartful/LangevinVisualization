#pragma once

#include <GL/glew.h>

#include <stdexcept>
#include <string>

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
