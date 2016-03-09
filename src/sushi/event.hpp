#ifndef SUSHI_EVENT_HPP
#define SUSHI_EVENT_HPP

#include <boost/variant.hpp>
#include <glm/vec2.hpp>

namespace sushi {

struct event_keyboard_press {
    int key;
};

struct event_keyboard_release {
    int key;
};

struct event_mouse_press {
    int button;
};

struct event_mouse_release {
    int button;
};

struct event_mouse_move {
    glm::vec2 from;
};

using event_data = boost::variant<
    event_keyboard_press,
    event_keyboard_release,
    event_mouse_press,
    event_mouse_release,
    event_mouse_move
>;

struct event {
    event_data data;
    glm::vec2 pos;
};

} // namespace sushi

#endif // SUSHI_EVENT_HPP
