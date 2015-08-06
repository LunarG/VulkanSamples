Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

echo $dPath

Set-Item -path env:Path -value ($env:Path + ";..\loader\$dPath")
Set-Item -path env:Path -value ($env:Path + ";gtest-1.7.0\$dPath")
$env:VK_INSTANCE_LAYERS = "Threading;MemTracker;ObjectTracker;DrawState;ParamChecker;ShaderChecker"
$env:VK_DEVICE_LAYERS = "Threading;MemTracker;ObjectTracker;DrawState;ParamChecker;ShaderChecker"
$env:VK_LAYER_PATH = "..\layers\$dPath"

& $dPath\vkbase.exe
& $dPath\vk_blit_tests
& $dPath\vk_image_tests
& $dPath\vk_render_tests --compare-images

del $SETTINGS_NAME


