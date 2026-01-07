# Langevin Visualization

This project visualizes 2D Langevin dynamics driven by a mixture
of Gaussians. It simulates a dense grid of particle positions on the GPU
and renders:
- An estimated distribution (from simulated particles)
- The analytical ground-truth distribution (mixture of Gaussians)

Built with SDL2 + OpenGL 3 on desktop and WebGL2 (via Emscripten) on the web.
ImGui is used for interactive controls.

## Live Demo

Try it in your browser:

https://theartful.github.io/LangevinVisualization/
