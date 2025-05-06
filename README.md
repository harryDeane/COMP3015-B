# Meteor Shower Simulation

This project is an interactive 3D scene built using OpenGL and GLSL, featuring a dynamic meteor shower over a cityscape. It combines advanced rendering techniques including particle systems, geometry shaders for explosions, mouse picking, and environmental effects. Below you'll find details about the development environment, features, and project structure.

## Development Environment

- **Operating System**: Windows 10/11
- **IDE**: Visual Studio 2022

### Core Libraries:
- OpenGL (via GLFW and GLEW)
- GLM (OpenGL Mathematics)
- stb_image (for texture loading)

## Key Features

### Dynamic Meteor System:
- Procedurally generated falling meteors with realistic physics
- Mouse-click interaction to explode meteors

### Advanced Rendering:
- HDR skybox with environment mapping
- Blinn-Phong shading with multiple light sources
- Normal mapping for meteor surface details

### Special Effects:
- Geometry shader-powered meteor explosions
- Particle system for smoke trails off the building
- Point sprite starfield background

### Interactive Controls:
- First-person camera navigation (WASD + mouse)
- Mouse picking to target meteors

## Project Structure

### Core Components
- **scenebasic_uniform.cpp**:
  - Main scene management and rendering loop
  - Meteor spawning/updating logic
  - Particle system implementation
  - Mouse picking functionality

### Shaders:
- **model.vert/frag**: Blinn-Phong shading with normal mapping
- **explosion.vert/geom/frag**: Meteor explosion effects
- **particles.vert/frag**: Smoke particle rendering
- **sprite.vert/geom/frag**: Starfield point sprites
- **skybox.vert/frag**: HDR environment rendering

### Assets:
- **rocknew.obj**: Meteor 3D model
- **smallcity.obj**: City landscape
- Various textures (asteroids, city, smoke, stars)

### Support Classes:
- **KeyboardController**: Handles camera movement
- **ObjMesh**: Model loading and rendering
- **Texture**: Texture loading utilities

## How to Run

### Prerequisites:
- Visual Studio 2022 with C++ support
- OpenGL-compatible GPU
- GLFW and GLEW libraries

### Setup:

```bash
git clone --recursive https://github.com/harryDeane/COMP3015-B
cd meteor-shower-opengl
Ensure all assets are in the media folder
Open solution in Visual Studio and build
```

## Controls:
- **WASD**: Camera movement
- **Mouse**: Look around
- **Left Click**: Explode meteors
- **ESC**: Exit application

## Technical Highlights

### Meteor System:
- Procedural spawning with random positions, rotation axes, and fall speeds
- Two rendering modes:
  - Standard rendering for intact meteors
  - Geometry shader explosion effect when clicked
- Physics-based falling and respawning at ground level

### Particle System:
- Transform feedback for efficient particle updates
- Texture-based smoke effects
- Lifetime management and respawning

### Rendering Techniques:
- **Multiple Light Sources**: Three configurable spotlights illuminating the scene
- **Normal Mapping**: Enhanced surface detail on meteors
- **Instanced Rendering**: Efficient drawing of stars and particles

## Customization Options

Key parameters you can adjust in **scenebasic_uniform.cpp**:
- **NUM_METEORS**: Controls number of active meteors
- **fallSpeed**: Adjusts meteor descent rate
- **particleLifetime**: Changes smoke duration
- **Light positions and intensities**

## Future Enhancements
- Sound effects for explosions and impacts
- Day/night cycle with dynamic lighting
- More sophisticated collision physics
- Advanced post-processing (bloom, motion blur)
- Different meteor types with varied behaviors

## Demonstration Video

https://youtu.be/iTD8svUHSG4

---

This project demonstrates modern OpenGL techniques in an interactive, game-like environment. The code is structured to be modular, making it easy to extend with additional features or adapt for other 3D rendering projects.

