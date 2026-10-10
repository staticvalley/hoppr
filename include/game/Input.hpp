#pragma once
#include <array>
#include <iosfwd>
#include <Log.hpp>
#include <fstream>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

enum class Action : uint8_t {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,

    JUMP,
    CROUCH,
    SNEAK,

    NOCLIP,
    DEBUG,

    // just used to count how many actions are possible dynamically
    IMPL_ACTION_COUNT
};

/// commands for binding file to be translated to actions
static const std::unordered_map<std::string, Action> ActionCommands = {
    {"FORWARD", Action::MOVE_FORWARD},
    {"BACKWARD", Action::MOVE_BACKWARD},
    {"LEFT", Action::MOVE_LEFT},
    {"RIGHT", Action::MOVE_RIGHT},

    {"JUMP", Action::JUMP},
    {"CROUCH", Action::CROUCH},
    {"SNEAK", Action::SNEAK},

    {"NOCLIP", Action::NOCLIP},
    {"DEBUG", Action::DEBUG},
};

enum class BindingType : uint8_t {
    KEY,
    MOUSE_BUTTON,
    MOUSE_WHEEL_UP,
    MOUSE_WHEEL_DOWN,
};

struct Binding {

    BindingType type;
    int scanCode = 0;

    static Binding key(SDL_Scancode sdlScanCode) { return {BindingType::KEY, (int)sdlScanCode}; }
    static Binding mouse(int button) { return {BindingType::MOUSE_BUTTON, button}; }
    static Binding wheelUp() { return {BindingType::MOUSE_WHEEL_UP, 0}; }
    static Binding wheelDown() { return {BindingType::MOUSE_WHEEL_DOWN, 0}; }
};

class Input {
public:

    /**
     * @brief add binding for some action
     * @param action action to add binding to
     * @param binding binding for action
     */
    void bind(Action action, Binding binding);

    /**
     * @brief unbind all bindings for some action
     * @param action action to unbind all on
     */
    void unbind(Action action);

    /**
     * @brief pass event to input to handle special bindings
     * @note this is mostly used for mouse scroll wheel updates
     * @param event
     */
    void handleEvent(const SDL_Event* event);

    /**
     * @brief reset mouse wheel booleans for polling events
     * @note should be called on each frame update
     */
    void resetMouseWheelPoll();

    /**
     * @brief check if any bindings for some action have been pressed
     * @param action action to search for
     * @return if any bindings for the action have been pressed @code(true == pressed, false == not pressed)
     */
    bool poll(Action action);

    /**
     * @brief sets binds given a file in a @code ACTION <space> BINDING @endcode format.
     * example:
     * @code
     * FORWARD 26
     * BACKWARD 22
     * LEFT 4
     * RIGHT 7
     * JUMP 44
     * JUMP MWHEELUP
     * JUMP MWHEELDOWN
     * CROUCH 224
     * SNEAK 225
     * @endcode
     * @param filePath file with bindings
     */
    void setBindsFromFile(const char* filePath);

private:
    std::array<std::vector<Binding>, (size_t)Action::IMPL_ACTION_COUNT> bindings;
    bool wheelUp = false;
    bool wheelDown = false;
};