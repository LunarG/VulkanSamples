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

#include <sstream>

#include "Game.h"
#include "Shell.h"

void Game::print_stats() {
    // Output frame count and measured elapsed time
    auto now = std::chrono::system_clock::now();
    auto elapsed = now - start_time;
    auto elapsed_millis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    std::stringstream ss;
    ss << "frames:" << frame_count << ", elapsedms:" << elapsed_millis;
    shell_->log(Shell::LogPriority::LOG_INFO, ss.str().c_str());
}

void Game::quit() {
    print_stats();
    shell_->quit();
}
