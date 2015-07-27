//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_KEY_ARRAY_HPP
#define SUSHI_KEY_ARRAY_HPP

#include <array>

/// Sushi
namespace sushi {

/// Array of key states. Keeps track of presses and releases.
/// \tparam NKeys Number of keys to track.
template<int NKeys>
class key_array {
    struct keystate {
        int when_last_pressed = 0;
        int when_last_released = 0;
    };
    std::array<keystate, NKeys> keystates;
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
    bool was_pressed(int k, int t) const {
        auto ks = keystates[k];
        return t < ks.when_last_pressed;
    }

    /// Checks if key `k` was last released after time `t`.
    /// \pre `k>0` and `k<NKeys`.
    /// \param k Index of key to check.
    /// \param t Time after which releases are detected.
    /// \return True if `t<x` for `x` in the last call to `release(k,x)`.
    bool was_released(int k, int t) const {
        auto ks = keystates[k];
        return t < ks.when_last_released;
    }

    /// Checks if key `k` is currently pressed.
    /// \pre `k>0` and `k<NKeys`.
    /// \param k Index of key to check.
    /// \return True if `x<y` for `x` and `y` in the last calls to `release(k,x)` and `press(k,y)`.
    bool is_down(int k) const {
        auto ks = keystates[k];
        return ks.when_last_released < ks.when_last_pressed;
    }

    /// Checks if key `k` is currently not pressed.
    /// \pre `k>0` and `k<NKeys`.
    /// \param k Index of key to check.
    /// \return Negation of `is_down(k)`.
    bool is_up(int k) const {
        return !is_down(k);
    }
};

}

#endif //SUSHI_KEY_ARRAY_HPP
