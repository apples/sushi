//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_WINDOW_HPP
#define SUSHI_WINDOW_HPP

#include "gl.hpp"
#include "key_array.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>

/// Sushi
namespace sushi {

/// Type of input device.
enum class input_type {
    /// Unknown input type (usually equivalent to "none").
            UNKNOWN,
    /// Keyboard button input.
            KEYBOARD,
    /// Mouse button input.
            MOUSE_BUTTON
};

/// Input button code.
struct input_button {
    /// Input device type. If `UNKNOWN`, `value` is undefined.
    input_type type = input_type::UNKNOWN;
    /// Virtual key code or index for button. Undefined if `type` is `UNKNOWN`.
    int value;

    input_button() = default;

    explicit input_button(input_type type, int value) : type(type), value(value) { }
};

/// Ref-counted initialization token for GLFW.
/// Terminates GLFW when all tokens are dropped.
class glfw_init_token {
    static int refs;
public:
    glfw_init_token() {
        if (refs == 0) {
            if (!glfwInit()) {
                throw std::runtime_error("Failed to init GLFW!");
            }
        }
        ++refs;
    }

    ~glfw_init_token() {
        if (--refs == 0) {
            glfwTerminate();
        }
    }
};

/// Custom deleter for storing `GLFWwindow*` in `std::unique_ptr`.
struct glfw_window_deleter {
    void operator()(GLFWwindow* w) const {
        glfwDestroyWindow(w);
    }
};

/// Unique handle to a GLFW window.
using glfw_window_ptr = std::unique_ptr<GLFWwindow, glfw_window_deleter>;

/// Main window class.
/// \invariant There is no more than 1 instance of this class.
/// \invariant This class is only used from the main thread.
class window {
    glfw_init_token t1;
    glfw_window_ptr handle;

    key_array<GLFW_KEY_LAST + 1> keyboard_keys;
    key_array<GLFW_MOUSE_BUTTON_LAST + 1> mouse_buttons;
    glm::vec2 mouse_pos;
    glm::vec2 scroll_offset;

    /// UTF-32 string that the user has typed during this frame.
    std::vector<unsigned> char_buffer;

    /// End of last tick cycle; beginning of current tick cycle.
    int last_tick = 0;

    /// End of current tick cycle; beginning of next tick cycle.
    int current_tick = 0;

    static void error_cb(int error, const char* description);

    static void char_cb(GLFWwindow* w, unsigned codepoint);

    static void cursor_pos_cb(GLFWwindow* w, double xpos, double ypos);

    static void key_cb(GLFWwindow* w, int key, int scancode, int action, int mods);

    static void mouse_button_cb(GLFWwindow* w, int button, int action, int mods);

    static void scroll_cb(GLFWwindow* w, double xoffset, double yoffset);

public:
    /// Opens a window.
    /// If `fullscreen` is true, the window will fill the primary monitor,
    /// and set the display mode to match `<width>x<height>`.
    /// \pre `<width>x<height>` is a valid resolution for the current platform and display mode.
    /// \param width Width of the window in pixels.
    /// \param height Height of the window in pixels.
    /// \param title Initial title of the window.
    /// \param fullscreen True to request fullscreen.
    window(int width, int height, const std::string& title, bool fullscreen);

    /// Start the main loop.
    /// Handles event polling and buffer swapping.
    /// Calls `func` once per frame.
    /// Returns when the window is ready to close.
    /// \pre `main_loop` is not already running.
    /// \param func Function to call once per frame.
    void main_loop(std::function<void()> func);

    /// Checks if a button was pressed this frame.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button was pressed this frame.
    bool was_pressed(input_button b) const;

    /// Checks if a button was released this frame.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button was released this frame.
    bool was_released(input_button b) const;

    /// Checks if a button is currently held down.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button is down.
    bool is_down(input_button b) const;

    /// Checks if a button is not held down.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button is not downs.
    bool is_up(input_button b) const;
};

}

#endif //SUSHI_WINDOW_HPP
