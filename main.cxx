#include "distribution_renderer.h"
#include "estimated_distribution_renderer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "mixture.h"
#include "particle_renderer.h"
#include "simulation.h"

#ifndef EMSCRIPTEN
#include <GL/glew.h>
#else
extern "C" {
void emscripten_set_main_loop_arg(void (*)(void *), void *, int, int);
}
#endif
#include <SDL.h>
#ifndef EMSCRIPTEN
#include <SDL_opengl.h>
#endif
#include <cmath>
#include <stdio.h>

struct AppState {
  SDL_Window *window = nullptr;
  SDL_GLContext gl_context = nullptr;
  Simulation simulation;
  ParticleRenderer particleRenderer;
  DistributionRenderer distributionRenderer;
  EstimatedDistributionRenderer estimatedDistributionRenderer;
  MixtureOfGaussians mog;
  float dt;
  glm::vec2 viewCenter;
  float viewScale;
  bool running;
};

static void InitDefaultState(AppState &s) {
  s.mog.count = 4;
  s.mog.g[0] = Gaussian{glm::vec2(-0.5f, -0.5f), glm::vec2(0.1f, 0.1f)};
  s.mog.g[1] = Gaussian{glm::vec2(0.5f, 0.5f), glm::vec2(0.1f, 0.1f)};
  s.mog.g[2] = Gaussian{glm::vec2(-0.5f, 0.5f), glm::vec2(0.1f, 0.1f)};
  s.mog.g[3] = Gaussian{glm::vec2(0.5f, -0.5f), glm::vec2(0.1f, 0.1f)};
  s.mog.UpdatePeak();
  s.simulation.SetMixture(s.mog);
  s.distributionRenderer.SetMixture(s.mog);
  s.estimatedDistributionRenderer.SetMixture(s.mog);
  s.dt = 0.00004f;
  s.viewCenter = glm::vec2(0.0f, 0.0f);
  s.viewScale = 1.0f;
  s.running = true;
}

static void Frame(void *arg) {
  AppState *s = static_cast<AppState *>(arg);
  ImGuiIO &io = ImGui::GetIO();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      s->running = false;
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(s->window))
      s->running = false;
  }
  if (SDL_GetWindowFlags(s->window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  bool mixture_changed = false;
  if (ImGui::Begin("Controls")) {
    ImGui::SeparatorText("Mixture");
    if (ImGui::SliderInt("Count", &s->mog.count, 1, 10)) {
      if (s->mog.count < 1)
        s->mog.count = 1;
      if (s->mog.count > 10)
        s->mog.count = 10;
      mixture_changed = true;
    }
    for (int i = 0; i < s->mog.count; ++i) {
      ImGui::PushID(i);
      ImGui::Text("Gaussian %d", i);
      ImGui::Separator();
      if (ImGui::DragFloat2("Mean", &s->mog.g[i].mean.x, 0.01f, -1.0f, 1.0f,
                            "%.3f")) {
        mixture_changed = true;
      }
      if (ImGui::DragFloat2("Sigma", &s->mog.g[i].sigma.x, 0.001f, 0.005f, 1.0f,
                            "%.4f")) {
        mixture_changed = true;
      }
      s->mog.g[i].sigma.x =
          s->mog.g[i].sigma.x < 0.001f ? 0.001f : s->mog.g[i].sigma.x;
      s->mog.g[i].sigma.y =
          s->mog.g[i].sigma.y < 0.001f ? 0.001f : s->mog.g[i].sigma.y;
      ImGui::PopID();
    }

    ImGui::SeparatorText("Simulation");
    ImGui::SliderFloat("dt", &s->dt, 0.000001f, 0.01f, "%.6f",
                       ImGuiSliderFlags_Logarithmic);

    ImGui::SeparatorText("View");
    ImGui::DragFloat2("Center", &s->viewCenter.x, 0.01f, -10.0f, 10.0f, "%.3f");
    ImGui::SliderFloat("Scale", &s->viewScale, 0.01f, 10.0f, "%.3f",
                       ImGuiSliderFlags_Logarithmic);
  }
  ImGui::End();

  if (mixture_changed) {
    s->mog.UpdatePeak();
    s->simulation.SetMixture(s->mog);
    s->distributionRenderer.SetMixture(s->mog);
    s->estimatedDistributionRenderer.SetMixture(s->mog);
  }

  {
    const Viewport basePV = {s->viewCenter - glm::vec2(s->viewScale),
                             s->viewCenter + glm::vec2(s->viewScale)};
    const Viewport halfPixels = {{0, 0},
                                 {io.DisplaySize.x / 2.0f, io.DisplaySize.y}};
    const Viewport pv = EnforceAspectRatio(basePV, halfPixels);
    const float sx = pv.Width() / halfPixels.Width();
    const float sy = pv.Height() / halfPixels.Height();

    if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      ImVec2 d = io.MouseDelta;
      if ((d.x != 0.0f) || (d.y != 0.0f)) {
        s->viewCenter.x -= d.x * sx;
        s->viewCenter.y += d.y * sy;
      }
    }
  }

  if (!io.WantCaptureMouse && io.KeyCtrl && io.MouseWheel != 0.0f) {
    const float zoomStep = 1.1f;
    if (io.MouseWheel > 0.0f) {
      s->viewScale /= zoomStep;
    } else {
      s->viewScale *= zoomStep;
    }
    if (s->viewScale < 0.01f)
      s->viewScale = 0.01f;
    if (s->viewScale > 10.0f)
      s->viewScale = 10.0f;
  }

  s->simulation.SetDt(s->dt);
  s->simulation.Update();

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  {
    const Viewport particleViewport = {s->viewCenter - glm::vec2(s->viewScale),
                                       s->viewCenter + glm::vec2(s->viewScale)};
    const Viewport pixelViewport = {{0, 0},
                                    {io.DisplaySize.x / 2, io.DisplaySize.y}};
    const Viewport particleViewportCorretAspect =
        EnforceAspectRatio(particleViewport, pixelViewport);

    s->estimatedDistributionRenderer.Render(
        particleViewportCorretAspect, pixelViewport, s->simulation.Width(),
        s->simulation.Height(), s->simulation.ParticlesTexture());
  }

  {
    const Viewport particleViewport = {s->viewCenter - glm::vec2(s->viewScale),
                                       s->viewCenter + glm::vec2(s->viewScale)};
    const Viewport pixelViewport = {{io.DisplaySize.x / 2, 0},
                                    {io.DisplaySize.x, io.DisplaySize.y}};
    const Viewport particleViewportCorretAspect =
        EnforceAspectRatio(particleViewport, pixelViewport);

    s->distributionRenderer.Render(particleViewportCorretAspect, pixelViewport);
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(s->window);
}
// Main code
int main(int, char **) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    printf("Error: %s\n", SDL_GetError());
    return 1;
  }

#ifdef EMSCRIPTEN
  const char *glsl_version = "#version 300 es";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  const char *glsl_version = "#version 330";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window = SDL_CreateWindow(
      "Langevin", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
  if (window == nullptr) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return 1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == nullptr) {
    printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
    return 1;
  }

  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

#ifndef EMSCRIPTEN
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    return 1;
  }
#endif

#ifdef EMSCRIPTEN
  AppState *state_ptr = new AppState();
  AppState &state = *state_ptr;
#else
  AppState state;
#endif

  state.window = window;
  state.gl_context = gl_context;
  InitDefaultState(state);

#if EMSCRIPTEN
  emscripten_set_main_loop_arg(Frame, state_ptr, 0, true);
  return 0;
#else

  while (state.running) {
    Frame(&state);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
#endif
}
