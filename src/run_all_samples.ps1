# Be sure to run "Set-ExecutionPolicy RemoteSigned" before running powershell scripts

Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
    } else {
    $dPath = "Release"
}

echo "Instance"
& $dPath\vk0.2-instance
echo "Instance Layer Properties"
& $dPath\vk0.2-instance_layer_properties
echo "Instance Layer Properties"
& $dPath\vk0.2-instance_layer_properties
echo "Instance Extension Properties"
& $dPath\vk0.2-instance_extension_properties
echo "Instance Layer ExtensionProperties"
& $dPath\vk0.2-instance_layer_extension_properties
echo "Enumerate"
& $dPath\vk0.2-enumerate
echo "Device"
& $dPath\vk0.2-device
echo "Enable Validation and Debug Message Callback"
& $dPath\vk0.2-enable_validation_with_callback
echo "Init WSI"
& $dPath\vk0.2-initwsi
echo "Init Command Buffer"
& $dPath\vk0.2-initcommandbuffer
echo "Depth Buffer"
& $dPath\vk0.2-depthbuffer
echo "Init Texture"
& $dPath\vk0.2-initTexture
echo "Uniform Buffer"
& $dPath\vk0.2-uniformbuffer
echo "Init RenderPass"
& $dPath\vk0.2-initrenderpass
echo "Init Framebuffer"
& $dPath\vk0.2-initframebuffers
echo "Descriptor and Pipeline Layouts"
& $dPath\vk0.2-descriptor_pipeline_layouts
echo "Vertex Buffer"
& $dPath\vk0.2-vertexbuffer
echo "Dynamic State"
& $dPath\vk0.2-dynamicstate
echo "Use GLSL Shader"
& $dPath\vk0.2-useglslshader
echo "Use SPIR-V Shader"
& $dPath\vk0.2-usespirvshader
