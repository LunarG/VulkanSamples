#!/bin/bash
#
# Run all the regression tests with validation layers enabled

# enable layers
export LIBXGL_LAYER_NAMES=DrawState:MemTracker:ParamChecker:ObjectTracker
# Save any existing settings file
RESTORE_SETTINGS="false"
SETTINGS_NAME="xgl_layer_settings.txt"
TMP_SETTINGS_NAME="xls.txt"
OUTPUT_LEVEL="XGL_DBG_LAYER_LEVEL_ERROR"
if [ -f $SETTINGS_NAME ]; then
    echo Saving $SETTINGS_NAME to $TMP_SETTINGS_NAME
    RESTORE_SETTINGS="true"
    mv $SETTINGS_NAME $TMP_SETTINGS_NAME
else
    echo not copying layer settings
fi
# Write out settings file to run tests with
echo "MemTrackerReportLevel = $OUTPUT_LEVEL" > $SETTINGS_NAME
echo "DrawStateReportLevel = $OUTPUT_LEVEL" >> $SETTINGS_NAME
echo "ObjectTrackerReportLevel = $OUTPUT_LEVEL" >> $SETTINGS_NAME
echo "ParamCheckerReportLevel = $OUTPUT_LEVEL" >> $SETTINGS_NAME

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

if [ "$RESTORE_SETTINGS" = "true" ]; then
    echo Restore $SETTINGS_NAME from $TMP_SETTINGS_NAME
    mv $TMP_SETTINGS_NAME $SETTINGS_NAME
fi

