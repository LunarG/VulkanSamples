#!/bin/bash

pushd $(dirname "$0") > /dev/null

vk_layer_path=$VK_LAYER_PATH:`pwd`/layers:../layers
ld_library_path=$LD_LIBRARY_PATH:`pwd`/layers:../layers

# Check for insertion of wrap-objects layer.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_wrap_objects \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Insertion test PASSED"

# Check for insertion of wrap-objects layer in front.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_parameter_validation:VK_LAYER_LUNARG_wrap_objects \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Front insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Front insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Front insertion test PASSED"

# Check for insertion of wrap-objects layer in back.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_wrap_objects:VK_LAYER_LUNARG_parameter_validation \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Back insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Back insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Back insertion test PASSED"

# Check for insertion of wrap-objects layer in middle.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_core_validation:VK_LAYER_LUNARG_wrap_objects:VK_LAYER_LUNARG_parameter_validation \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Middle insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Middle insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Middle insertion test PASSED"

# Run a sanity check to make sure the validation tests can be run in the current environment.
GTEST_PRINT_TIME=0 \
   VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   GTEST_FILTER=VkLayerTest.ReservedParameter \
   ./vk_layer_validation_tests > /dev/null
ec=$?

if [ $ec -ne 0 ]
then
   echo "Execution test FAILED - there may be a problem executing the layer validation tests" >&2
   exit 1
fi

# Pick a random subset of valid LVT tests to wrap -- None of these can use the device profile API extension!
filter=VkLayerTest.ThreadCommandBufferCollision:VkLayerTest.ImageDescriptorLayoutMismatch:VkLayerTest.CreateBufferViewNoMemoryBoundToBuffer:VkLayerTest.CommandBufferResetErrors:VkLayerTest.PSOLineWidthInvalid:VkLayerTest.EndCommandBufferWithinRenderPass:VkLayerTest.DSBufferLimitErrors:VkLayerTest.InvalidImageLayout:VkLayerTest.CreatePipelineVsFsMismatchByLocation:VkLayerTest.ImageBufferCopyTests:VkLayerTest.ClearImageErrors:VkPositiveLayerTest.NonCoherentMemoryMapping:VkPositiveLayerTest.BarrierLayoutToImageUsage:VkPositiveLayerTest.TwoSubmitInfosWithSemaphoreOneQueueSubmitsOneFence

# Run the layer validation tests with and without the wrap-objects layer. Diff the results.
# Filter out the "Unexpected:" lines because they contain varying object handles.
GTEST_PRINT_TIME=0 \
   VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   GTEST_FILTER=$filter \
   ./vk_layer_validation_tests | grep -v "^Unexpected: " > unwrapped.out
GTEST_PRINT_TIME=0 \
   VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_wrap_objects \
   GTEST_FILTER=$filter \
   ./vk_layer_validation_tests | grep -v "^Unexpected: " > wrapped.out
diff unwrapped.out wrapped.out
ec=$?

if [ $ec -eq 1 ]
then
   echo "Wrap-objects layer validation tests FAILED - wrap-objects altered the results of the layer validation tests" >&2
   exit 1
fi
echo "Wrap-objects layer validation tests PASSED"

popd > /dev/null

exit 0
