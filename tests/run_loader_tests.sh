#!/bin/bash

pushd $(dirname "$0") > /dev/null

./vk_loader_validation_tests

# Check for layer insertion via CreateInstance.
output=$(VK_LOADER_DEBUG=all \
   GTEST_FILTER=CreateInstance.LayerPresent \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_parameter_validation"
ec=$?

if [ $ec -eq 1 ]
then
   echo "CreateInstance insertion test FAILED - parameter-validation not detected in instance layers" >&2
   exit 1
fi
echo "CreateInstance Insertion test PASSED"

# Test the wrap objects layer.
./run_wrap_objects_tests.sh

popd > /dev/null
