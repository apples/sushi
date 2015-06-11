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
#include <sstream>

using namespace std;

using glm::vec2;
using glm::vec3;

namespace sushi {

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

    struct shader_type {
        static constexpr auto VERTEX = GL_VERTEX_SHADER;
        static constexpr auto FRAGMENT = GL_FRAGMENT_SHADER;
    };
}

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
    unique_buffer vertex_buffer;
    int num_triangles = 0;
};

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
            texcoords.push_back(v);
        } else if (word == "f") {
            int index;
            for (int i=0; i<3; ++i) {
                iss >> word;
                replace(begin(word),end(word),'/',' ');
                istringstream iss2 (word);
                iss2 >> index;
                data.push_back(vertices[index].x);
                data.push_back(vertices[index].y);
                data.push_back(vertices[index].z);
                iss2 >> index;
                data.push_back(texcoords[index].x);
                data.push_back(texcoords[index].y);
                iss2 >> index;
                data.push_back(normals[index].x);
                data.push_back(normals[index].y);
                data.push_back(normals[index].z);
            }
            ++num_tris;
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

struct shader_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        GLuint shader = p;
        glDeleteShader(shader);
    }
};

struct program_deleter {
    using pointer = fake_nullable<GLuint>;
    void operator()(pointer p) const {
        GLuint program = p;
        glDeleteProgram(program);
    }
};

using unique_shader = unique_gl_resource<shader_deleter>;
using unique_program = unique_gl_resource<program_deleter>;

inline unique_shader make_unique_shader(GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    if (shader == 0) {
        throw runtime_error("Failed to create shader!");
    }
    return unique_shader(shader);
}

inline unique_program make_unique_program() {
    GLuint program = glCreateProgram();
    if (program == 0) {
        throw runtime_error("Failed to create shader program!");
    }
    return unique_program(program);
}

inline unique_shader compile_shader_file(GLenum shader_type, const string& fname) {
    auto lines = load_file(fname);

    vector<const GLchar*> line_pointers;
    line_pointers.reserve(lines.size());
    transform(begin(lines),end(lines),back_inserter(line_pointers),[](auto& line){return line.data();});

    unique_shader rv = make_unique_shader(shader_type);

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

template <typename T>
class const_reference_wrapper {
    const T* ptr = nullptr;
public:
    const_reference_wrapper() = default;
    const_reference_wrapper(const T& t) : ptr(&t) {}
    const T& get() const { return *ptr; }
};

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

struct texture_2d {
    unique_texture handle;
    int width = 0;
    int height = 0;
};

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

inline void set_program(const unique_program& program) {
    glUseProgram(program.get());
}

inline void set_texture(int slot, const texture_2d& tex) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex.handle.get());
}

inline void draw_mesh(const static_mesh& mesh) {
    glBindVertexArray(mesh.vao.get());
    glDrawArrays(GL_TRIANGLES, 0, mesh.num_triangles * 3);
}

} // namespace sushi

int main() {
    namespace sc = sushi::constants;

    auto window = sushi::window(800, 600, "Sushi Test", false);

    auto texture = sushi::load_texture_2d("assets/test.png", false, false);
    auto mesh = sushi::load_static_mesh("assets/test.obj");
    auto program = sushi::link_program({
        sushi::compile_shader_file(sc::shader_type::VERTEX, "assets/vert.glsl"),
        sushi::compile_shader_file(sc::shader_type::FRAGMENT, "assets/frag.glsl"),
    });

    window.main_loop([&]{
        sushi::set_program(program);
        sushi::set_texture(0, texture);
        sushi::draw_mesh(mesh);
    });
}
