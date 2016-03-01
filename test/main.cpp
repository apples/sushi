/// \file Sushi test file.

#include "sushi/sushi.hpp"

#include <iostream>

using namespace std;

using glm::vec2;
using glm::vec3;

int main() try {
    auto window = sushi::window(800, 600, "Sushi Test", false);

    auto texture = sushi::load_texture_2d("assets/test.png", false, false, false);
    auto mesh = sushi::load_static_mesh_file("assets/test.obj");
    auto program = sushi::link_program({
                                               sushi::compile_shader_file(sushi::shader_type::VERTEX,
                                                                          "assets/vert.glsl"),
                                               sushi::compile_shader_file(sushi::shader_type::FRAGMENT,
                                                                          "assets/frag.glsl"),
                                       });

    auto xrot = 0.f;
    auto yrot = 0.f;

    window.main_loop([&] {
        sushi::set_program(program);

        auto proj_mat = glm::perspective(90.f, 4.f / 3.f, 0.01f, 100.f);
        auto view_mat = glm::translate(glm::mat4(1.f), glm::vec3{0.f, 0.f, -5.f});
        auto model_mat = glm::mat4(1.f);

        model_mat = glm::rotate(model_mat, xrot, glm::vec3{1, 0, 0});
        model_mat = glm::rotate(model_mat, yrot, glm::vec3{0, 1, 0});

        xrot += 0.0001;
        yrot += 0.001;

        auto mvp = proj_mat * view_mat * model_mat;

        sushi::set_uniform(program, "MVP", mvp);
        sushi::set_uniform(program, "DiffuseTexture", 0);

        auto key_A = sushi::input_button(sushi::input_type::KEYBOARD, GLFW_KEY_A);

        if (window.was_pressed(key_A)) {
            clog << "A pressed." << endl;
        }

        if (window.was_released(key_A)) {
            clog << "A released." << endl;
        }

        sushi::set_uniform(program, "GrayScale",
                           int(window.is_down(sushi::input_button(sushi::input_type::KEYBOARD, GLFW_KEY_A))));

        sushi::set_texture(0, texture);
        sushi::draw_mesh(mesh);
    });

    return EXIT_SUCCESS;
} catch (const exception& e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
}
