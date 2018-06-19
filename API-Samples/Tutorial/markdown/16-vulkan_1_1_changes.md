# Vulkan 1.1 Changes

<link href="../css/lg_stylesheet.css" rel="stylesheet"></link>

Now that Vulkan 1.1 has been released, you may desire to
take advantage of the new functionality.  However, there
are some changes to the VkInstance creation process that
you must perform in order to properly access all of the
new functionality.

Primary code file for this section is `16-vulkan_1_1.cpp`

## Determining Vulkan Loader Support

The main change to the VkInstance creation step, starting
with Vulkan 1.1, is the new `vkEnumerateInstanceVersion`
function call, which has the following prototype:

    VkResult vkEnumerateInstanceVersion(
        uint32_t*    pApiVersion);

All Vulkan 1.1 loaders will natively export this command.
However, if you write a Vulkan 1.1 application and someone
attempts to use that application with a Vulkan 1.0 loader,
then the application will fail to run.  For more information
on handling this in a more versatile fashion, see the section
on [Making Your Application More Versatile](#making-your-application-more-versatile).

The `pApiVersion` returned by the new `vkEnumerateInstanceVersion`
command indicates the most recent API version of Vulkan that
the Vulkan runtime/loader on your system supports.  It is
formatted in a similar way to wall the other standard Vulkan
API versions defined using the VK\_MAKE\_VERSION macro and
resulting in a bit-shifted unsigned integeger containing the
major, minor, and patch version of the API.  For those
unfamiliar with the VK\_MAKE\_VERSION macro, it does the
following:

    #define VK_MAKE_VERSION(major, minor, patch) \
        (((major) << 22) | ((minor) << 12) | (patch))

You could easily write your own macros/functions to extract
this same information from a returned value, but Khronos
provides the following additional macros for your use:

    #define VK_VERSION_MAJOR(version) ((uint32_t)(version) >> 22)
    #define VK_VERSION_MINOR(version) (((uint32_t)(version) >> 12) & 0x3ff)
    #define VK_VERSION_PATCH(version) ((uint32_t)(version) & 0xfff)

Using these provided macros, you can easily extract the
Vulkan major and minor API versions out of the returned
uint32_t as the following code snippet shows:

    uint32_t api_version= 0;
    vkEnumerateInstanceVersion(&api_version);
    uint16_t api_major_version = VK_VERSION_MAJOR(api_version);
    uint16_t api_minor_version = VK_VERSION_MINOR(api_version);

Then, to determine if you can execute a particular version,
say, Vulkan 1.1, you perform a simple check:

    if (api_major_version > 1 || api_minor_version >= 1) {
        // 1.1 is available
    }

If the check passes, you can then continue from here and create
a Vulkan 1.1 Instance.  If this fails, your only option is
creating a Vulkan 1.0 Instance.


## Letting Vulkan Know What Version You Are Using

When you've determine that you can create a Vulkan Instance
for a particular version of the API, you should what that
desired API version is to the Vulkan components.  This
is not required, but certain behaviors may change from one
minor version of Vulkan to another.  By providing clear
information about the API version you intend to use, your
application will get the behavior it expects.  Also,
validation layers will use this information to inform you
of cases where your application may be using something
incorrectly based on the API version you requested.

To inform Vulkan of what version of the API you intend
to use, simply provide your desired API version in the
`VkApplicationInfo` structure during
[Instance Creation](01-init_instance.html).

As a brief recap, the `VkApplicationInfo` structure
contains the following information:

    struct VkApplicationInfo {
        VkStructureType    sType;
        const void*        pNext;
        const char*        pApplicationName;
        uint32_t           applicationVersion;
        const char*        pEngineName;
        uint32_t           engineVersion;
        uint32_t           apiVersion;
    };

To fill in the API version your application intends
to use, simply update `apiVersion` using the
VK\_MAKE\_VERSION macro to indicate the correct
version.  For example, to indicate you're using
Vulkan 1.1, you would do the following:

    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = NULL;
    application_info.apiVersion = VK_MAKE_VERSION(1, 1, 0);
    ...

Make sure that you pass the pointer to your `VkApplicationinfo`
structure to the `pApplicationInfo` member of your
`VkInstanceCreateInfo` structure that gets pass into
`vkCreateInstance`, and you're set.

One other change that may affect you is that `vkCreateInstance`
no longer returns the  `VK_ERROR_INCOMPATIBLE_DRIVER` error if
you pass in an incorrect version.  This is because the
`apiVersion` field of the `VkApplicationinfo` structure is now
considered informational only.  This means that the following
code from the first sample in this Tutorial is no longer
valid if you create an instance in Vulkan 1.1:

    res = vkCreateInstance(&inst_info, NULL, &inst);
    if (res == VK_ERROR_INCOMPATIBLE_DRIVER) {
        std::cout << "cannot find a compatible Vulkan ICD\n";
        exit(-1);
    } else if (res) {
        std::cout << "unknown error\n";
        exit(-1);
    }

Now perform only the "else if" portion of the check:

    res = vkCreateInstance(&inst_info, NULL, &inst);
    if (res) {
        std::cout << "unknown error\n";
        exit(-1);
    }


## Selecting a Vulkan 1.1 Device

Once you've created a Vulkan 1.1 Instance, you need
to determine if any of the physical devices support
1.1 as well.  The first step is to gather a list of
Vulkan physical devices available on your system,
just like you did for
[Step 2 Enumerate Device](02-enumerate_devices.html).

When you have your list of available physical devices,
you need to determine what version of the Vulkan API
each device supports using `vkGetPhysicalDeviceProperties`:

    void vkGetPhysicalDeviceProperties(
        VkPhysicalDevice               physicalDevice,
        VkPhysicalDeviceProperties*    pProperties);

This command returns physical device information in the
`VkPhysicalDeviceProperties` structure.  Which includes
the following information:

    struct VkPhysicalDeviceProperties {
        uint32_t                            apiVersion;
        uint32_t                            driverVersion;
        uint32_t                            vendorID;
        uint32_t                            deviceID;
        VkPhysicalDeviceType                deviceType;
        char                                deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
        uint8_t                             pipelineCacheUUID[VK_UUID_SIZE];
        VkPhysicalDeviceLimits              limits;
        VkPhysicalDeviceSparseProperties    sparseProperties;
    };

For each physical device returned by
`vkEnumeratePhysicalDevices` you should make the corresponding
call to `vkGetPhysicalDeviceProperties` to get the properties.
Then, check the `apiVersion` member to determine the API
version that the driver associated with that physical device
supports.

Once you've looped through all your devices, use the
physical device with the API closest to the one you desire.
If none supports the desired version, you can only use
functionality included in the version of the physical device that
you do end up selecting.


## Making Your Application More Versatile

As mentioned in [Determining Vulkan Loader Support](#determining-vulkan-loader-support),
if you use the new API commands that are directly exported by the
Vulkan loader/runtime, you run into the possibility of a user
encountering issues when they try to use your application.  The
main issue that typically arises is if a user has an older
loader which doesn't export the new Vulkan 1.1 commands.
This scenario would result in your application failing to start
on their system due to missing symbols.  This may not be a bad
thing because it can be used to force your users to upgrade
their runtime and/or graphics drivers.

However, if you want your application to gracefully handle
the lack of a Vulkan 1.1-capable loader, you should query
if the command exists and then use function pointers for
all the new functionality.  This is what the
`vkGetInstanceProcAddr` and `vkGetDeviceProcAddr` commands
are provided for.

The first command you want to query existence of is the
new `vkEnumerateInstanceVersion` command.  Since this command
has to be called prior to the creation of an Instance, you can
pass in VK\_NULL\_HANDLE for the `vkGetInstanceProcAddr` call:

    PFN_vkEnumerateInstanceVersion my_EnumerateInstanceVersion =
        (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(
            VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
    if (nullptr != my_EnumerateInstanceVersion) {
        // Command is present and defined.
    } else {
        // Command is not present, treat as Vulkan 1.0 only
    }

For each new instance command (commands that take either a VkInstance
or a VkPhysicalDevice) you should query a similar function pointer
using `vkGetInstanceProcAddr`.  For all other commands, you should
query a pointer using the created logical device and `vkGetDeviceProcAddr`.
The "cube" demo provided with the Vulkan SDK provides several
examples of using `vkGetInstanceProcAddr` and `vkGetDeviceProcAddr`
should you need an example.

Using the returned function pointers will allow you to check at
runtime for the presence of a Vulkan 1.1-capable system, and the fall
back gracefully if functionality isn't present.  This does mean
that your application would have to support at least two different
versions of the API (1.0 and whatever version you would prefer).

The code for this modified sample showing the more flexible way
to query the Vulkan API version can be found in `vulkan_1_1_flexible.cpp`

That's all, we hope you enjoy working on Vulkan!

<table border="1" width="100%">
    <tr>
        <td align="center" width="33%">Previous: <a href="15-draw_cube.html" title="Prev">Draw Cube</a></td>
        <td align="center" width="33%"><a href="index.html" title="Index">Index</a></td>
    </tr>
</table>
<footer>&copy; Copyright 2017 LunarG, Inc</footer>
