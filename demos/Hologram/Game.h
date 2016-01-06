#ifndef GAME_H
#define GAME_H

#include <string>

class Shell;

class Game {
public:
    Game(const Game &game) = delete;
    Game &operator=(const Game &game) = delete;

    struct Settings {
        std::string name;
        int initial_width;
        int initial_height;
        int queue_count;
        bool vsync;
        bool animate;
    };
    const Settings &settings() const { return settings_; }

    virtual void attach_shell(Shell &shell) { shell_ = &shell; }
    virtual void detach_shell() { shell_ = nullptr; }

    virtual void attach_swapchain() {}
    virtual void detach_swapchain() {}

    virtual void on_frame(float frame_time, int fb) {}
    virtual void on_key(int key) {}
    virtual void on_shutdown() {}

protected:
    Game(const std::string &name, const std::vector<std::string> &args)
        : settings_(), shell_(nullptr)
    {
        settings_.name = name;
        settings_.initial_width = 1280;
        settings_.initial_height = 1024;
        settings_.queue_count = 1;
        settings_.vsync = true;
        settings_.animate = true;

        for (auto it = args.begin(); it != args.end(); ++it) {
            if (*it == "-b")
                settings_.vsync = false;
            else if (*it == "-w")
                settings_.initial_width = std::stoi(*(it + 1));
            else if (*it == "-h")
                settings_.initial_height = std::stoi(*(it + 1));
        }
    }

    Settings settings_;
    Shell *shell_;
};

#endif // GAME_H
