/// \file Sushi test file.

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <lodepng.h>

#include <cstdlib>
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
#include <sstream>

using namespace std;

using glm::vec2;
using glm::vec3;

/// Sushi
namespace sushi {

/// Sushi constants
namespace constants {
    struct attrib_location {
        static constexpr auto POSITION = 0;
        static constexpr auto TEXCOORD = 1;
        static constexpr auto NORMAL = 2;
    };

    struct attrib_name {
        static constexpr auto POSITION = "VertexPosition";
        static constexpr auto TEXCOORD = "VertexTexCoord";
        static constexpr auto NORMAL = "VertexNormal";
    };

    struct common_uniform {
        static constexpr auto PROJ_MAT = "ProjectionMat";
        static constexpr auto VIEW_MAT = "ViewMat";
        static constexpr auto MODEL_MAT = "ModelMat";
        static constexpr auto MVP_MAT = "MVP";
    };

    /// Shader types.
    enum class shader_type : GLenum {
        VERTEX = GL_VERTEX_SHADER,
        FRAGMENT = GL_FRAGMENT_SHADER
    };
}

/// Ref-counted initialization token for GLFW.
/// Terminates GLFW when all tokens are dropped.
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

/// Custom deleter for storing `GLFWwindow*` in `std::unique_ptr`.
struct glfw_window_deleter {
    void operator()(GLFWwindow* w) const {
        glfwDestroyWindow(w);
    }
};

/// Unique handle to a GLFW window.
using glfw_window_ptr = unique_ptr<GLFWwindow,glfw_window_deleter>;

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
    explicit input_button(input_type type, int value) : type(type), value(value) {}
};

/// Array of key states. Keeps track of presses and releases.
/// \tparam NKeys Number of keys to track.
template <int NKeys>
class key_array {
    struct keystate {
        int when_last_pressed = 0;
        int when_last_released = 0;
    };
    array<keystate,NKeys> keystates;
public:
    /// Default-initialization.
    /// Initializes as if `press(k,0)` and `release(k,0)` are called for every valid `k`.
    key_array() = default;

    /// Press key `k` at time `t`.
    /// \pre `k>0` and `k<NKeys`.
    /// \pre `t` is greater than the last `t` passed to either `press` or `release`.
    /// \post `was_pressed(k,x)` will return `true` for any `x<t` until a call to `press(k,y)` for any `y`.
    /// \post `is_down(k)` will return `true` until a call to `release(k,y)` for any `y`.
    /// \param k Index of key to press.
    /// \param t Time to press key.
    void press(int k, int t) {
        keystates[k].when_last_pressed = t;
    }

    /// Release key `k` at time `t`.
    /// \pre `k>0` and `k<NKeys`.
    /// \pre `t` is greater than the last `t` passed to either `press` or `release`.
    /// \post `was_released(k,x)` will return `true` for any `x<t` until a call to `release(k,y)` for any `y`.
    /// \post `is_up(k)` will return `true` until a call to `press(k,y)` for any `y`.
    /// \param k Index of key to press.
    /// \param t Time to press key.
    void release(int k, int t) {
        keystates[k].when_last_released = t;
    }

    /// Checks if key `k` was last pressed after time `t`.
    /// \pre `k>0` and `k<NKeys`.
    /// \param k Index of key to check.
    /// \param t Time after which presses are detected.
    /// \return True if `t<x` for `x` in the last call to `press(k,x)`.
    bool was_pressed(int k, int t) {
        auto ks = keystates[k];
        return t < ks.when_last_pressed;
    }

    /// Checks if key `k` was last released after time `t`.
    /// \pre `k>0` and `k<NKeys`.
    /// \param k Index of key to check.
    /// \param t Time after which releases are detected.
    /// \return True if `t<x` for `x` in the last call to `release(k,x)`.
    bool was_released(int k, int t) {
        auto ks = keystates[k];
        return t < ks.when_last_released;
    }

    /// Checks if key `k` is currently pressed.
    /// \pre `k>0` and `k<NKeys`.
    /// \param k Index of key to check.
    /// \return True if `x<y` for `x` and `y` in the last calls to `release(k,x)` and `press(k,y)`.
    bool is_down(int k) {
        auto ks = keystates[k];
        return ks.when_last_released < ks.when_last_pressed;
    }

