# Vulkan Samples Kit
  - This repository is a collection of Vulkan C++ sample applications.
  - Run the following script to obtain a short description of all or a 
    specific sample:
    `$ src/get-short-descripts.sh`
  - Run the following script to obtain a more detailed description of all
    samples with a long description set:
    `$ src/get-descripts.sh`

## Requirements/Dependencies
  - A recent version of the LunarG Vulkan SDK must be installed on the system.
    In addtion to the Vulkan headers and libraries, the SDK also includes the
    glslang components necessary to build the sample progams.
  - Linux package dependencies:
    - Vulkan SDK required packages
    - other - cmake
  - Windows dependencies include:
    - Vulkan SDK required components
    - other - cmake, cygwin

## Kit Structure
  - The Vulkan Samples Kit is a set of source and data files in a specific
    directory hierarchy:
      - data/ - static data files used in the samples, ie images, shaders, etc; 
        Vulkan-version-specific files will reside in directories named by the
        version (i.e. vk0.9)
      - ext/ - external third party components outside of the Vulkan SDK and
        samples framework (glm)
      - src/ - samples source code, where sample file name is prefixed by Vulkan
        version (i.e. vk0.9-instance.cpp)
      - utils/ - source code for common utilities used by the sample programs

## Building the Vulkan Samples Kit
- Linux:
  ```
  $ cmake -H. -Bbuild
  $ make -C build 
  ```

- Windows:
  ```
  $ cmake -G "Visual Studio 12 Win64" -H. -Bbuild
  ```
  Open the VULKAN_SAMPLES.sln file in the build folder with Microsoft Visual Studio and build the solution.
## Contributing
  Refer to the README.contrib file for specific info regarding contributing to
  the Vulkan samples creation effort.

