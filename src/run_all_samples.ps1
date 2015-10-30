# Be sure to run "Set-ExecutionPolicy RemoteSigned" before running powershell scripts

Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
    } else {
    $dPath = "Release"
}

echo "Instance"
& $dPath\vk0.9-instance
echo "Instance Layer Properties"
& $dPath\vk0.9-instance_layer_properties
echo "Instance Layer Properties"
& $dPath\vk0.9-instance_layer_properties
echo "Instance Extension Properties"
& $dPath\vk0.9-instance_extension_properties
echo "Instance Layer ExtensionProperties"
& $dPath\vk0.9-instance_layer_extension_properties
echo "Enumerate"
& $dPath\vk0.9-enumerate
echo "Device"
& $dPath\vk0.9-device
echo "Enable Validation and Debug Message Callback"
& $dPath\vk0.9-enable_validation_with_callback
echo "Init Swapchain"
& $dPath\vk0.9-initswapchain
echo "Init Command Buffer"
& $dPath\vk0.9-initcommandbuffer
echo "Depth Buffer"
& $dPath\vk0.9-depthbuffer
echo "Init Texture"
& $dPath\vk0.9-inittexture
echo "Uniform Buffer"
& $dPath\vk0.9-uniformbuffer
echo "Init RenderPass"
& $dPath\vk0.9-initrenderpass
echo "Init Framebuffer"
& $dPath\vk0.9-initframebuffers
echo "Descriptor and Pipeline Layouts"
& $dPath\vk0.9-descriptor_pipeline_layouts
echo "Vertex Buffer"
& $dPath\vk0.9-vertexbuffer
echo "Alloc Descriptor Sets"
& $dPath\vk0.9-allocdescriptorsets
echo "Initialize Shaders"
& $dPath\vk0.9-initshaders
echo "Initialize Pipeline"
& $dPath\vk0.9-initpipeline
echo "Draw Cube"
& $dPath\vk0.9-drawcube
echo "Draw Textured Cube"
& $dPath\vk0.9-copyblitimage
echo "Copy/Blit Image"
& $dPath\vk0.9-drawtexturedcube
echo "Draw Cubes with Dynamic Uniform Buffer"
& $dPath\vk0.9-dynamicuniform
echo "Texel Buffer"
& $dPath\vk0.9-texelbuffer
echo "Use GLSL Shader"
& $dPath\vk0.9-useglslshader
echo "Use SPIR-V Shader"
& $dPath\vk0.9-usespirvshader
