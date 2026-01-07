cmake_minimum_required(VERSION 3.20)
project(Langevin)

if (NOT EMSCRIPTEN)
  # Emscripten: use SDL2 via ports, WebGL2 via flags
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  find_package(SDL2 REQUIRED)
endif()

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY  https://github.com/g-truc/glm.git
    GIT_TAG         1.0.3
    GIT_SHALLOW     1
)
FetchContent_MakeAvailable(glm)

add_library(imgui
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/backends/imgui_impl_sdl2.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

if (EMSCRIPTEN)
  target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)
else()
  target_link_libraries(imgui PUBLIC OpenGL::OpenGL SDL2::SDL2)
endif()
target_include_directories(imgui PUBLIC third_party/imgui/ third_party/imgui/backends)

add_custom_command(
    OUTPUT   ${CMAKE_BINARY_DIR}/shaders/simulation.frag.h
             ${CMAKE_BINARY_DIR}/shaders/simulation.vert.h
             ${CMAKE_BINARY_DIR}/shaders/particle.frag.h
             ${CMAKE_BINARY_DIR}/shaders/particle.vert.h
             ${CMAKE_BINARY_DIR}/shaders/distribution.frag.h
             ${CMAKE_BINARY_DIR}/shaders/distribution.vert.h
             ${CMAKE_BINARY_DIR}/shaders/accumulator.frag.h
             ${CMAKE_BINARY_DIR}/shaders/accumulator.vert.h
             ${CMAKE_BINARY_DIR}/shaders/estimated_distribution.frag.h
             ${CMAKE_BINARY_DIR}/shaders/estimated_distribution.vert.h
    DEPENDS  ${CMAKE_SOURCE_DIR}/shaders/simulation.frag
             ${CMAKE_SOURCE_DIR}/shaders/simulation.vert
             ${CMAKE_SOURCE_DIR}/shaders/particle.frag
             ${CMAKE_SOURCE_DIR}/shaders/particle.vert
             ${CMAKE_SOURCE_DIR}/shaders/distribution.frag
             ${CMAKE_SOURCE_DIR}/shaders/distribution.vert
             ${CMAKE_SOURCE_DIR}/shaders/accumulator.frag
             ${CMAKE_SOURCE_DIR}/shaders/accumulator.vert
             ${CMAKE_SOURCE_DIR}/shaders/estimated_distribution.frag
             ${CMAKE_SOURCE_DIR}/shaders/estimated_distribution.vert
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/shaders
    COMMAND xxd -i -n SimulationFrag ${CMAKE_SOURCE_DIR}/shaders/simulation.frag ${CMAKE_BINARY_DIR}/shaders/simulation.frag.h
    COMMAND xxd -i -n SimulationVert ${CMAKE_SOURCE_DIR}/shaders/simulation.vert ${CMAKE_BINARY_DIR}/shaders/simulation.vert.h
    COMMAND xxd -i -n ParticleFrag ${CMAKE_SOURCE_DIR}/shaders/particle.frag ${CMAKE_BINARY_DIR}/shaders/particle.frag.h
    COMMAND xxd -i -n ParticleVert ${CMAKE_SOURCE_DIR}/shaders/particle.vert ${CMAKE_BINARY_DIR}/shaders/particle.vert.h
    COMMAND xxd -i -n DistributionFrag ${CMAKE_SOURCE_DIR}/shaders/distribution.frag ${CMAKE_BINARY_DIR}/shaders/distribution.frag.h
    COMMAND xxd -i -n DistributionVert ${CMAKE_SOURCE_DIR}/shaders/distribution.vert ${CMAKE_BINARY_DIR}/shaders/distribution.vert.h
    COMMAND xxd -i -n AccumulatorFrag ${CMAKE_SOURCE_DIR}/shaders/accumulator.frag ${CMAKE_BINARY_DIR}/shaders/accumulator.frag.h
    COMMAND xxd -i -n AccumulatorVert ${CMAKE_SOURCE_DIR}/shaders/accumulator.vert ${CMAKE_BINARY_DIR}/shaders/accumulator.vert.h
    COMMAND xxd -i -n EstimatedDistributionFrag ${CMAKE_SOURCE_DIR}/shaders/estimated_distribution.frag ${CMAKE_BINARY_DIR}/shaders/estimated_distribution.frag.h
    COMMAND xxd -i -n EstimatedDistributionVert ${CMAKE_SOURCE_DIR}/shaders/estimated_distribution.vert ${CMAKE_BINARY_DIR}/shaders/estimated_distribution.vert.h
)

add_executable(Langevin
    main.cxx
    simulation.h
    simulation.cxx
    particle_renderer.h
    particle_renderer.cxx
    distribution_renderer.h
    distribution_renderer.cxx
    estimated_distribution_renderer.h
    estimated_distribution_renderer.cxx
    ${CMAKE_BINARY_DIR}/shaders/simulation.frag.h
    ${CMAKE_BINARY_DIR}/shaders/simulation.vert.h
    ${CMAKE_BINARY_DIR}/shaders/particle.frag.h
    ${CMAKE_BINARY_DIR}/shaders/particle.vert.h
    ${CMAKE_BINARY_DIR}/shaders/distribution.frag.h
    ${CMAKE_BINARY_DIR}/shaders/distribution.vert.h
    ${CMAKE_BINARY_DIR}/shaders/accumulator.frag.h
    ${CMAKE_BINARY_DIR}/shaders/accumulator.vert.h
    ${CMAKE_BINARY_DIR}/shaders/estimated_distribution.frag.h
    ${CMAKE_BINARY_DIR}/shaders/estimated_distribution.vert.h
)

if (EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(EM_COMPILE_FLAGS
      -sUSE_SDL=2
      -fwasm-exceptions
  )
  set(EM_LINK_FLAGS
      -sMIN_WEBGL_VERSION=2
      -sMAX_WEBGL_VERSION=2
      -sALLOW_MEMORY_GROWTH=1
      -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS=1
      -sEXCEPTION_DEBUG=1
      -sASSERTIONS=2
      -sUSE_SDL=2
      --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html
  )
  target_link_libraries(Langevin
      glm::glm
      imgui
  )
  target_link_options(Langevin PRIVATE ${EM_LINK_FLAGS})
  target_compile_options(Langevin PRIVATE ${EM_COMPILE_FLAGS})
  target_link_options(imgui PRIVATE ${EM_LINK_FLAGS})
  target_compile_options(imgui PRIVATE ${EM_COMPILE_FLAGS})

  # Install generated html/js/wasm
  install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/Langevin.html
      ${CMAKE_CURRENT_BINARY_DIR}/Langevin.js
      ${CMAKE_CURRENT_BINARY_DIR}/Langevin.wasm
      DESTINATION .
  )
else()
  target_link_libraries(Langevin
      OpenGL::OpenGL
      GLEW::GLEW
      SDL2::SDL2
      glm::glm
      imgui
  )
endif()

target_include_directories(Langevin PRIVATE
    ${CMAKE_BINARY_DIR}/shaders
)
