# Be sure to run "Set-ExecutionPolicy RemoteSigned" before running powershell scripts

Param([switch]$Debug)

if ($Debug) {
    $dPath = "Debug"
    } else {
    $dPath = "Release"
}

trap
{
    Write-Error $_
    exit 1
}

function Exec
{
    param ($Command)
    & $dPath\$Command
    if ($LastExitCode -ne 0) {
        throw "Error running sample"
    }
}

echo "Instance"
Exec {instance}
echo "Instance Layer Properties"
Exec {instance_layer_properties}
echo "Instance Layer Properties"
Exec {instance_layer_properties}
echo "Instance Extension Properties"
Exec {instance_extension_properties}
echo "Instance Layer ExtensionProperties"
Exec {instance_layer_extension_properties}
echo "Enumerate"
Exec {enumerate}
echo "Device"
Exec {device}
echo "Enable Validation and Debug Message Callback"
Exec {enable_validation_with_callback}
echo "Create Debug Message Callback"
Exec {dbgcreatemsgcallback}
echo "Init Swapchain"
Exec {initswapchain}
echo "Init Command Buffer"
Exec {initcommandbuffer}
echo "Depth Buffer"
Exec {depthbuffer}
echo "Init Texture"
Exec {inittexture}
echo "Uniform Buffer"
Exec {uniformbuffer}
echo "Init RenderPass"
Exec {initrenderpass}
echo "Init Framebuffer"
Exec {initframebuffers}
echo "Descriptor and Pipeline Layouts"
Exec {descriptor_pipeline_layouts}
echo "Vertex Buffer"
Exec {vertexbuffer}
echo "Alloc Descriptor Sets"
Exec {allocdescriptorsets}
echo "Initialize Shaders"
Exec {initshaders}
echo "Initialize Pipeline"
Exec {initpipeline}
echo "Draw Cube"
Exec {drawcube}
echo "Copy/Blit Image"
Exec {copyblitimage}
echo "Draw Textured Cube"
Exec {drawtexturedcube}
echo "Draw Cubes with Dynamic Uniform Buffer"
Exec {dynamicuniform}
echo "Texel Buffer"
Exec {texelbuffer}
echo "Immutable Sampler"
Exec {immutable_sampler}
echo "Memory Barriers"
Exec {membarriers}
echo "Multiple Descriptor Sets"
Exec {multiple_sets}
echo "Multithreaded Command Buffers"
Exec {multithreadcmdbuf}
echo "Push Constants"
Exec {push_constants}
echo "Separate image sampler"
Exec {separate_image_sampler}
echo "Draw Sub-passes"
Exec {drawsubpasses}
echo "Occlusion Query"
Exec {occlusion_query}
echo "Pipeline Cache"
Exec {pipeline_cache}
echo "Pipeline Derivative"
Exec {pipeline_derivative}
echo "Secondary Command Buffers"
Exec {secondarycmd}
echo "SPIR-V Assembly"
Exec {spirv_assembly}
echo "SPIR-V Specialization"
Exec {spirv_specialization}