    /// Checks if key `k` is currently not pressed.
    /// \pre `k>0` and `k<NKeys`.
    /// \param k Index of key to check.
    /// \return Negation of `is_down(k)`.
    bool is_up(int k) {
        return !is_down(k);
    }
};

inline void error_cb(int error, const char* description) {
    throw runtime_error(description);
}

/// Main window class.
/// \invariant There is no more than 1 instance of this class.
/// \invariant This class is only used from the main thread.
class window {
    glfw_init_token t1;
    glfw_window_ptr handle;

    key_array<GLFW_KEY_LAST+1> keyboard_keys;
    key_array<GLFW_MOUSE_BUTTON_LAST+1> mouse_buttons;
    glm::vec2 mouse_pos;
    glm::vec2 scroll_offset;

    /// UTF-32 string that the user has typed during this frame.
    vector<unsigned> char_buffer;

    /// End of last tick cycle; beginning of current tick cycle.
    int last_tick = 0;

    /// End of current tick cycle; beginning of next tick cycle.
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

    static void mouse_button_cb(GLFWwindow* w, int button, int action, int mods) {
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

    static void scroll_cb(GLFWwindow* w, double xoffset, double yoffset) {
        window& self = *static_cast<window*>(glfwGetWindowUserPointer(w));
        self.scroll_offset += glm::vec2{xoffset,yoffset};
    }

public:
    /// Opens a window.
    /// If `fullscreen` is true, the window will fill the primary monitor,
    /// and set the display mode to match `<width>x<height>`.
    /// \pre `<width>x<height>` is a valid resolution for the current platform and display mode.
    /// \param width Width of the window in pixels.
    /// \param height Height of the window in pixels.
    /// \param title Initial title of the window.
    /// \param fullscreen True to request fullscreen.
    window(int width, int height, const string& title, bool fullscreen) {
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

    /// Start the main loop.
    /// Handles event polling and buffer swapping.
    /// Calls `func` once per frame.
    /// Returns when the window is ready to close.
    /// \pre `main_loop` is not already running.
    /// \param func Function to call once per frame.
    void main_loop(function<void()> func) {
        while (!glfwWindowShouldClose(handle.get())) {
            last_tick = current_tick;
            glfwPollEvents();
            glClear(GL_DEPTH_BUFFER_BIT);
            func();
            glfwSwapBuffers(handle.get());
        }
    }

    template <typename Func>
    auto apply_key_array(input_type t, Func&& func) {
        switch (t) {
            case input_type::KEYBOARD:
                return func(keyboard_keys);
            case input_type::MOUSE_BUTTON:
                return func(mouse_buttons);
            default:
                throw; // TODO
        }
    }

    /// Checks if a button was pressed this frame.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button was pressed this frame.
    bool was_pressed(input_button b) {
        auto func = [&](auto& ka){
            return ka.was_pressed(b.value, last_tick);
        };
        return apply_key_array(b.type,func);
    }

    /// Checks if a button was released this frame.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button was released this frame.
    bool was_released(input_button b) {
        auto func = [&](auto& ka){
            return ka.was_released(b.value, last_tick);
        };
        return apply_key_array(b.type,func);
    }

    /// Checks if a button is currently held down.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button is down.
    bool is_down(input_button b) {
        auto func = [&](auto& ka){
            return ka.is_down(b.value);
        };
        return apply_key_array(b.type,func);
    }

    /// Checks if a button is not held down.
    /// \pre `b.type` is not `UNKNOWN`.
    /// \param b Button to check.
    /// \return True if the button is not downs.
    bool is_up(input_button b) {
        auto func = [&](auto& ka){
            return ka.is_up(b.value);
        };
        return apply_key_array(b.type,func);
    }
};

/// Allows a value type to be used as a Nullable.
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

/// A unique handle to an OpenGL resource.
template <typename Deleter>
using unique_gl_resource = std::unique_ptr<typename Deleter::pointer,Deleter>;

/// Deleter for OpenGL buffer objects.
struct buffer_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteBuffers(1, &buf);
    }
};

