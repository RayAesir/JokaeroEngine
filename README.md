## Overview

Straightforward render engine.
Demonstrate principles of AZDO (Approach Zero Driver Overhead) and DSA (Direct State Access).
Practical implementation of various 3D techniques.
Modern C++20 and latest OpenGL with extensions (Vulcan Subgroups).

## Table of Contents

- MultiThreaded OpenGL (Shared Lists, Task System)
- Custom OpenGL Wrapper
- Bindless Textures with Anisotropy Filtering and LOD Bias
- Collision Primitives and Intersection
- Shader ecosystem: edit-aware cache, hot-reload, generated GLSL header
- Multiple Engine States (loading, main, etc.)
- ImGui interface with Guizmos
- Quaternion camera system: FPS and Free
- Input/Event system
- Load/Save settings from file

## Techniques

Frustum Culling (Objects, Shadows) [Compute shader]

Occlusion Culling (Objects) [Compute shader]
![Frustum Culling](./docs/culling1.jpg 'Frustum Culling')
![Occlusion Culling](./docs/culling2.jpg 'Frustum Occlusion')

Cluster Light Culling (Bitonic Sorting for shadowcasters) [Compute Shader]
![Cluster Light](./docs/cluster-light.jpg 'Cluster Light')

Cascaded PCF shadowmapping: Bounding sphere method

Switchable Forward and Deferred pipelines, PBR (metallic/roughness), IBL, for Opaque, Alpha Clip and Alpha Blend meshes
![PBR buffers](./docs/pbr-buffers.jpg 'PBR buffers')
![PBR final](./docs/pbr-final.jpg 'PBR final')

Order-independent transparency (OIT)
MP4 - cube

Instanced Particles [Compute shader]
MP4 - particles

Outline Selection: Pixel-point selection via depth buffer
MP4 - leaf selection

Postprocess:

- Correct Luminosity Encoding
- Tonemapping
- HDR Bloom (Compute shader)
  MP4 - bloom
- FXAA
  ![FXAA](./docs/fxaa.jpg 'FXAA')

HBAO: Nvidia .ref.
![HBAO](./docs/hbao.jpg 'HBAO')

GTAO: ported from DX12 to OpenGL .ref.
![GTAO](./docs/gtao.jpg 'GTAO')

Simple Crosshair [Geometry shader]

Debug shaders:

- Boxes, Cameras
- Point Lights
- Normals
- Texture Buffers (Layers)

## Controls

Tab - toggle UI
1 - Settings window
2 - Active window
3 - Resources window
4 - ImGUI Demo window
LMB - select object
RMB - object Context Menu
WASD/Space/Control - camera
R - reset camera
G - toggle guizmo
V/F - input events
Esc - close

## Assets (Blender)

OpenGL Z axis is forward-backward
Blender Z axis is top-bottom

Export FBX from Blender:

Static objects:

- Rotate all -90deg by X, Apply transformation, Rotate all +90deg by X
- Transform: select "Apply Scalings" -> FBX All
- Geometry: tick "Triangulate Faces" and "Tangent Space"

![Blender Static](./docs/blender-static.jpg 'Blender Static')

Skinned objects:

- The same as Static objects
- Set scale of Armature and Objects to 0.01 (Blender use meters vs FBX centimeters)
- Armature: tick "Only Deform Bones", untick "Add Leaf Bones"

![Blender Skinned](./docs/blender-skinned.jpg 'Blender Skinned')

Transparent Materials:

- Append/Insert/Prepend "AlphaClip" or "AlphaBlend" at the name of material
  (case doesn't matter)
- Set 'Alpha' value in the BSDF node manually (Value inverted!)

![Blender Material](./docs/blender-material.jpg 'Blender Material')

## Unfinished

- GLX (Linux X11) support
- VRAM dynamic allocation
- LOD system
- Texture Atlases
- Load model from UI
- Load scene from file

## Building (Windows)

Install vcpkg:
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && .\bootstrap-vcpkg.bat -disableMetrics
Configure the VCPKG_ROOT environment variable.

Reccomended to use VSCode with CMake Tools extension

## Dependencies

Assets: Assimp
Window management: GLFW3
Math: GLM
Interface: ImGui and ImGuizmo
Memory allocation: MiMalloc
Input/Events: Sigslot
Logger: spdlog
Settings: simpleini
Textures: stb
Misc: FMT, Console Indicators, Google Fruit DI library

## License

The code in this repository is licensed under the MIT License.
The libraries provided by ports are licensed under the terms of their original authors.
