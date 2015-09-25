# Build Targets
The build target (i.e. program name) for all samples is the base source
filename of the sample without the .cpp suffix.  (If it will never be the
case that we could have more than one version with same base file name, then
would not need the prefix in the compiled program.)

All sample programs are linked with the Vulkan loader and samples utility
library.  Windows samples are linked with XX libraries as well.

The Vulkan Samples Kit currently supports the following types of build targets:
  - single file sample, no shaders
  - single file sample, spirv shaders
  - single file sample, glsl shaders (convert to spirv)

CMake functions are provided for each of the above types of build targets.
Certain variables must be set before invoking the functions, one of which is
VULKAN_VERSION.  This variable must be set to the version prefix used in both
the file name and data subdirectory name.

## Single File, No Shaders
The `sampleWithSingleFile` function will build all samples identified in the
S_TARGETS variable.

To add a single file sample to the build, include the base name (no prefix or
.cpp) of the file in the `S_TARGETS` list.

```
set(VULKAN_VERSION vk0.9)
set (S_TARGETS instance device)
sampleWithSingleFile()
```

## Single File, SPIRV Shaders
Samples with dependencies on SPIRV shaders are built with the
`sampleWithSPIRVShaders` function.

To add a sample with SPIRV shader dependencies to the build, identify the list
of SPIRV shader file names (no .spv suffix) to the SAMPLE_SPIRV_SHADERS
variable.

```
set(VULKAN_VERSION vk0.9)
set(SAMPLE_SPIRV_SHADERS spirvshader-vert spirvshader-frag)
sampleWithSPIRVShaders(usespirvshader)
```

## Single File, GLSL Shaders
Samples with dependencies on GLSL shaders that must be converted to SPIRV are
built with the `sampleWithGLSLShaders` function.  This function will convert
the GLSL vertex and fragment shaders to the SPIRV equivalents that are used
in the sample program.

To add a sample with GLSL shader dependencies to the build, identify the list
of GLSL shader file names (no suffixes) of the specific type to the
appropriate SAMPLE_GLSL_VERT_SHADERS and SAMPLE_GLSL_FRAG_SHADERS variables.

```
set(VULKAN_VERSION vk0.9)
set(SAMPLE_GLSL_FRAG_SHADERS glslshader)
set(SAMPLE_GLSL_VERT_SHADERS glslshader)
sampleWithGLSLShaders(useglslshader)
```

In the example above, the files glslshader.vert and glslshader.frag reside in
$VULKAN_SAMPLES/data/vk0.9.  These GLSL shaders are converted to SPIRV via
glslangValidator, with the resulting files named glslshader-frag.spv and
glslshader-vert.spv, residing in the same directory as the originals.

## TODO

- flesh out the build framework for Windows
- samples with more than one source file?
- other variations and types of shaders? (TBD)