/// Deleter for OpenGL vertex array objects.
struct vertex_array_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteVertexArrays(1, &buf);
    }
};

/// Deleter for OpenGL texture objects.
struct texture_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteTextures(1, &buf);
    }
};

/// A unique handle to an OpenGL buffer object.
using unique_buffer = unique_gl_resource<buffer_deleter>;

/// A unique handle to an OpenGL vertex array object.
using unique_vertex_array = unique_gl_resource<vertex_array_deleter>;

/// A unique handle to an OpenGL texture object.
using unique_texture = unique_gl_resource<texture_deleter>;

/// Creates a unique OpenGL buffer object.
/// \return A unique buffer object.
inline unique_buffer make_unique_buffer() {
    GLuint buf;
    glGenBuffers(1, &buf);
    return unique_buffer(buf);
}

/// Creates a unique OpenGL vertex array object.
/// \return A unique vertex array object.
inline unique_vertex_array make_unique_vertex_array() {
    GLuint buf;
    glGenVertexArrays(1, &buf);
    return unique_vertex_array(buf);
}

/// Creates a unique OpenGL texture object.
/// \return A unique texture object.
inline unique_texture make_unique_texture() {
    GLuint buf;
    glGenTextures(1, &buf);
    return unique_texture(buf);
}

/// A static OpenGL mesh made of triangles.
struct static_mesh {
    unique_vertex_array vao;
    unique_buffer vertex_buffer;
    int num_triangles = 0;
};

/// Loads an entire file into memory.
/// The file is loaded line-by-line into a vector.
/// \param fname File name.
/// \return All lines in the file, or nothing if the file cannot be opened.
inline vector<string> load_file(const string& fname) {
    ifstream file(fname);
    string line;
    vector<string> rv;
    while (getline(file, line)) {
        line += "\n";
        rv.push_back(line);
    }
    return rv;
}

/// Loads a mesh from an OBJ file.
/// The following OBJ directives are supported:
/// - `#` - Comments
/// - `v` - Vertex location.
/// - `vn` - Vertex normal.
/// - `vt` - Vertex texture coordinate.
/// - `f` - Face (triangles only).
/// \param fname File name.
/// \return The static mesh described by the file.
static_mesh load_static_mesh(const string& fname) {
    ifstream file (fname);
    string line;
    string word;
    int line_number = 0;

    vector<vec3> vertices;
    vector<vec3> normals;
    vector<vec2> texcoords;

    vector<GLfloat> data;
    int num_tris = 0;

    while (getline(file,line)) {
        ++line_number;
        istringstream iss (line);
        iss >> word;

        if (word == "v") {
            vec3 v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (word == "vn") {
            vec3 v;
            iss >> v.x >> v.y >> v.z;
            normals.push_back(v);
        } else if (word == "vt") {
            vec2 v;
            iss >> v.x >> v.y;
            v.y = 1.0-v.y;
            texcoords.push_back(v);
        } else if (word == "f") {
            int index;
            for (int i=0; i<3; ++i) {
                iss >> word;
                replace(begin(word),end(word),'/',' ');
                istringstream iss2 (word);
                iss2 >> index;
                --index;
                data.push_back(vertices[index].x);
                data.push_back(vertices[index].y);
                data.push_back(vertices[index].z);
                iss2 >> index;
                --index;
                data.push_back(texcoords[index].x);
                data.push_back(texcoords[index].y);
                iss2 >> index;
                --index;
                data.push_back(normals[index].x);
                data.push_back(normals[index].y);
                data.push_back(normals[index].z);
            }
            ++num_tris;
        } else if (word[0] == '#') {
            // pass
        } else {
            clog << "sushi::load_static_mesh(): Warning: Unknown OBJ directive at " << fname << "[" << line_number
                 << "]: \"" << word << "\"." << endl;
        }
    }

    static_mesh rv;

    rv.vao = make_unique_vertex_array();
    rv.vertex_buffer = make_unique_buffer();
    rv.num_triangles = num_tris;

    glBindBuffer(GL_ARRAY_BUFFER, rv.vertex_buffer.get());
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

    glBindVertexArray(rv.vao.get());

    auto stride = sizeof(GLfloat) * (3 + 2 + 3);
    glEnableVertexAttribArray(constants::attrib_location::POSITION);
    glEnableVertexAttribArray(constants::attrib_location::TEXCOORD);
    glEnableVertexAttribArray(constants::attrib_location::NORMAL);
    glVertexAttribPointer(constants::attrib_location::POSITION, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const GLvoid *>(0));
    glVertexAttribPointer(constants::attrib_location::TEXCOORD, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(constants::attrib_location::NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * (3 + 2)));

    return rv;
}

/// Deleter for OpenGL shader objects.
struct shader_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        GLuint shader = p;
        glDeleteShader(shader);
    }
};

