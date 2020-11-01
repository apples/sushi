# Sushi

Sushi is a lightweight OpenGL-focused rendering framework.
It provides many fundamental types and functions for 2D and 3D rendering,
without taking control of your engine architecture.

## Features

- Simple shader system that encourages minimal abstraction.
- Texture loading and manipulation, including cube maps.
- Easy framebuffer management, also including cube maps.
- 3D model rendering including skeletal animation.
- Easy-to-use mesh builder for custom mesh generation.
- Basic `transform` vocabulary type.
- Frustum calculation.
- Emscripten support.
- Doesn't get in your way of manually calling OpenGL functions when needed.

## Supported Asset Formats

Images:

- PNG

Models:

- OBJ
- IQM

## Dependencies

- [GLM](https://glm.g-truc.net/)
- [LodePNG](https://lodev.org/lodepng/)
- [GLFW](https://www.glfw.org/) (only needed for tests/examples)

Note: If no `lodepng` CMake target exists, it will be fetched from [https://github.com/apples/lodepng.git](https://github.com/apples/lodepng.git).

## Quick Tutorial

This tutorial will assume you have a window already opened and an OpenGL context initialized.

### Build setup

The easiest way to integrate Sushi is to use `add_subdirectory` in your `CMakeFile`, and then link to the `sushi` library.

Here is an example assuming the use of GLFW:

```cmake
cmake_minimum_required(VERSION 3.2)
project(MY_PROJECT)

find_package(glfw3 REQUIRED)

add_subdirectory(deps/sushi)

add_executable(my_project main.cpp)
target_link_libraries(my_project sushi glfw)
```

In this example, it is also assumed that GLM is found via `find_package`.

### Creating a shader

Sushi's shader system requires you to wrap your shader in a class derived from `sushi::shader_base`.

How exactly you do this is largely up to you, but the recommended way is this:

```cpp
class my_shader : public sushi::shader_base {
public:
    my_shader() :
        shader_base({
            {sushi::shader_type::VERTEX, "assets/vert.glsl"},
            {sushi::shader_type::FRAGMENT, "assets/frag.glsl"}
        })
    {
        bind();
        uniforms.MVP = get_uniform_location("MVP");
        uniforms.DiffuseTexture = get_uniform_location("DiffuseTexture");
    }

    void set_MVP(const mat4& mat) {
        sushi::set_current_program_uniform(uniforms.MVP, mat);
    }

    void set_DiffuseTexture(int i) {
        sushi::set_current_program_uniform(uniforms.DiffuseTexture, i);
    }

private:
    struct {
        GLint MVP;
        GLint DiffuseTexture;
    } uniforms;
};
```

To use `my_shader`, simply construct an instance of it, and call `.bind()`.

Note that `my_shader` uses `sushi::set_current_program_unifrom`,
which requires the shader to be actively bound before you can set its uniforms.

```cpp
// At load time
auto shader = my_shader();

// At render time
shader.bind();
shader.set_MVP(/* your MVP matrix */);
shader.set_DiffuseTexture(0); // Note that this is the texture slot index.
render(); // Implementation left as an exercise for the reader.
```

### Loading textures and models

Textures are loaded easily, but must be PNG format.

```cpp
auto my_texture = sushi::load_texture_2d("assets/player.png", true, false, true, true);
```

Static OBJ models are also easily loaded. Note that OBJ file loading can fail and return `std::nullopt`.

```cpp
auto my_obj = sushi::load_obj_file("assets/player.obj").value_or(sushi::mesh_group{});
```

Loading animated IQM models is a bit more complicated.

First, the IQM file itself must be loaded, and then the meshes and skeleton can be extracted from it.

```cpp
auto player_iqm = sushi::iqm::load_iqm("assets/player.iqm");
auto player_meshes = sushi::load_meshes(*player_iqm);
auto player_skele = sushi::load_skeleton(*player_iqm);
auto player_anim = sushi::get_animation_index(player_skele, "Walk");
auto player_anim_time = 0.f;
```

It is up to the user to encapsulate animated meshes and skeletons, since their exact usage will vary between engines.

### Generating textures and models

Textures and models can both be generated instead of loaded from a file.

Currently, Sushi does not have helper methods for initializing textures, since this is fairly easy with OpenGL.

You can create an uninitialized texture using `sushi::create_uninitialized_texture_2d`,
and then access the texture ID via the `.model` field.

Meshes can be generated using `sushi::mesh_group_builder`.
Note that a `sushi::mesh_group` can contain multiple meshes.

```cpp
auto mb = sushi::mesh_group_builder();
mb.enable(sushi::attrib_location::POSITION);
mb.enable(sushi::attrib_location::TEXCOORD);

mb.mesh("terrain"); // Name of mesh object, Sushi doesn't use this.

auto v1 = mb.vertex()
    .position({0, 0, 0})
    .texcoord({0, 1})
    .get();

auto v2 = mb.vertex()
    .position({0, 1, 0})
    .texcoord({0, 0})
    .get();

auto v3 = mb.vertex()
    .position({1, 0, 0})
    .texcoord({1, 1})
    .get();

mb.tri(v1, v2, v3);

auto completed_mesh = mb.get();
```

Generating skeletal animations is far more complicated, but `sushi::skeleton` follows fairly standard conventions,
so it should not be terribly difficult to integrate into existing systems.

### Transforms

Sushi provides `sushi::transform`, a nice abstraction over an unskewed affine transform.
This was designed for use within the skeletal animation functions, but it works equally well for general transforms.

```cpp
auto camera = sushi::transform{};

camera.pos = {1, 2, 3};
camera.rot = glm::angleAxis(glm::radians(45.f), glm::vec3{0, 1, 0});

auto view_mat = sushi::to_mat4(camera);
```

### Rendering

To assign a texture to a texture slot, use `sushi::set_texture`.

```cpp
sushi::set_texture(0, my_texture);
```

To render a static mesh, simply call `sushi::draw_mesh`.

```cpp
sushi::draw_mesh(my_obj);
```

Drawing animated meshes is only slightly more complicated:

```cpp
auto pose = sushi::get_pose(player_skele, player_anim, player_anim_time, true);
sushi::draw_mesh(player_meshes, pose);
```

Note: Animation smoothing is a complex calculation, so it's recommended that you only enable it for important objects.

All together, rendering is fairly simple:

```cpp
void draw_model(
    my_shader& shader,
    const glm::mat4& mvp,
    const sushi::texture_2d& texture,
    const sushi::mesh_group& mesh
) {
    shader.bind();
    shader.set_MVP(mvp);
    shader.set_DiffuseTexture(0); 

    sushi::set_texture(0, texture);
    sushi::draw_mesh(mesh);
}
```

## License

MIT license. See `LICENSE.txt`.
