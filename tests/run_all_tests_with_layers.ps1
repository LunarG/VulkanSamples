Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

echo $dPath

Set-Item -path env:Path -value ($env:Path + ";..\loader\$dPath")
Set-Item -path env:Path -value ($env:Path + ";gtest-1.7.0\$dPath")
$env:VK_INSTANCE_LAYERS = "Threading:MemTracker:ObjectTracker:DrawState:ParamChecker:ShaderChecker"
$env:VK_DEVICE_LAYERS = "Threading:MemTracker:ObjectTracker:DrawState:ParamChecker:ShaderChecker"
$env:VK_LAYER_DIRS = "..\layers\$dPath"
OUTPUT_FLAGS="error"
OUTPUT_ACTION="VK_DBG_LAYER_ACTION_LOG_MSG"
$SETTINGS_NAME="vk_layer_settings.txt"

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

& $dPath\vkbase.exe
& $dPath\vk_blit_tests
& $dPath\vk_image_tests
& $dPath\vk_render_tests --compare-images

del $SETTINGS_NAME