/// Deleter for OpenGL shader program objects.
struct program_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        GLuint program = p;
        glDeleteProgram(program);
    }
};

/// A unique handle to an OpenGL shader object.
using unique_shader = unique_gl_resource<shader_deleter>;

/// A unique handle to an OpenGL shader program object.
using unique_program = unique_gl_resource<program_deleter>;

/// Creates a unique OpenGL shader object.
/// \return A unique shader object.
inline unique_shader make_unique_shader(GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    if (shader == 0) {
        throw runtime_error("Failed to create shader!");
    }
    return unique_shader(shader);
}

/// Creates a unique OpenGL shader program object.
/// \return A unique shader program object.
inline unique_program make_unique_program() {
    GLuint program = glCreateProgram();
    if (program == 0) {
        throw runtime_error("Failed to create shader program!");
    }
    return unique_program(program);
}

/// Loads and compiles an OpenGL shader from a file.
/// \param type The type of shader to be created.
/// \param fname File name.
/// \return A unique shader object.
inline unique_shader compile_shader_file(constants::shader_type type, const string& fname) {
    auto lines = load_file(fname);

    vector<const GLchar*> line_pointers;
    line_pointers.reserve(lines.size());
    transform(begin(lines),end(lines),back_inserter(line_pointers),[](auto& line){return line.data();});

    unique_shader rv = make_unique_shader(GLenum(type));

    glShaderSource(rv.get(), line_pointers.size(), &line_pointers[0], nullptr);

    glCompileShader(rv.get());

    GLint result;
    glGetShaderiv(rv.get(), GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        ostringstream oss;

        oss << "Shader compilation failed!" << endl;

        GLint log_length;
        glGetShaderiv(rv.get(), GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            auto log = make_unique<GLchar[]>(log_length);
            glGetShaderInfoLog(rv.get(), log_length, nullptr, log.get());

            oss << "Shader compilation log:\n" << log.get() << endl;
        }

        throw runtime_error(oss.str());
    }

    return rv;
}

/// A simple value wrapper for storing a const reference.
template <typename T>
class const_reference_wrapper {
    const T* ptr = nullptr;
public:
    const_reference_wrapper() = default;
    const_reference_wrapper(const T& t) : ptr(&t) {}
    const T& get() const { return *ptr; }
};

/// Links a shader program.
/// \pre All of the shaders are compiled.
/// \param shaders List of shaders to link.
/// \return Unique handle to the new shader program.
inline unique_program link_program(const vector<const_reference_wrapper<unique_shader>>& shaders) {
    unique_program rv = make_unique_program();

    for (const auto& shader : shaders) {
        glAttachShader(rv.get(), shader.get().get());
    }

    glLinkProgram(rv.get());

    GLint result;
    glGetProgramiv(rv.get(), GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        ostringstream oss;

        oss << "Program compilation failed!" << endl;

        GLint log_length;
        glGetProgramiv(rv.get(), GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            auto log = make_unique<GLchar[]>(log_length);
            glGetProgramInfoLog(rv.get(), log_length, nullptr, log.get());

            oss << "Program compilation log:\n" << log.get() << endl;
        }

        throw runtime_error(oss.str());
    }

    return rv;
}

/// A 2D texture.
struct texture_2d {
    unique_texture handle;
    int width = 0;
    int height = 0;
};

