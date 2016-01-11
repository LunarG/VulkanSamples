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
$env:VK_LAYER_PATH = "..\layers\$dPath"

.\vktracereplay.ps1 "-$dPath"
.\vkvalidatelayerdoc.ps1

$env:VK_INSTANCE_LAYERS = "VK_LAYER_LUNARG_threading;VK_LAYER_LUNARG_mem_tracker;VK_LAYER_LUNARG_object_tracker;VK_LAYER_LUNARG_draw_state;VK_LAYER_LUNARG_param_checker;VK_LAYER_LUNARG_swapchain;VK_LAYER_LUNARG_device_limits;VK_LAYER_LUNARG_image"
$env:VK_DEVICE_LAYERS = "VK_LAYER_LUNARG_threading;VK_LAYER_LUNARG_mem_tracker;VK_LAYER_LUNARG_object_tracker;VK_LAYER_LUNARG_draw_state;VK_LAYER_LUNARG_param_checker;VK_LAYER_LUNARG_swapchain;VK_LAYER_LUNARG_device_limits;VK_LAYER_LUNARG_image"

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

#unset the layer enviroment variables before running the layer validation tests
$env:VK_INSTANCE_LAYERS = ""
$env:VK_DEVICE_LAYERS = ""

& $dPath\vk_layer_validation_tests

