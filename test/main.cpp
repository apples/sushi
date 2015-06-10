#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>

#include <lodepng.h>

#include <stdexcept>
#include <vector>
#include <tuple>
#include <utility>
#include <functional>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <memory>

using namespace std;

namespace sushi {

class glfw_init_token {
    static int refs;
public:
    glfw_init_token() {
        if (refs == 0) {
            if (!glfwInit()) {
                throw runtime_error("Failed to init GLFW!");
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
int glfw_init_token::refs = 0;

struct glfw_window_deleter {
    void operator()(GLFWwindow* w) const {
        glfwDestroyWindow(w);
    }
};
using glfw_window_ptr = unique_ptr<GLFWwindow,glfw_window_deleter>;

template <int NKeys>
class key_array {
    struct key {
        int when_last_pressed = 0;
        int when_last_released = 0;
    };
    array<key,NKeys> keys;
public:
    void press(int k, int t) {
        keys[k].when_last_pressed = t;
    }

    void release(int k, int t) {
        keys[k].when_last_released = t;
    }

    bool was_pressed(int k) {
        return false; // TODO
    }
};

inline void error_cb(int error, const char* description) {
    throw runtime_error(description);
}

class window {
    glfw_init_token t1;
    glfw_window_ptr handle;

    key_array<GLFW_KEY_LAST+1> keyboard_keys;
    key_array<GLFW_MOUSE_BUTTON_LAST+1> mouse_buttons;
    glm::vec2 mouse_pos;
    glm::vec2 scroll_offset;
    vector<unsigned> char_buffer;

    int current_tick = 0;

    static void char_cb(GLFWwindow* w, unsigned codepoint) {
        window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
        self.char_buffer.push_back(codepoint);
    }

    static void cursor_pos_cb(GLFWwindow* w, double xpos, double ypos) {
        window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
        self.mouse_pos = glm::vec2{xpos,ypos};
    }

    static void key_cb(GLFWwindow* w, int key, int scancode, int action, int mods) {
        window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
        if (key == GLFW_KEY_UNKNOWN) {
            return;
        }

        switch (action) {
            case GLFW_PRESS:
                self.keyboard_keys.press(key, self.current_tick);
                break;
            case GLFW_RELEASE:
                self.keyboard_keys.release(key, self.current_tick);
                break;
        }
    }

    static void mouse_button_cb(GLFWwindow* w, int button, int action, int mods) {
        window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
        switch (action) {
            case GLFW_PRESS:
                self.mouse_buttons.press(button, self.current_tick);
                break;
            case GLFW_RELEASE:
                self.mouse_buttons.release(button, self.current_tick);
                break;
        }
    }

    static void scroll_cb(GLFWwindow* w, double xoffset, double yoffset) {
        window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
        self.scroll_offset += glm::vec2{xoffset,yoffset};
    }

public:
    window(int width, int height, const string& title, bool fullscreen) {
        glfwSetErrorCallback(error_cb);

        GLFWmonitor* monitor = nullptr;
        if (fullscreen) {
            monitor = glfwGetPrimaryMonitor();
        }

        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        handle.reset(glfwCreateWindow(width, height, title.data(), monitor, nullptr));
        if (!handle) {
            throw runtime_error("Failed to open window!");
        }

        glfwSetWindowUserPointer(handle.get(), this);
        glfwSetCharCallback(handle.get(), char_cb);
        glfwSetCursorPosCallback(handle.get(), cursor_pos_cb);
        glfwSetKeyCallback(handle.get(), key_cb);
        glfwSetMouseButtonCallback(handle.get(), mouse_button_cb);
        glfwSetScrollCallback(handle.get(), scroll_cb);

        glfwMakeContextCurrent(handle.get());

        if (!gladLoadGL()) {
            throw runtime_error("Failed to load GL!");
        }

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearDepth(1.f);
        glClearColor(1,0,1,1);
    }

    void main_loop(function<void()> func) {
        while (!glfwWindowShouldClose(handle.get())) {
            glfwPollEvents();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            func();
            glfwSwapBuffers(handle.get());
        }
    }
};

template <typename T>
struct fake_nullable {
    T value;
    fake_nullable() : value() {}
    fake_nullable(std::nullptr_t) : fake_nullable() {}
    fake_nullable(T v) : value(move(v)) {}
    operator T() const { return value; }
    bool operator==(const fake_nullable& other) const { return value==other.value; }
    bool operator!=(const fake_nullable& other) const { return value!=other.value; }
};

template <typename Deleter>
using unique_gl_resource = std::unique_ptr<typename Deleter::pointer,Deleter>;

struct buffer_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteBuffers(1, &buf);
    }
};

struct vertex_array_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteVertexArrays(1, &buf);
    }
};

struct texture_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteTextures(1, &buf);
    }
};

using unique_buffer = unique_gl_resource<buffer_deleter>;
using unique_vertex_array = unique_gl_resource<vertex_array_deleter>;
using unique_texture = unique_gl_resource<texture_deleter>;

inline unique_buffer make_unique_buffer() {
    GLuint buf;
    glGenBuffers(1, &buf);
    return unique_buffer(buf);
}

inline unique_vertex_array make_unique_vertex_array() {
    GLuint buf;
    glGenVertexArrays(1, &buf);
    return unique_vertex_array(buf);
}

inline unique_texture make_unique_texture() {
    GLuint buf;
    glGenTextures(1, &buf);
    return unique_texture(buf);
}

struct static_mesh {
    unique_vertex_array vao;
    unique_buffer vertices;
    int num_triangles = 0;
};

inline vector<string> load_file(const string &fname) {
    ifstream file(fname);
    string line;
    vector<string> rv;
    while (getline(file, line)) {
        line += "\n";
        rv.push_back(line);
    }
    return rv;
}



} // namespace sushi

int main() {
    auto window = sushi::window(800, 600, "Sushi Test", false);

    window.main_loop([&]{

    });
}
