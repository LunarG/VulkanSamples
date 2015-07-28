#!/bin/bash
#
# Run all the regression tests with validation layers enabled

# enable layers
export VK_INSTANCE_LAYERS=Threading:MemTracker:ObjectTracker:DrawState:ParamChecker:ShaderChecker
export VK_DEVICE_LAYERS=Threading:MemTracker:ObjectTracker:DrawState:ParamChecker:ShaderChecker
# Save any existing settings file
RESTORE_SETTINGS="false"
SETTINGS_NAME="vk_layer_settings.txt"
TMP_SETTINGS_NAME="xls.txt"
OUTPUT_FLAGS="error"
OUTPUT_ACTION="VK_DBG_LAYER_ACTION_LOG_MSG"
if [ -f $SETTINGS_NAME ]; then
    echo Saving $SETTINGS_NAME to $TMP_SETTINGS_NAME
    RESTORE_SETTINGS="true"
    mv $SETTINGS_NAME $TMP_SETTINGS_NAME
else
    echo not copying layer settings
fi
# Write out settings file to run tests with
echo "MemTrackerReportFlags = $OUTPUT_FLAGS" > $SETTINGS_NAME
echo "MemTrackerDebugAction = $OUTPUT_ACTION" >> $SETTINGS_NAME
echo "DrawStateReportFlags = $OUTPUT_FLAGS" >> $SETTINGS_NAME
echo "DrawStateDebugAction = $OUTPUT_ACTION" >> $SETTINGS_NAME
echo "ObjectTrackerReportFlags = $OUTPUT_FLAGS" >> $SETTINGS_NAME
echo "ObjectTrackerDebugAction = $OUTPUT_ACTION" >> $SETTINGS_NAME
echo "ParamCheckerReportFlags = $OUTPUT_FLAGS" >> $SETTINGS_NAME
echo "ParamCheckerDebugAction = $OUTPUT_ACTION" >> $SETTINGS_NAME
echo "ThreadingReportFlags = $OUTPUT_FLAGS" >> $SETTINGS_NAME
echo "ThreadingDebugAction = $OUTPUT_ACTION" >> $SETTINGS_NAME
echo "ShaderCheckerReportFlags = $OUTPUT_FLAGS" >> $SETTINGS_NAME
echo "ShaderCheckerDebugAction = $OUTPUT_ACTION" >> $SETTINGS_NAME

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

# vkglavetracereplay.sh tests glave trace and replay
D=`dirname \`pwd\``
GDIR=../../../Glave/`basename $D`/
if [ ! -d "$GDIR" ]; then
   printf "Glave unavailable, skipping vkglavetracereplay.sh\n"
else
   ./vkglavetracereplay.sh
fi

if [ "$RESTORE_SETTINGS" = "true" ]; then
    echo Restore $SETTINGS_NAME from $TMP_SETTINGS_NAME
    mv $TMP_SETTINGS_NAME $SETTINGS_NAME
fi

