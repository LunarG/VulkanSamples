#!/bin/bash
#
# Run all the regression tests

# xglbase tests that basic XGL calls are working (don't return an error).
./xglbase

# xgl_blit_tests test Fill/Copy Memory, Clears, CopyMemoryToImage
./xgl_blit_tests

# xgl_image_tests check that image can be allocated and bound.
./xgl_image_tests

#xgl_render_tests tests a variety of features using rendered images
# --compare-images will cause the test to check the resulting image against
# a saved "golden" image and will report an error if there is any difference
./xgl_render_tests --compare-images
