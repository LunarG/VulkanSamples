Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

echo $dPath

Set-Item -path env:Path -value ($env:Path + ";..\loader\$dPath")
Set-Item -path env:Path -value ($env:Path + ";gtest-1.7.0\$dPath")
$env:VK_LAYER_NAMES = "Validation"

$SETTINGS_NAME="vk_layer_settings.txt"
$OUTPUT_LEVEL="VK_DBG_LAYER_LEVEL_ERROR"

echo "MemTrackerReportLevel = $OUTPUT_LEVEL" > $SETTINGS_NAME
echo "DrawStateReportLevel = $OUTPUT_LEVEL" >> $SETTINGS_NAME
echo "ObjectTrackerReportLevel = $OUTPUT_LEVEL" >> $SETTINGS_NAME
echo "ParamCheckerReportLevel = $OUTPUT_LEVEL" >> $SETTINGS_NAME
echo "ThreadingReportLevel = $OUTPUT_LEVEL" >> $SETTINGS_NAME

& $dPath\vkbase.exe
& $dPath\vk_blit_tests
& $dPath\vk_image_tests
& $dPath\vk_render_tests --compare-images

del $SETTINGS_NAME


