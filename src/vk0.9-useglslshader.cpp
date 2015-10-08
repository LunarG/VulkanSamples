/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
build sample for config when external glsl shaders used
*/

/*
VULKAN_SAMPLE_DESCRIPTION_START
Example to demonstrate a build target for a sample that uses glsl shaders.
The build converts the glsl shaders are to spirv.  See CMakeLists.txt.
VULKAN_SAMPLE_DESCRIPTION_END
*/

#include <iostream>
#include <cstdlib>
#include "util.hpp"

#define APP_SHORT_NAME "vulkansamples_glslshader"

int main(int argc, char **argv)
{

    // initialize the VkApplicationInfo structure
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = APP_SHORT_NAME;
    app_info.appVersion = 1;
    app_info.pEngineName = APP_SHORT_NAME;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION;

    // initialize the VkInstanceCreateInfo structure
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_info;
    inst_info.pAllocCb = NULL;
    inst_info.extensionCount = 0;
    inst_info.ppEnabledExtensionNames = NULL;

    VkInstance inst;
    VkResult res;

    res = vkCreateInstance(&inst_info, &inst);
    if (res == VK_ERROR_INCOMPATIBLE_DRIVER) {
        std::cout << "cannot find a compatible Vulkan ICD\n";
        exit(-1);
    } else if (res) {
        std::cout << "unknown error\n";
        exit(-1);
    }

    /* VULKAN_KEY_START */
    std::string basedatadir = get_base_data_dir();
    std::string reldatadir = get_data_dir( __FILE__ );
    /* VULKAN_KEY_END */

    vkDestroyInstance(inst);

    return 0;
}

