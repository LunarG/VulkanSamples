# Be sure to run "Set-ExecutionPolicy RemoteSigned" before running powershell scripts

Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
    } else {
    $dPath = "Release"
}

echo "Instance"
& $dPath\vk0.10-instance
echo "Instance Layer Properties"
& $dPath\vk0.10-instance_layer_properties
echo "Instance Layer Properties"
& $dPath\vk0.10-instance_layer_properties
echo "Instance Extension Properties"
& $dPath\vk0.10-instance_extension_properties
echo "Instance Layer ExtensionProperties"
& $dPath\vk0.10-instance_layer_extension_properties
echo "Enumerate"
& $dPath\vk0.10-enumerate
echo "Device"
& $dPath\vk0.10-device
echo "Enable Validation and Debug Message Callback"
& $dPath\vk0.10-enable_validation_with_callback
echo "Create Debug Message Callback"
& $dPath\vk0.10-dbgcreatemsgcallback
echo "Init Swapchain"
& $dPath\vk0.10-initswapchain
echo "Init Command Buffer"
& $dPath\vk0.10-initcommandbuffer
echo "Depth Buffer"
& $dPath\vk0.10-depthbuffer
echo "Init Texture"
& $dPath\vk0.10-inittexture
echo "Uniform Buffer"
& $dPath\vk0.10-uniformbuffer
echo "Init RenderPass"
& $dPath\vk0.10-initrenderpass
echo "Init Framebuffer"
& $dPath\vk0.10-initframebuffers
echo "Descriptor and Pipeline Layouts"
& $dPath\vk0.10-descriptor_pipeline_layouts
echo "Vertex Buffer"
& $dPath\vk0.10-vertexbuffer
echo "Alloc Descriptor Sets"
& $dPath\vk0.10-allocdescriptorsets
echo "Initialize Shaders"
& $dPath\vk0.10-initshaders
echo "Initialize Pipeline"
& $dPath\vk0.10-initpipeline
echo "Draw Cube"
& $dPath\vk0.10-drawcube
echo "Copy/Blit Image"
& $dPath\vk0.10-copyblitimage
echo "Draw Textured Cube"
& $dPath\vk0.10-drawtexturedcube
echo "Draw Cubes with Dynamic Uniform Buffer"
& $dPath\vk0.10-dynamicuniform
echo "Texel Buffer"
& $dPath\vk0.10-texelbuffer
echo "Immutable Sampler"
& $dPath\vk0.10-immutable_sampler
echo "Multiple Descriptor Sets"
& $dPath\vk0.10-multiple_sets
echo "Multithreaded Command Buffers"
& $dPath\vk0.10-multithreadcmdbuf
echo "Push Constants"
& $dPath\vk0.10-push_constants
echo "Separate image sampler"
& $dPath\vk0.10-separate_image_sampler
echo "Draw Sub-passes"
& $dPath\vk0.10-drawsubpasses
echo "Occlusion Query"
& $dPath\vk0.10-occlusion_query
echo "Pipeline Cache"
& $dPath\vk0.10-pipeline_cache
echo "Pipeline Derivative"
& $dPath\vk0.10-pipeline_derivative
echo "Secondary Command Buffers"
& $dPath\vk0.10-secondarycmd
echo "SPIR-V Assembly"
& $dPath\vk0.10-spirv_assembly
echo "SPIR-V Specialization"
& $dPath\vk0.10-spirv_specialization
