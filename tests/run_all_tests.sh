#!/bin/bash
#
# Run all the regression tests

# vkbase tests that basic VK calls are working (don't return an error).
./vkbase

# vk_blit_tests test Fill/Copy Memory, Clears, CopyMemoryToImage
./vk_blit_tests

# vk_image_tests check that image can be allocated and bound.
./vk_image_tests

#vk_render_tests tests a variety of features using rendered images
# --compare-images will cause the test to check the resulting image against
# a saved "golden" image and will report an error if there is any difference
./vk_render_tests --compare-images

# vk_layer_validation_tests check to see that validation layers will
# catch the errors that they are supposed to by intentionally doing things
# that are wrong
./vk_layer_validation_tests
