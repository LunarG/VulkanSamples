#!/bin/bash
#
# Run all the regression tests with validation layers enabled

# enable layers
export VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_Threading:VK_LAYER_LUNARG_MemTracker:VK_LAYER_LUNARG_ObjectTracker:VK_LAYER_LUNARG_DrawState:VK_LAYER_LUNARG_ParamChecker:VK_LAYER_LUNARG_Swapchain:VK_LAYER_LUNARG_DeviceLimits:VK_LAYER_LUNARG_Image
export VK_DEVICE_LAYERS=VK_LAYER_LUNARG_Threading:VK_LAYER_LUNARG_MemTracker:VK_LAYER_LUNARG_ObjectTracker:VK_LAYER_LUNARG_DrawState:VK_LAYER_LUNARG_ParamChecker:VK_LAYER_LUNARG_Swapchain:VK_LAYER_LUNARG_DeviceLimits:VK_LAYER_LUNARG_Image

set -e

if [ -f ../../tests/vk_layer_settings.txt ];
then
    cp ../../tests/vk_layer_settings.txt .
fi 

if [ ! -f ./vk_layer_settings.txt ];
then
    printf "ERROR:  No vk_layer_settings.txt file found\n"
    exit 1;
fi

# vkbase tests that basic VK calls are working (don't return an error).
./vkbase

# vk_blit_tests test Fill/Copy Memory, Clears, CopyMemoryToImage
./vk_blit_tests

# vk_image_tests check that image can be allocated and bound.
./vk_image_tests

#vk_render_tests tests a variety of features using rendered images
# --compare-images will cause the test to check the resulting image against
# a saved "golden" image and will report an error if there is any difference
./vk_render_tests # TODO: Reenable after fixing test framework for correct image layouts --compare-images

# vktracereplay.sh tests vktrace trace and replay
./vktracereplay.sh

if [ -f ./vk_layer_settings.txt ];
then
    rm ./vk_layer_settings.txt
fi
