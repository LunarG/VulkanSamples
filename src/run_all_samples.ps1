# Be sure to run "Set-ExecutionPolicy RemoteSigned" before running powershell scripts

Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
    } else {
    $dPath = "Release"
}

echo "Depth Buffer"
& $dPath\vk0.2-depthbuffer
echo "Device"
& $dPath\vk0.2-device
echo "Enumerate"
& $dPath\vk0.2-enumerate
echo "Init Texture"
& $dPath\vk0.2-initTexture
echo "Init WSI"
& $dPath\vk0.2-initwsi
echo "Instance"
& $dPath\vk0.2-instance
echo "Instance Layer Properties"
& $dPath\vk0.2-instance_layer_properties
echo "Use GLSL Shader"
& $dPath\vk0.2-useglslshader
echo "Use SPIR-V Shader"
& $dPath\vk0.2-usespirvshader
