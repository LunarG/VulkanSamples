#!/bin/bash

pushd $(dirname "$0") > /dev/null

echo "Testing wrap-objects layer..."

# Don't print the elapsed time of each test.
export GTEST_PRINT_TIME=0

# Run the layer validation tests with and without the wrap-objects layer. Diff the results.
diff \
   <(./vk_layer_validation_tests) \
   <(VK_LAYER_PATH=$VK_LAYER_PATH:`pwd`/layers \
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/layers \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_wrap_objects \
   ./vk_layer_validation_tests) \

result=$?

popd > /dev/null

if [ $result -eq 1 ]
then
   echo "TEST FAILED - Diff detected" >&2
   exit 1
fi

echo "TEST PASSED"

exit 0
