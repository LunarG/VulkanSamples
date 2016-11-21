set(LIBRARY_NAME VULKAN_WRAPPER)

file(GLOB SRC_LIST "*.h" "*.cpp")                       # List all source files from current directory
add_library (${LIBRARY_NAME} STATIC ${SRC_LIST})        # and add them to the WSIWindow library.
target_include_directories (${LIBRARY_NAME} PUBLIC .)

#=======================Find Vulkan Header========================
find_path(VULKAN_INCLUDE vulkan/vulkan.h DOC "Path to Vulkan header file. (vulkan.h)")
find_path(VULKAN_INCLUDE vulkan/vulkan.h PATH "$ENV{VULKAN_SDK}/include")    # Search in Vulkan SDK
find_path(VULKAN_INCLUDE vulkan/vulkan.h PATH "../../../../include")         # Search in VulkanSamples
target_include_directories(${LIBRARY_NAME} PUBLIC "${VULKAN_INCLUDE}")       # include vulkan.h
message("VULKAN_INCLUDE path: ${VULKAN_INCLUDE}")
#=================================================================
