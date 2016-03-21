#ifndef SUSHI_EVENT_HPP
#define SUSHI_EVENT_HPP

#include "gl.hpp"

#include <boost/variant.hpp>
#include <glm/vec2.hpp>

namespace sushi {

struct event_any {};
struct event_keyboard : event_any {};
struct event_mouse : event_any {};

struct event_keyboard_press : event_keyboard {
    int key;
    event_keyboard_press(int key) : key(key) {}
};

struct event_keyboard_release : event_keyboard {
    int key;
    event_keyboard_release(int key) : key(key) {}
};

struct event_mouse_press : event_mouse {
    int button;
    event_mouse_press(int button) : button(button) {}
};

struct event_mouse_release : event_mouse {
    int button;
    event_mouse_release(int button) : button(button) {}
};

struct event_mouse_move : event_mouse {
    glm::vec2 from;
    event_mouse_move(glm::vec2 from) : from(from) {}
};

struct event_char : event_keyboard {
    unsigned codepoint;
    event_char(unsigned codepoint) : codepoint(codepoint) {}
};

using event_data = boost::variant<
    event_keyboard_press,
    event_keyboard_release,
    event_mouse_press,
    event_mouse_release,
    event_mouse_move,
    event_char
>;

struct event {
    event_data data;
    glm::vec2 pos;
};

} // namespace sushi

#endif // SUSHI_EVENT_HPP
