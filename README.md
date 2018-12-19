# Vulkan Samples
  - This repository is a collection of Vulkan C++ sample applications.
  - Run the following script to obtain a short description of all or a 
    specific sample:
    `$ API-Samples/get-short-descripts.sh`
  - Run the following script to obtain a more detailed description of all
    samples with a long description set:
    `$ API-Samples/get-descripts.sh`

## CI Build Status
| Platform | Build Status |
|:--------:|:------------:|
| Linux/Android | [![Build Status](https://travis-ci.org/LunarG/VulkanSamples.svg?branch=master)](https://travis-ci.org/LunarG/VulkanSamples) |
| Windows | [![Build status](https://ci.appveyor.com/api/projects/status/c5l2y9nk7wve9xvu/branch/master?svg=true)](https://ci.appveyor.com/project/karl-lunarg/vulkansamples/branch/master) |

## Structure

Vulkan Samples
 - The Vulkan Samples repo is a set of source and data files in a specific
    directory hierarchy:
      - API-Samples - Samples that demonstrate the use of various aspects of the
        Vulkan API
      - Vulkan Tutorial - Steps you through the process of creating a simple Vulkan application, learning the basics along the way. This [Vulkan Tutorial link](https://vulkan.lunarg.com/doc/sdk/latest/windows/tutorial/html/index.html) allows you to view the Vulkan Tutorial on LunarXchange as well. 
      - Sample-Programs - Samples that are more functional and go deeper than simple API use.
      - Layer-Samples - Samples that are implemented as layers.  The Overlay layer sample is deprecated and does not build.
	  
## Sample progression
  - In general, the samples are not interrelated, but there is a progression
      among some of the samples that lead to drawing a cube.  Start with the
      instance sample, then enumerate-adv, device, initcommandbuffer, initswapchain, initdepthbuffer,
      inituniformbuffer, descriptor_pipeline_layouts, initrenderpass, initshaders,
      initframebuffers, vertexbuffer, allocdescriptorsets, initpipeline, and they
      culminate in the drawcube sample.  Each sample uses utility routines from
      the code from previous samples to get to the point to show something new.
      The drawtexturedcube sample takes all of the drawcube code and adds texturing.

## Contributing
  Refer to the README.contrib file for specific info regarding contributing to
  the Vulkan samples creation effort.

## Contact Information
* [Tony Barbour](mailto:tony@lunarg.com)
* [Mark Lobodzinski](mailto:mark@lunarg.com)

## Information for Developing or Contributing:

Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file in this repository for more details.

## How to Build and Run

[BUILD.md](BUILD.md)
Includes directions for building all components as well as running validation tests and demo applications.

## Version Tagging Scheme

Updates to the `LunarG-VulkanSamples` repository which correspond to a new Vulkan specification release are tagged using the following format: `v<`_`version`_`>` (e.g., `v1.1.96`).

**Note**: Marked version releases have undergone thorough testing but do not imply the same quality level as SDK tags. SDK tags follow the `sdk-<`_`version`_`>.<`_`patch`_`>` format (e.g., `sdk-1.1.92.0`).

This scheme was adopted following the 1.1.96 Vulkan specification release.

## License
This work is released as open source under a Apache-style license.  See LICENSE.txt for full license.

See COPYRIGHT.txt for a full list of licenses used in this repository.

## Acknowledgements
While this project has been developed primarily by LunarG, Inc., there are many other
companies and individuals making this possible: Valve Corporation, funding
project development; Google providing significant contributions to the samples.
