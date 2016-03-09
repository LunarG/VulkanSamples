## How to Contribute to Vulkan Source Repositories

### **The Repositories**

The Vulkan source code is distributed across several GitHub repositories.
The repositories sponsored by Khronos and LunarG are described here.
In general, the canonical Vulkan Loader and Validation Layers sources are in the Khronos repository,
while the LunarG repositories host sources for additional tools and sample programs.

* [Khronos Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
* [LunarG VulkanTools](https://github.com/LunarG/VulkanTools)
* [LunarG VulkanSamples](https://github.com/LunarG/VulkanSamples)

As a convenience, the contents of the Vulkan-LoaderAndValidationLayers repository are downstreamed into the VulkanTools and VulkanSamples repositories via a branch named `trunk`.
This makes the VulkanTools and VulkanSamples easier to work with and avoids compatibility issues 
that might arise with Vulkan-LoaderAndValidationLayers components if they were obtained from a separate repository.

### **How to Submit Fixes**

* **Ensure that the bug was not already reported or fixed** by searching on GitHub under Issues
  and Pull Requests.
* Use the existing GitHub forking and pull request process.
  This will involve [forking the repository](https://help.github.com/articles/fork-a-repo/),
  creating a branch with your commits, and then [submitting a pull request](https://help.github.com/articles/using-pull-requests/).
* Please base your fixes on the master branch.  SDK branches are generally not updated except for critical fixes needed to repair an SDK release.
* Please include the GitHub Issue number near the beginning of the commit text if applicable.
    * Example: "GitHub 123: Fix missing init"
* If your changes are restricted only to files from the Vulkan-LoaderAndValidationLayers repository, please direct your pull request to that repository, instead of VulkanTools or VulkanSamples.


#### **Coding Conventions and Formatting**
* Try to follow any existing style in the file.  "When in Rome..."
* Run clang-format on your changes to maintain formatting.
    * There are `.clang-format files` throughout the repository to define clang-format settings
      which are found and used automatically by clang-format.
    * A sample git workflow may look like:

>        # Make changes to the source.
>        $ git add .
>        $ clang-format -style=file -i < list of changed code files >
>        # Check to see if clang-format made any changes and if they are OK.
>        $ git add .
>        $ git commit

#### **Testing**
* Run the existing tests in the repository before and after your changes to check for any regressions.  
  There are some tests that appear in all repositories.
  These tests can be found in the following folders inside of your target build directory:
  (These instructions are for Linux)
* In the `demos` directory, run:

>        cube
>        cube --validate
>        tri
>        tri --validate
>        smoke
>        smoke --validate
>        vulkaninfo

* In the `tests` directory, run:

>        run_all_tests.sh

* Note that some tests may fail with known issues or driver-specific problems.
  The idea here is that your changes shouldn't change the test results, unless that was the intent of your changes.
* Run tests that explicitly exercise your changes.
* Feel free to subject your code changes to other tests as well!

### **Contributor License Agreement (CLA)**

#### **Khronos Repository (Vulkan-LoaderAndValidationLayers)**

The Khronos Group is still finalizing the CLA process and documentation,
so the details about using or requiring a CLA are not available yet.
In the meantime, we suggest that you not submit any contributions unless you are comfortable doing so without a CLA.

#### **LunarG Repositories**

You'll be prompted with a "click-through" CLA as part of submitting your pull request in GitHub.

### **License and Copyrights**

All contributions made to the Vulkan-LoaderAndValidationLayers repository are Khronos branded and as such,
any new files need to have the Khronos license (MIT style) and copyright included.
Please see an existing file in this repository for an example.

All contributions made to the LunarG repositories are to be made under the MIT license
and any new files need to include this license and any applicable copyrights.

You can include your individual copyright after any existing copyrights.
