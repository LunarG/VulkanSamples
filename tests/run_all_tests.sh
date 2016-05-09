#!/bin/bash
#
# Run all the regression tests
cd $(dirname "$0")

# Halt on error
set -e

#Verify that the loader is working
./vk_loader_validation_tests

# Verify that validation checks in source match documentation
./vkvalidatelayerdoc.sh

# vk_layer_validation_tests check to see that validation layers will
# catch the errors that they are supposed to by intentionally doing things
# that are wrong
./vk_layer_validation_tests

