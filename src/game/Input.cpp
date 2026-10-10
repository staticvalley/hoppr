#include <Input.hpp>

void Input::bind(Action action, Binding binding) { bindings[(size_t)action].push_back(binding); }
void Input::unbind(Action action) { bindings[(size_t)action].clear(); }

void Input::handleEvent(const SDL_Event* event) {
    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        if (event->wheel.y > 0) wheelUp = true;
        if (event->wheel.y < 0) wheelDown = true;
    }
}

void Input::resetMouseWheelPoll() {
    wheelUp = false;
    wheelDown = false;
}

bool Input::poll(Action action) {
    const bool* keys = SDL_GetKeyboardState(nullptr);
    SDL_MouseButtonFlags mouse = SDL_GetMouseState(nullptr, nullptr);

    for (const Binding& b : bindings[(size_t)action]) {
        switch (b.type) {
            case BindingType::KEY:                if (keys[b.scanCode]) return true; break;
            case BindingType::MOUSE_BUTTON:       if (mouse & SDL_BUTTON_MASK(b.scanCode)) return true; break;
            case BindingType::MOUSE_WHEEL_UP:     if (wheelUp) return true; break;
            case BindingType::MOUSE_WHEEL_DOWN:   if (wheelDown) return true; break;
        }
    }
    return false;
}

void Input::setBindsFromFile(const char* filePath) {
    std::ifstream bindConfig(filePath);

    if (!bindConfig.is_open()) {
        Log::error(Log::Channel::GAME, "Bind config file @ '{}' can not be opened.", filePath);
        return;
    }

    std::string line;
    for (size_t lineNumber = 0; std::getline(bindConfig, line); lineNumber++) {
        // get rid of windows formatting
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        size_t splitPosition = line.find(' ');
        if (splitPosition == std::string::npos) {
            Log::warn(Log::Channel::GAME, "Bind config syntax error on line #{} (no space?). Skipping.", lineNumber);
            continue;
        }

        // get command and bind pair from line
        std::string_view actionStringView{line.data(), splitPosition};
        std::string_view bindStringView{line.data() + splitPosition + 1, line.size() - splitPosition - 1};

        // empty line
        if (actionStringView.empty() || bindStringView.empty()) {
            continue;
        }

        Action action;

        std::string actionString(actionStringView);
        if (ActionCommands.contains(actionString)) {
            action = ActionCommands.at(actionString);
        } else {
            Log::warn(Log::Channel::GAME, "Bind config syntax error on line #{}. Not a valid command. Skipping.",
                      lineNumber);
            continue;
        }

        Binding binding;

        std::string bindString(bindStringView);
        // mouse wheel binds
        if (bindString == "MWHEELUP") {
            binding = Binding::wheelUp();
        } else if (bindString == "MWHEELDOWN") {
            binding = Binding::wheelDown();
        }
        // mouse button binds
        else if ((bindString[0] == 'M' || bindString[0] == 'm') && bindString.size() > 1) {
            int buttonNumber = 0;

            // Attempt to parse everything after the 'M' as an integer
            auto [ptr, ec] = std::from_chars(bindString.data(), bindString.data() + bindString.size(), buttonNumber);

            if (ec == std::errc()) {
                binding = Binding::mouse(buttonNumber);
            } else {
                Log::warn(Log::Channel::GAME, "Invalid mouse button number on line #{}. Skipping.", lineNumber);
                continue;
            }
        }
        // normal keyboard scancode
        else {
            int scanCode = 0;

            auto [ptr, ec] = std::from_chars(bindString.data(), bindString.data() + bindString.size(), scanCode);

            if (ec == std::errc() && ptr == bindString.data() + bindString.size()) {
                binding = Binding::key((SDL_Scancode) scanCode);
            } else {
                Log::warn(Log::Channel::GAME, "Invalid bind scancode '{}' on line #{}. Skipping.", bindString,
                          lineNumber);
                continue;
            }
        }

        bind(action, binding);
    }
}


