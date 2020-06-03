/// \file Sushi test file.


#include <glad/glad.h>
#include <sushi/sushi.hpp>
#include <sushi/obj_loader.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <chrono>

using namespace std;

using glm::vec2;
using glm::vec3;
using glm::mat4;

class example_shader : public sushi::shader_base {
public:
    example_shader() :
        shader_base({
            {sushi::shader_type::VERTEX, "assets/vert.glsl"},
            {sushi::shader_type::FRAGMENT, "assets/frag.glsl"}
        })
    {
        bind();
        uniforms.MVP = get_uniform_location("MVP");
        uniforms.DiffuseTexture = get_uniform_location("DiffuseTexture");
        uniforms.GrayScale = get_uniform_location("GrayScale");
        uniforms.Animated = get_uniform_location("Animated");
        uniforms.Bones = get_uniform_location("Bones");
    }

    void set_MVP(const mat4& mat) {
        sushi::set_current_program_uniform(uniforms.MVP, mat);
    }

    void set_DiffuseTexture(int i) {
        sushi::set_current_program_uniform(uniforms.DiffuseTexture, i);
    }

    void set_GrayScale(int i) {
        sushi::set_current_program_uniform(uniforms.GrayScale, i);
    }

    void set_Animated(bool b) {
        sushi::set_current_program_uniform(uniforms.Animated, +b);
    }

    void set_Bones(const glm::mat4* mats, int num_mats) {
        glUniformMatrix4fv(uniforms.Bones, num_mats, GL_FALSE, &mats[0][0].x);
    }

private:
    struct {
        GLint MVP;
        GLint DiffuseTexture;
        GLint GrayScale;
        GLint Animated;
        GLint Bones;
    } uniforms;
};

struct window_data {
    bool a_pressed = false;
    bool a_released = false;
    bool a_down = false;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (auto user_pointer = glfwGetWindowUserPointer(window)) {
        auto data = static_cast<window_data*>(user_pointer);
        if (key == GLFW_KEY_A && action == GLFW_PRESS) {
            data->a_pressed = true;
            data->a_down = true;
        }
        if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
            data->a_released= true;
            data->a_down = false;
        }
    }
}

int main() try {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to init GLFW");
    }

    auto window = glfwCreateWindow(800, 600, "Sushi Test", nullptr, nullptr);

    if (!window) {
        throw std::runtime_error("Failed to open window");
    }

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to load OpenGL extensions.");
    }

    glEnable(GL_DEPTH_TEST);

    auto texture = sushi::load_texture_2d("assets/test.png", false, false, false, false);
    auto mesh = sushi::load_obj_file("assets/test.obj");
    auto program = example_shader();
    auto xrot = 0.f;
    auto yrot = 0.f;

    auto player_iqm = sushi::iqm::load_iqm("assets/player.iqm");
    auto player_meshes = sushi::load_meshes(player_iqm);
    auto player_skele = sushi::load_skeleton(player_iqm);
    auto player_anim = sushi::get_animation_index(player_skele, "Walk");
    auto player_anim_time = 0.f;
    auto player_tex = sushi::load_texture_2d("assets/player.png", true, false, true, true);

    auto data = window_data{};

    glfwSetWindowUserPointer(window, &data);

    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();

    while (!glfwWindowShouldClose(window)) {
        auto time = clock::now();
        auto delta = std::chrono::duration<double>(time - last_time).count();
        last_time = time;

        data.a_pressed = false;
        data.a_released = false;

        glfwPollEvents();

        if (data.a_pressed) {
            clog << "A pressed." << endl;
        }

        if (data.a_released) {
            clog << "A released." << endl;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto proj_mat = glm::perspective(90.f, 4.f / 3.f, 0.01f, 100.f);
        auto view_mat = glm::translate(glm::mat4(1.f), glm::vec3{0.f, 0.f, -2.f});

        // draw static mesh
        {
            auto model_mat = glm::mat4(1.f);

            xrot += 0.0001;
            yrot += 0.001;

            model_mat = glm::translate(model_mat, glm::vec3{-1, 0, 0});
            model_mat = glm::rotate(model_mat, xrot, glm::vec3{1, 0, 0});
            model_mat = glm::rotate(model_mat, yrot, glm::vec3{0, 1, 0});
            model_mat = glm::scale(model_mat, glm::vec3{0.5, 0.5, 0.5});

            auto mvp = proj_mat * view_mat * model_mat;

            program.bind();
            program.set_MVP(mvp);
            program.set_DiffuseTexture(0);
            program.set_GrayScale(data.a_down);
            sushi::set_texture(0, texture);
            sushi::draw_mesh(mesh);
        }

        // draw animated mesh
        {
            auto model_mat = glm::mat4(1.f);

            model_mat = glm::translate(model_mat, glm::vec3{1, 0, 0});
            model_mat = glm::rotate(model_mat, glm::radians(-90.f), glm::vec3{1, 0, 0});

            auto mvp = proj_mat * view_mat * model_mat;

            program.bind();
            program.set_MVP(mvp);
            program.set_DiffuseTexture(0);
            program.set_GrayScale(data.a_down);
            sushi::set_texture(0, player_tex);
            sushi::draw_mesh(player_meshes, &player_skele, player_anim, player_anim_time);
            player_anim_time += delta;
        }

        glfwSwapBuffers(window);
    }

    return EXIT_SUCCESS;
} catch (const exception& e) {
    cerr << "ERROR: " << e.what() << endl;
    glfwTerminate();
    return EXIT_FAILURE;
}
