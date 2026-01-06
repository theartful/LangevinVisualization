#include "distribution_renderer.h"
#include "estimated_distribution_renderer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "mixture.h"
#include "particle_renderer.h"
#include "simulation.h"

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <cmath>
#include <stdio.h>

// Main code
int main(int, char **) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    printf("Error: %s\n", SDL_GetError());
    return 1;
  }

  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 330";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window = SDL_CreateWindow(
      "Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, (int)(1280 * main_scale), (int)(800 * main_scale),
      window_flags);
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
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup scaling
  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(
      main_scale); // Bake a fixed style scale. (until we have a solution for
                   // dynamic style scaling, changing this requires resetting
                   // Style + calling this again)
  style.FontScaleDpi =
      main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true
                  // makes this unnecessary. We leave both here for
                  // documentation purpose)

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    return 1;
  }

  Simulation simulation{};
  ParticleRenderer particleRenderer{};
  DistributionRenderer distributionRenderer{};
  EstimatedDistributionRenderer estimatedDistributionRenderer{};

  // Initialize default mixture of Gaussians (4 components)
  MixtureOfGaussians mog{};
  mog.count = 4;
  mog.g[0] = Gaussian{glm::vec2(-0.5f, -0.5f), glm::vec2(0.1f, 0.1f)};
  mog.g[1] = Gaussian{glm::vec2(0.5f, 0.5f), glm::vec2(0.1f, 0.1f)};
  mog.g[2] = Gaussian{glm::vec2(-0.5f, 0.5f), glm::vec2(0.1f, 0.1f)};
  mog.g[3] = Gaussian{glm::vec2(0.5f, -0.5f), glm::vec2(0.1f, 0.1f)};

  // Compute and store mixture peak on CPU
  mog.UpdatePeak();

  simulation.SetMixture(mog);
  distributionRenderer.SetMixture(mog);
  estimatedDistributionRenderer.SetMixture(mog);

  // Our state
  ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  float dt = 0.00004f;
  glm::vec2 viewCenter(0.0f, 0.0f);
  float viewScale = 1.0f; // half-extent before aspect correction

  // Main loop
  bool done = false;
  while (!done) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application, or clear/overwrite your copy of the
    // keyboard data. Generally you may always pass all inputs to dear imgui,
    // and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Controls panel: Mixture and Simulation
    bool mixture_changed = false;
    if (ImGui::Begin("Controls")) {
      ImGui::SeparatorText("Mixture");
      // Count
      int prev_count = mog.count;
      if (ImGui::SliderInt("Count", &mog.count, 1, 10)) {
        if (mog.count < 1)
          mog.count = 1;
        if (mog.count > 10)
          mog.count = 10;
        mixture_changed = true;
      }

      // Per-Gaussian controls
      for (int i = 0; i < mog.count; ++i) {
        ImGui::PushID(i);
        ImGui::Text("Gaussian %d", i);
        ImGui::Separator();
        // Mean in [-1, 1]
        if (ImGui::DragFloat2("Mean", &mog.g[i].mean.x, 0.01f, -1.0f, 1.0f,
                              "%.3f")) {
          mixture_changed = true;
        }
        // Sigma > 0, reasonable range
        if (ImGui::DragFloat2("Sigma", &mog.g[i].sigma.x, 0.001f, 0.005f, 1.0f,
                              "%.4f")) {
          mixture_changed = true;
        }
        // Clamp sigma to positive minimum to avoid degenerate PDFs
        mog.g[i].sigma.x =
            mog.g[i].sigma.x < 0.001f ? 0.001f : mog.g[i].sigma.x;
        mog.g[i].sigma.y =
            mog.g[i].sigma.y < 0.001f ? 0.001f : mog.g[i].sigma.y;
        ImGui::PopID();
      }

      ImGui::SeparatorText("Simulation");
      ImGui::SliderFloat("dt", &dt, 0.000001f, 0.01f, "%.6f",
                         ImGuiSliderFlags_Logarithmic);

      ImGui::SeparatorText("View");
      ImGui::DragFloat2("Center", &viewCenter.x, 0.01f, -10.0f, 10.0f, "%.3f");
      ImGui::SliderFloat("Scale", &viewScale, 0.01f, 10.0f, "%.3f",
                         ImGuiSliderFlags_Logarithmic);
    }
    ImGui::End();

    if (mixture_changed) {
      mog.UpdatePeak();
      simulation.SetMixture(mog);
      distributionRenderer.SetMixture(mog);
      estimatedDistributionRenderer.SetMixture(mog);
    }

    // Mouse drag to pan view center (when not interacting with UI)
    {
      const Viewport basePV = {viewCenter - glm::vec2(viewScale),
                               viewCenter + glm::vec2(viewScale)};
      const Viewport halfPixels = {{0, 0},
                                   {io.DisplaySize.x / 2.0f, io.DisplaySize.y}};
      const Viewport pv = EnforceAspectRatio(basePV, halfPixels);
      const float sx = pv.Width() / halfPixels.Width();
      const float sy = pv.Height() / halfPixels.Height();

      if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 d = io.MouseDelta;
        if ((d.x != 0.0f) || (d.y != 0.0f)) {
          viewCenter.x -= d.x * sx;
          viewCenter.y += d.y * sy; // screen Y down -> world up
        }
      }
    }

    // Ctrl + Mouse Wheel to zoom (adjust viewScale)
    if (!io.WantCaptureMouse && io.KeyCtrl && io.MouseWheel != 0.0f) {
      const float zoomStep = 1.1f; // multiplicative step per wheel notch
      if (io.MouseWheel > 0.0f) {
        viewScale /= zoomStep; // zoom in
      } else {
        viewScale *= zoomStep; // zoom out
      }
      if (viewScale < 0.01f)
        viewScale = 0.01f;
      if (viewScale > 10.0f)
        viewScale = 10.0f;
    }

    simulation.SetDt(dt);
    simulation.Update();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Rendering
    {
      const Viewport particleViewport = {viewCenter - glm::vec2(viewScale),
                                         viewCenter + glm::vec2(viewScale)};
      const Viewport pixelViewport = {{0, 0},
                                      {io.DisplaySize.x / 2, io.DisplaySize.y}};
      const Viewport particleViewportCorretAspect =
          EnforceAspectRatio(particleViewport, pixelViewport);

      estimatedDistributionRenderer.Render(
          particleViewportCorretAspect, pixelViewport, simulation.Width(),
          simulation.Height(), simulation.ParticlesTexture());

      // particleRenderer.Render(particleViewportCorretAspect, pixelViewport,
      //                         simulation.Width(), simulation.Height(),
      //                         simulation.ParticlesTexture());
    }

    {
      const Viewport particleViewport = {viewCenter - glm::vec2(viewScale),
                                         viewCenter + glm::vec2(viewScale)};
      const Viewport pixelViewport = {{io.DisplaySize.x / 2, 0},
                                      {io.DisplaySize.x, io.DisplaySize.y}};
      const Viewport particleViewportCorretAspect =
          EnforceAspectRatio(particleViewport, pixelViewport);

      distributionRenderer.Render(particleViewportCorretAspect, pixelViewport);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