/// Loads a 2D texture from a PNG file.
/// \param fname File name.
/// \param smooth Request texture smoothing.
/// \param wrap Request texture wrapping.
/// \return The texture represented by the file, or an empty texture if a failure occurs.
texture_2d load_texture_2d(const string& fname, bool smooth, bool wrap) {
    std::vector<unsigned char> image;
    unsigned width, height;
    auto error = lodepng::decode(image, width, height, fname);

    texture_2d rv;

    if (error != 0) {
        clog << "sushi::load_texture_2d: Warning: Unable to load texture \"" << fname << "\"." << endl;
        return rv;
    }

    rv.handle = make_unique_texture();
    rv.width = width;
    rv.height = height;

    glBindTexture(GL_TEXTURE_2D, rv.handle.get());

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (smooth? GL_LINEAR : GL_NEAREST));
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (smooth? GL_LINEAR : GL_NEAREST));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);

    return rv;
}

/// Sets the current shader program.
/// \pre The program was successfully linked.
/// \param program Shader program to set.
inline void set_program(const unique_program& program) {
    glUseProgram(program.get());
}

/// Sets the texture for a slot.
/// \param slot Slot index. Must be within the range `[0,GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)`.
/// \param tex The texture to bind.
inline void set_texture(int slot, const texture_2d& tex) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex.handle.get());
}

/// Draws a mesh.
/// \param mesh The mesh to draw.
inline void draw_mesh(const static_mesh& mesh) {
    glBindVertexArray(mesh.vao.get());
    glDrawArrays(GL_TRIANGLES, 0, mesh.num_triangles * 3);
}

/// Sets a uniform in the shader program.
/// \param program The shader program.
/// \param name The name of the uniform.
/// \param data The value to set to the uniform.
template <typename T>
void set_uniform(const unique_program& program, const string& name, const T& data);

template <>
inline void set_uniform(const unique_program& program, const string& name, const glm::mat4& mat) {
    glProgramUniformMatrix4fv(program.get(), glGetUniformLocation(program.get(), name.data()), 1, GL_FALSE, glm::value_ptr(mat));
}

template <>
inline void set_uniform(const unique_program& program, const string& name, const GLint& i) {
    glProgramUniform1i(program.get(), glGetUniformLocation(program.get(), name.data()), i);
}

} // namespace sushi

int main() try {
    namespace sc = sushi::constants;

    auto window = sushi::window(800, 600, "Sushi Test", false);

    auto texture = sushi::load_texture_2d("assets/test.png", false, false);
    auto mesh = sushi::load_static_mesh("assets/test.obj");
    auto program = sushi::link_program({
        sushi::compile_shader_file(sc::shader_type::VERTEX, "assets/vert.glsl"),
        sushi::compile_shader_file(sc::shader_type::FRAGMENT, "assets/frag.glsl"),
    });

    auto xrot = 0.f;
    auto yrot = 0.f;

    window.main_loop([&]{
        sushi::set_program(program);

        auto proj_mat = glm::perspective(90.f, 4.f/3.f, 0.01f, 100.f);
        auto view_mat = glm::translate(glm::mat4(1.f), glm::vec3{0.f,0.f,-5.f});
        auto model_mat = glm::mat4(1.f);

        model_mat = glm::rotate(model_mat, xrot, glm::vec3{1,0,0});
        model_mat = glm::rotate(model_mat, yrot, glm::vec3{0,1,0});

        xrot += 0.0001;
        yrot += 0.001;

        auto mvp = proj_mat * view_mat * model_mat;

        sushi::set_uniform(program, "MVP", mvp);
        sushi::set_uniform(program, "DiffuseTexture", 0);

        auto key_A = sushi::input_button(sushi::input_type::KEYBOARD,GLFW_KEY_A);

        if (window.was_pressed(key_A)) {
            clog << "A pressed." << endl;
        }

        if (window.was_released(key_A)) {
            clog << "A released." << endl;
        }

        sushi::set_uniform(program, "GrayScale", int(window.is_down(sushi::input_button(sushi::input_type::KEYBOARD,GLFW_KEY_A))));

        sushi::set_texture(0, texture);
        sushi::draw_mesh(mesh);
    });

    return EXIT_SUCCESS;
} catch (const exception& e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
}
