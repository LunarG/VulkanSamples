# Sample BIL to Intel ISA Compiler

This compiler stack was brought over from the GlassyMesa driver LunarG created for Valve.
It uses the following tools:
- BIL support and LunarGLASS middle end optimizer (pulled in via 
[update_external_sources.sh](../../../update_external_sources.sh) script)
(mesa-utils/src/glsl)
- [GlassyMesa's GLSLIR and supporting infrastructure](shader)
- [GlassyMesa's DRI i965 backend](pipeline)

For vkCreateShader, we primarily used the existing standalone device independent front end which can consume GLSL or BIL, and results in a separately linked shader object.

For vkCreateGraphicsPipeline, we pulled over only the files needed to lower the shader object to ISA and supporting metadata.  Much of the i965 DRI driver was removed or commented out for future use, and is still being actively bootstrapped.

Currently only Vertex and Fragment shaders are supported.  Any shader that fits within the IO parameters you see tested in compiler_render_tests.cpp should work.  Buffers with bindings, samplers with bindings, interstage IO with locations, are all working.  Vertex input locations work if they are sequential and start from 0.  Fragment output locations only work for location 0.

We recommend using only buffers with bindings for uniforms, no global, non-block uniforms.

Design decisions we made to get this stack working with current specified VK and BIL.  We know these are active areas of discussion, and we'll update when decisions are made:
- Samplers:
  - GLSL sampler bindings equate to a sampler/texture pair of the same number, as set up by the VK application.  i.e. the following sampler:
```
    layout (binding = 2) uniform sampler2D surface;
```
will read from VK_SLOT_SHADER_SAMPLER entity 2 and VK_SLOT_SHADER_RESOURCE entity 2.

- Buffers:
  - GLSL buffer bindings equate to the buffer bound at the same slot. i.e. the following uniform buffer:
```
    layout (std140, binding = 2) uniform foo { vec4 bar; } myBuffer;
```
will be read from VK_SHADER_RESOURCE entity 2.
