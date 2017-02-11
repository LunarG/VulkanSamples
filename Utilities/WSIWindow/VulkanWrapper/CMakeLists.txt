set(LIBRARY_NAME VULKAN_WRAPPER)

file(GLOB SRC_LIST "*.h" "*.cpp")                       # List all source files from current directory
add_library (${LIBRARY_NAME} STATIC ${SRC_LIST})        # and add them to the WSIWindow library.

target_include_directories (${LIBRARY_NAME} PUBLIC .)
target_include_directories (${LIBRARY_NAME} PUBLIC "${VULKAN_INCLUDE}") # include vulkan.h

target_link_libraries (${LIBRARY_NAME} ${CMAKE_DL_LIBS}) # for dlsym
