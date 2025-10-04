# raster (like in raster bars)

**raster** is a small C playground for experimenting with retro-style graphics and audio. It runs the same code on desktop OpenGL and in the browser via WebGL2, keeping the project lightweight and easy to repurpose for demos or game jams.

## What You Get
- Sprite and text rendering with a tiny API
- Minimal app loop for desktop and web builds
- Immediate-style input polling
- Simple audio playback backed by miniaudio
- Helper scripts (`scripts/`) to build, run, and serve demos quickly

## Project Layout
```
include/          public headers
src/raster/impl/  implementation units per subsystem
examples/         hello_world demo
assets/           textures, shaders, fonts
scripts/          build/run/serve helpers
```

## Build & Run
```bash
# Native desktop
./scripts/build-native.sh
./scripts/run-demo.sh

# Web (WebGL2 via Emscripten)
source /path/to/emsdk/emsdk_env.sh
./scripts/build-web.sh
./scripts/serve-web.sh   # emrun or python3 -m http.server
```

The `examples/hello_world` demo shows sprites, text, input, and a touch of audio. Use it as a starting point, then swap in your own assets or gameplay.
