Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

$ErrorActionPreference = "Stop"
echo $dPath

Set-Item -path env:Path -value ($env:Path + ";..\loader\$dPath")
Set-Item -path env:Path -value ($env:Path + ";gtest-1.7.0\$dPath")
$env:VK_INSTANCE_LAYERS = "Threading;MemTracker;ObjectTracker;DrawState;ParamChecker;ShaderChecker"
$env:VK_DEVICE_LAYERS = "Threading;MemTracker;ObjectTracker;DrawState;ParamChecker;ShaderChecker"
$env:VK_LAYER_PATH = "..\layers\$dPath"

Copy-Item ..\..\tests\vk_layer_settings.txt .
If (-Not (Test-Path .\vk_layer_settings.txt)) {
    throw "Missing vk_layer_settings.txt file"
}

& $dPath\vkbase.exe
& $dPath\vk_blit_tests
& $dPath\vk_image_tests
& $dPath\vk_render_tests 

If (Test-Path .\vk_layer_settings.txt) {
    del vk_layer_settings.txt
}

