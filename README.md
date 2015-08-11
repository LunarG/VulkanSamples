# Vulkan Samples Kit
  - This repository is a collection of Vulkan sample applications.
  - Run the following script to obtain a short description of all samples:
    `$ src/get-short-descripts.sh`
  - Run the following script to obtain a more detailed description of all or
    specific samples:
    `$ src/get-descripts.sh`

## Requirements/Dependencies
  - A recent version of the LunarG Vulkan SDK must be installed on the system.
  - Linux package dependencies:
    - Vulkan SDK required packages
    - other - cmake ??
  - Windows dependencies include:
    - cmake ??

## Kit Structure
  - The Vulkan Samples Kit is a set of source and data files in a specific
    directory hierarchy:
      - data/ - static data files used in the samples, ie images, shaders, etc; 
        Vulkan-version-specific files will reside in directories named by the
        version (i.e. vk0.2)
      - ext/ - external third party components outside of the Vulkan SDK and
        samples framework (TBD)
      - src/ - samples source code, where sample file name is prefixed by Vulkan
        version (i.e. vk0.2-instance.cpp)
      - utils/ - source code for common utilities used by the sample programs

## Building the Vulkan Samples Kit
  ```
  $ cmake -H. -Bbuild
  $ make -C build 
  ```

## Contributing
  Refer to the README.contrib file for specific info regarding contributing to
  the Vulkan samples creation effort.

