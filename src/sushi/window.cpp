//
// Created by Jeramy on 7/22/2015.
//

#include "window.hpp"

#include <stdexcept>

/// Sushi
namespace sushi {

int glfw_init_token::refs = 0; // extern

namespace {

template<typename Func, typename Keyboard, typename Mouse>
auto apply_key_array(input_type t, Func&& func, const Keyboard& kb, const Mouse& m) {
    switch (t) {
        case input_type::KEYBOARD:
            return func(kb);
        case input_type::MOUSE_BUTTON:
            return func(m);
        default:
            throw; // TODO
    }
}

}

void window::error_cb(int error, const char* description) {
    (void) error;
    throw std::runtime_error(description);
}

void window::char_cb(GLFWwindow* w, unsigned codepoint) {
    window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
    self.char_buffer.push_back(codepoint);
}

void window::cursor_pos_cb(GLFWwindow* w, double xpos, double ypos) {
    window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
    self.mouse_pos = glm::vec2{xpos, ypos};
}

void window::key_cb(GLFWwindow* w, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;

    window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
    if (key == GLFW_KEY_UNKNOWN) {
        return;
    }

    ++self.current_tick;

    switch (action) {
        case GLFW_PRESS:
            self.keyboard_keys.press(key, self.current_tick);
            break;
        case GLFW_RELEASE:
            self.keyboard_keys.release(key, self.current_tick);
            break;
    }
}

void window::mouse_button_cb(GLFWwindow* w, int button, int action, int mods) {
    (void) mods;

    window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));

    ++self.current_tick;

    switch (action) {
        case GLFW_PRESS:
            self.mouse_buttons.press(button, self.current_tick);
            break;
        case GLFW_RELEASE:
            self.mouse_buttons.release(button, self.current_tick);
            break;
    }
}

void window::scroll_cb(GLFWwindow* w, double xoffset, double yoffset) {
    window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
    self.scroll_offset += glm::vec2{xoffset, yoffset};
}

window::window(int width, int height, const std::string& title, bool fullscreen) {
    glfwSetErrorCallback(error_cb);

    GLFWmonitor* monitor = nullptr;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    handle.reset(glfwCreateWindow(width, height, title.data(), monitor, nullptr));
    if (!handle) {
        throw std::runtime_error("Failed to open window!");
    }

    glfwSetWindowUserPointer(handle.get(), this);
    glfwSetCharCallback(handle.get(), char_cb);
    glfwSetCursorPosCallback(handle.get(), cursor_pos_cb);
    glfwSetKeyCallback(handle.get(), key_cb);
    glfwSetMouseButtonCallback(handle.get(), mouse_button_cb);
    glfwSetScrollCallback(handle.get(), scroll_cb);

    glfwMakeContextCurrent(handle.get());

    if (!gladLoadGL()) {
        throw std::runtime_error("Failed to load GL!");
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.f);
    glClearColor(1, 0, 1, 1);
}

void window::main_loop(std::function<void()> func) {
    while (!glfwWindowShouldClose(handle.get())) {
        last_tick = current_tick;
        glfwPollEvents();
        glClear(GL_DEPTH_BUFFER_BIT);
        func();
        glfwSwapBuffers(handle.get());
    }
}

void window::stop_loop() {
    glfwSetWindowShouldClose(handle.get(), true);
}

bool window::was_pressed(input_button b) const {
    auto func = [&](const auto& ka) {
        return ka.was_pressed(b.value, last_tick);
    };
    return apply_key_array(b.type, func, keyboard_keys, mouse_buttons);
}

bool window::was_released(input_button b) const {
    auto func = [&](const auto& ka) {
        return ka.was_released(b.value, last_tick);
    };
    return apply_key_array(b.type, func, keyboard_keys, mouse_buttons);
}

bool window::is_down(input_button b) const {
    auto func = [&](const auto& ka) {
        return ka.is_down(b.value);
    };
    return apply_key_array(b.type, func, keyboard_keys, mouse_buttons);
}

bool window::is_up(input_button b) const {
    auto func = [&](const auto& ka) {
        return ka.is_up(b.value);
    };
    return apply_key_array(b.type, func, keyboard_keys, mouse_buttons);
}

}
