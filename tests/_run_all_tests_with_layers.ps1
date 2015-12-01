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
$env:VK_INSTANCE_LAYERS = "VK_LAYER_LUNARG_Threading;VK_LAYER_LUNARG_MemTracker;VK_LAYER_LUNARG_ObjectTracker;VK_LAYER_LUNARG_DrawState;VK_LAYER_LUNARG_ParamChecker;VK_LAYER_LUNARG_Swapchain;VK_LAYER_LUNARG_DeviceLimits;VK_LAYER_LUNARG_Image"
$env:VK_DEVICE_LAYERS = "VK_LAYER_LUNARG_Threading;VK_LAYER_LUNARG_MemTracker;VK_LAYER_LUNARG_ObjectTracker;VK_LAYER_LUNARG_DrawState;VK_LAYER_LUNARG_ParamChecker;VK_LAYER_LUNARG_Swapchain;VK_LAYER_LUNARG_DeviceLimits;VK_LAYER_LUNARG_Image"
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

