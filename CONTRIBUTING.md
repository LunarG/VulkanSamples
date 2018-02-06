## How to Contribute to Vulkan Source Repositories

### **The Repository**

The source code for The Vulkan-LoaderAndValidationLayer components is sponsored by Khronos and LunarG.
* [Khronos Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)


### **The Vulkan Ecosystem Needs Your Help**

The Vulkan validation layers are one of the larger and more important components in this repository.
While there are often active and organized development efforts underway to improve their coverage,
there are always opportunities for anyone to help by contributing additional validation layer checks
and tests for these validation checks.

There are a couple of methods to identify areas of need:
* Examine the [issues list](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues)
in this repository and look for issues that are of interest
* Alternatively, examine the [vk_validation_error_database.txt](layers/vk_validation_error_database.txt) file -- unimplemented validation checks are marked
with an 'N' in the 'check_implemented' column and each of these needs coverage in the validation layers.

Of course, if you have your own work in mind, please open an issue to describe it and assign it to yourself.
Finally, please feel free to contact any of the developers that are actively contributing should you
wish to coordinate further.
Please see the [section about Validation Layers](#special-considerations-for-validation-layers)
later on this page.

Repository Issue labels:

* _Bug_:          These issues refer to invalid or broken functionality and are the highest priority.
* _Incomplete_:   These issues refer to missing validation checks that users have encountered during application
development that would have been directly useful, and are high priority.
* _Enhancement_:  These issues refer to ideas for extending or improving the loader, demos, or validation layers.

It is the maintainers goal for all issues to be assigned within one business day of their submission. If you choose
to work on an issue that is assigned, simply coordinate with the current assignee.

### **How to Submit Fixes**

* **Ensure that the bug was not already reported or fixed** by searching on GitHub under Issues
  and Pull Requests.
* Use the existing GitHub forking and pull request process.
  This will involve [forking the repository](https://help.github.com/articles/fork-a-repo/),
  creating a branch with your commits, and then [submitting a pull request](https://help.github.com/articles/using-pull-requests/).
* Please read and adhere to the style and process [guidelines ](#coding-conventions-and-formatting) enumerated below.
* Please base your fixes on the master branch.  SDK branches are generally not updated except for critical fixes needed to repair an SDK release.


#### **Coding Conventions and Formatting**
* Use the **[Google style guide](https://google.github.io/styleguide/cppguide.html)** for source code with the following exceptions:
    * The column limit is 132 (as opposed to the default value 80). The clang-format tool will handle this. See below.
    * The indent is 4 spaces instead of the default 2 spaces. Again, the clang-format tool will handle this.
    * If you can justify a reason for violating a rule in the guidelines, then you are free to do so. Be prepared to defend your
decision during code review. This should be used responsibly. An example of a bad reason is "I don't like that rule." An example of
a good reason is "This violates the style guide, but it improves type safety."

* Run **clang-format** on your changes to maintain consistent formatting
    * There are `.clang-format files` present in the repository to define clang-format settings
      which are found and used automatically by clang-format.
    * A sample git workflow may look like:

>        # Make changes to the source.
>        $ git add -u .
>        $ git clang-format --style=file
>        # Check to see if clang-format made any changes and if they are OK.
>        $ git add -u .
>        $ git commit

* **Commit Messages**
    * Limit the subject line to 50 characters -- this allows the information to display correctly in git/Github logs
    * Begin subject line with a one-word component description followed by a colon (e.g. loader, layers, tests, etc.)
    * Separate subject from body with a blank line
    * Wrap the body at 72 characters
    * Capitalize the subject line
    * Do not end the subject line with a period
    * Use the body to explain what and why vs. how
    * Use the imperative mode in the subject line. This just means to write it as a command (e.g. Fix the sprocket)

Strive for commits that implement a single or related set of functionality, using as many commits as is necessary (more is better).
That said, please ensure that the repository compiles and passes tests without error for each commit in your pull request.  Note
that to be accepted into the repository, the pull request must [pass all tests](#testing your changes) on all supported platforms
-- the automatic Github Travis and AppVeyor continuous integration features will assist in enforcing this requirement.

#### **Testing Your Changes**
* Run the existing tests in the repository before and after each if your commits to check for any regressions.
  There are some tests that appear in all repositories.
  These tests can be found in the following folders inside of your target build directory:

  (These instructions are for Linux)
* In the `demos` directory, run:

>        cube
>        cube --validate
>        smoke
>        smoke --validate
>        vulkaninfo

* In the `tests` directory, run:

>        run_all_tests.sh

* On Windows, a quick sanity check can be run from inside Visual Studio -- just run the `vk_layer_validation_tests` project,
or you can run `run_all_tests.ps1` from a PowerShell window

* Note that some tests may fail with known issues or driver-specific problems.
  The idea here is that your changes should not change the test results, unless that was the intent of your changes.
* Run tests that explicitly exercise your changes.
* Feel free to subject your code changes to other tests as well!


#### **Special Considerations for Validation Layers**
* **Validation Tests**  If you are submitting a change that adds a new validation check, you should also construct a "negative" test function.
The negative test function purposely violates the validation rule that the new validation check is looking for.
The test should cause your new validation check to identify the violation and issue a validation error report.
And finally, the test should check that the validation error report is generated and consider the test as "passing"
if the report is received.  Otherwise, the test should indicate "failure".
This new test should be added to the validation layer test program in the `tests` directory and contributed
at the same time as the new validation check itself, along with appropriate updates to `layers\vk_validation_error_database.txt`.
There are many existing validation tests in this directory that can be used as a starting point.
* **Validation Checks**  The majority of validation checks are carried out by the Core Validation layer. In general, this layer
contains checks that require some amount of application state to carry out. In contrast, the parameter validation layer contains
checks that require (mostly) no state at all. Please inquire if you are unsure of the location for your contribution. The other
layers (threading, object_tracker, unique_objects) are more special-purpose and are mostly code-generated from the specification.

### **Contributor License Agreement (CLA)**

You will be prompted with a one-time "click-through" CLA dialog as part of submitting your pull request
or other contribution to GitHub.

### **License and Copyrights**

All contributions made to the Vulkan-LoaderAndValidationLayers repository are Khronos branded and as such,
any new files need to have the Khronos license (Apache 2.0 style) and copyright included.
Please see an existing file in this repository for an example.

All contributions made to the LunarG repositories are to be made under the Apache 2.0 license
and any new files need to include this license and any applicable copyrights.

You can include your individual copyright after any existing copyrights.
