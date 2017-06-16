/* Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
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
 *
 * Author: Tobin Ehlis <tobine@google.com>
 */

#ifndef LAYER_DATA_H
#define LAYER_DATA_H

#include <cassert>
#include <unordered_map>
#include "vk_layer_table.h"

// For the given data key, look up the layer_data instance from given layer_data_map
template <typename DATA_T>
DATA_T *GetLayerDataPtr(void *data_key, std::unordered_map<void *, DATA_T *> &layer_data_map) {
    DATA_T *debug_data;
    typename std::unordered_map<void *, DATA_T *>::const_iterator got;

    /* TODO: We probably should lock here, or have caller lock */
    got = layer_data_map.find(data_key);

    if (got == layer_data_map.end()) {
        debug_data = new DATA_T;
        layer_data_map[(void *)data_key] = debug_data;
    } else {
        debug_data = got->second;
    }

    return debug_data;
}

template <typename DATA_T>
void FreeLayerDataPtr(void *data_key, std::unordered_map<void *, DATA_T *> &layer_data_map) {
    auto got = layer_data_map.find(data_key);
    assert(got != layer_data_map.end());

    delete got->second;
    layer_data_map.erase(got);
}

#endif  // LAYER_DATA_H
