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

#include "Game.h"
#include "Shell.h"

#if (defined(_MSC_VER) && _MSC_VER < 1900 /*vs2015*/) ||                       \
    defined MINGW_HAS_SECURE_API
#include <basetsd.h>
#define snprintf sprintf_s
#endif

void Game::print_stats() {
    // Output frame count and measured elapsed time
    auto now = std::chrono::system_clock::now();
    auto elapsed = now - start_time;
    auto elapsed_millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    char msg[256];
    snprintf(msg, 255, "frames:%d, elapsedms:%ld", frame_count, elapsed_millis);
    shell_->log(Shell::LogPriority::LOG_INFO, msg);
}

void Game::quit() {
    print_stats();
    shell_->quit();
}
