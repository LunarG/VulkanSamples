## How to Contribute to Vulkan-LoaderAndValidationLayers

### **How to Submit Fixes**

* **Ensure that the bug was not already reported or fixed** by searching on GitHub under [Issues](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues)
  and
  [Pull Requests](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/pulls).
* Use the existing GitHub forking and pull request process.
  This will involve [forking the repository](https://help.github.com/articles/fork-a-repo/),
  creating a branch with your commits, and then [submitting a pull request](https://help.github.com/articles/using-pull-requests/).
* Please base your fixes on the master branch.  SDK branches are generally not updated except for critical fixes needed to repair an SDK release.
* Please include the GitHub Issue or Pull Request number near the beginning of the commit text.
    * Example: "GitHub PR 123: Fix missing init"


#### **Coding Conventions and Formatting**
* Try to follow any existing style in the file.  "When in Rome..."
* Run clang-format on your changes to maintain formatting.
    * There are `.clang-format files` throughout the repository to define clang-format settings
      which are found and used automatically by clang-format.
    * Note that there are some files that may not have been run through clang-format.
      These should be obvious from their appearance and the number of changes that clang-format would make.
      Don't format these files.
      In other words, please run clang-format on your changes where appropriate.
    * A sample git workflow may look like:

>        # Make changes to the source.
>        $ git add .
>        $ clang-format -i < list of changed code files >
>        # Check to see if clang-format made any changes and if they are OK.
>        $ git add .
>        $ git commit

#### **Testing**
* Run the existing tests in the repository before and after your changes to check for any regressions.
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
* Feel free to subject your code changes to other tests as well!

### **Contributor License Agreement (CLA)**

The Khronos Group is still finalizing the CLA process and documentation,
so the details about using or requiring a CLA are not available yet.
In the meantime, we suggest that you not submit any contributions unless you are comfortable doing so without a CLA.

### **Large or New Contributions**
 
All contributions made to the Vulkan-LoaderAndValidationLayers repository are Khronos branded and as such any new files need to have the Khronos license (MIT like) and copyright included. You can include your individual copyright after the Khronos copyright. See an existing file as an example.
