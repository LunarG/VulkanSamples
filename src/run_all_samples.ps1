# Be sure to run "Set-ExecutionPolicy RemoteSigned" before running powershell scripts

Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
    } else {
    $dPath = "Release"
}

echo "Instance"
& $dPath\vk1.0-instance
echo "Instance Layer Properties"
& $dPath\vk1.0-instance_layer_properties
echo "Instance Layer Properties"
& $dPath\vk1.0-instance_layer_properties
echo "Instance Extension Properties"
& $dPath\vk1.0-instance_extension_properties
echo "Instance Layer ExtensionProperties"
& $dPath\vk1.0-instance_layer_extension_properties
echo "Enumerate"
& $dPath\vk1.0-enumerate
echo "Device"
& $dPath\vk1.0-device
echo "Enable Validation and Debug Message Callback"
& $dPath\vk1.0-enable_validation_with_callback
echo "Init Swapchain"
& $dPath\vk1.0-initswapchain
echo "Init Command Buffer"
& $dPath\vk1.0-initcommandbuffer
echo "Depth Buffer"
& $dPath\vk1.0-depthbuffer
echo "Init Texture"
& $dPath\vk1.0-inittexture
echo "Uniform Buffer"
& $dPath\vk1.0-uniformbuffer
echo "Init RenderPass"
& $dPath\vk1.0-initrenderpass
echo "Init Framebuffer"
& $dPath\vk1.0-initframebuffers
echo "Descriptor and Pipeline Layouts"
& $dPath\vk1.0-descriptor_pipeline_layouts
echo "Vertex Buffer"
& $dPath\vk1.0-vertexbuffer
echo "Alloc Descriptor Sets"
& $dPath\vk1.0-allocdescriptorsets
echo "Initialize Shaders"
& $dPath\vk1.0-initshaders
echo "Initialize Pipeline"
& $dPath\vk1.0-initpipeline
echo "Draw Cube"
& $dPath\vk1.0-drawcube
echo "Draw Textured Cube"
& $dPath\vk1.0-copyblitimage
echo "Copy/Blit Image"
& $dPath\vk1.0-drawtexturedcube
echo "Draw Cubes with Dynamic Uniform Buffer"
& $dPath\vk1.0-dynamicuniform
echo "Texel Buffer"
& $dPath\vk1.0-texelbuffer
echo "Use GLSL Shader"
& $dPath\vk1.0-useglslshader
echo "Use SPIR-V Shader"
& $dPath\vk1.0-usespirvshader
