/*
 * Copyright (C) 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GAME_H
#define GAME_H

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

class Shell;

class Game {
   public:
    Game(const Game &game) = delete;
    Game &operator=(const Game &game) = delete;
    virtual ~Game() {}

    struct Settings {
        std::string name;
        int initial_width;
        int initial_height;
        int queue_count;
        int back_buffer_count;
        int ticks_per_second;
        bool vsync;
        bool animate;

        bool validate;
        bool validate_verbose;

        bool no_tick;
        bool no_render;
        bool no_present;

        // Whether or not to use VkFlushMappedMemoryRanges
        bool flush_buffers;

        int max_frame_count;
    };
    const Settings &settings() const { return settings_; }

    virtual void attach_shell(Shell &shell) { shell_ = &shell; }
    virtual void detach_shell() { shell_ = nullptr; }

    virtual void attach_swapchain() {}
    virtual void detach_swapchain() {}

    enum Key {
        // virtual keys
        KEY_SHUTDOWN,
        // physical keys
        KEY_UNKNOWN,
        KEY_ESC,
        KEY_UP,
        KEY_DOWN,
        KEY_SPACE,
    };
    virtual void on_key(Key key) {}
    virtual void on_tick() {}

    virtual void on_frame(float frame_pred) {}

    void print_stats();
    void quit();

   protected:
    int frame_count;
    std::chrono::time_point<std::chrono::system_clock> start_time;

    Game(const std::string &name, const std::vector<std::string> &args) : settings_(), shell_(nullptr) {
        settings_.name = name;
        settings_.initial_width = 1280;
        settings_.initial_height = 1024;
        settings_.queue_count = 1;
        settings_.back_buffer_count = 1;
        settings_.ticks_per_second = 30;
        settings_.vsync = true;
        settings_.animate = true;

        settings_.validate = false;
        settings_.validate_verbose = false;

        settings_.no_tick = false;
        settings_.no_render = false;
        settings_.no_present = false;

        settings_.flush_buffers = false;

        settings_.max_frame_count = -1;

        parse_args(args);

        frame_count = 0;
        // Record start time for printing stats later
        start_time = std::chrono::system_clock::now();
    }

    Settings settings_;
    Shell *shell_;

   private:
    void parse_args(const std::vector<std::string> &args) {
        for (auto it = args.begin(); it != args.end(); ++it) {
            if (*it == "--b") {
                settings_.vsync = false;
            } else if (*it == "--w") {
                ++it;
                settings_.initial_width = std::stoi(*it);
            } else if (*it == "--h") {
                ++it;
                settings_.initial_height = std::stoi(*it);
            } else if (*it == "--v") {
                settings_.validate = true;
            } else if (*it == "--validate") {
                settings_.validate = true;
            } else if (*it == "--vv") {
                settings_.validate = true;
                settings_.validate_verbose = true;
            } else if (*it == "--nt") {
                settings_.no_tick = true;
            } else if (*it == "--nr") {
                settings_.no_render = true;
            } else if (*it == "--np") {
                settings_.no_present = true;
            } else if (*it == "--flush") {
                settings_.flush_buffers = true;
            } else if (*it == "--c") {
                ++it;
                settings_.max_frame_count = std::stoi(*it);
            }
        }
    }
};

#endif  // GAME_H
