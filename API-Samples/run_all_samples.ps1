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

echo "Initialize Instance"
Exec {init_instance}
echo "Instance Layer Properties"
Exec {instance_layer_properties}
echo "Instance Layer Properties"
Exec {instance_layer_properties}
echo "Instance Extension Properties"
Exec {instance_extension_properties}
echo "Instance Layer ExtensionProperties"
Exec {instance_layer_extension_properties}
echo "Enumerate Devices"
Exec {enumerate_devices}
echo "Initialize Device"
Exec {init_device}
echo "Enable Validation and Debug Message Callback"
Exec {enable_validation_with_callback}
echo "Create Debug Message Callback"
Exec {dbg_create_msg_callback}
echo "Initialize Swapchain"
Exec {init_swapchain}
echo "Initialize Command Buffer"
Exec {init_command_buffer}
echo "Initialize Depth Buffer"
Exec {init_depth_buffer}
echo "Initialize Texture"
Exec {init_texture}
echo "Initialize Uniform Buffer"
Exec {init_uniform_buffer}
echo "Initialize Render Pass"
Exec {init_render_pass}
echo "Initialize Frame Buffers"
Exec {init_frame_buffers}
echo "Initialize Pipeline Layout"
Exec {init_pipeline_layout}
echo "Initialize Vertex Buffer"
Exec {init_vertex_buffer}
echo "Initialize Descriptor Set"
Exec {init_descriptor_set}
echo "Initialize Shaders"
Exec {init_shaders}
echo "Initialize Pipeline"
Exec {init_pipeline}
echo "Draw Cube"
Exec {draw_cube}
echo "Copy/Blit Image"
Exec {copy_blit_image}
echo "Draw Textured Cube"
Exec {draw_textured_cube}
echo "Draw Cubes with Dynamic Uniform Buffer"
Exec {dynamic_uniform}
echo "Texel Buffer"
Exec {texel_buffer}
echo "Immutable Sampler"
Exec {immutable_sampler}
echo "Memory Barriers"
Exec {memory_barriers}
echo "Multiple Descriptor Sets"
Exec {multiple_sets}
echo "Multithreaded Command Buffers"
Exec {multithreaded_command_buffer}
echo "Push Constants"
Exec {push_constants}
echo "Separate image sampler"
Exec {separate_image_sampler}
echo "Draw Sub-passes"
Exec {draw_subpasses}
echo "Occlusion Query"
Exec {occlusion_query}
echo "Pipeline Cache"
Exec {pipeline_cache}
echo "Pipeline Derivative"
Exec {pipeline_derivative}
echo "Secondary Command Buffers"
Exec {secondary_command_buffer}
echo "SPIR-V Assembly"
Exec {spirv_assembly}
echo "SPIR-V Specialization"
Exec {spirv_specialization}
