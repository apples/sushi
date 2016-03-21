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
    self.event_buffer.push_back({event_char{codepoint}, self.mouse_pos});
}

void window::cursor_pos_cb(GLFWwindow* w, double xpos, double ypos) {
    window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
    auto old_pos = self.mouse_pos;
    self.mouse_pos = glm::vec2{xpos, ypos};
    self.event_buffer.push_back({event_mouse_move{old_pos}, self.mouse_pos});
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
            self.event_buffer.push_back({event_keyboard_press{key}, self.mouse_pos});
            break;
        case GLFW_RELEASE:
            self.keyboard_keys.release(key, self.current_tick);
            self.event_buffer.push_back({event_keyboard_release{key}, self.mouse_pos});
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
            self.event_buffer.push_back({event_mouse_press{button}, self.mouse_pos});
            break;
        case GLFW_RELEASE:
            self.mouse_buttons.release(button, self.current_tick);
            self.event_buffer.push_back({event_mouse_release{button}, self.mouse_pos});
            break;
    }
}

void window::scroll_cb(GLFWwindow* w, double xoffset, double yoffset) {
    window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
    self.scroll_offset += glm::vec2{xoffset, yoffset};
}

window::window(int width, int height, const std::string& title, bool fullscreen) {
    glfwSetErrorCallback(error_cb);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    if (width == 0 || height == 0) {
        if (fullscreen) {
            width = mode->width;
            height = mode->height;
        } else {
            std::array<std::array<int,2>,5> vmodes = {{
                {{1920,1080}},
                {{1280,768}},
                {{1024,768}},
                {{800,600}},
                {{640,480}},
            }};
            for (auto& res : vmodes) {
                if (res[0] < mode->width && res[1] < mode->height) {
                    width = res[0];
                    height = res[1];
                    break;
                }
            }
            if (width == 0 || height == 0) {
                width = 800;
                height = 600;
            }
        }
    }

    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    handle.reset(glfwCreateWindow(width, height, title.data(), (fullscreen? monitor : nullptr), nullptr));
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwGetFramebufferSize(handle.get(), &width, &height);
    glViewport(0, 0, width, height);
}

void window::main_loop(std::function<void()> func) {
    while (!glfwWindowShouldClose(handle.get())) {
        last_tick = current_tick;
        event_buffer.clear();
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
