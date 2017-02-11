include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    set(DisplayServer Win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
       add_definitions(-DVK_USE_PLATFORM_XLIB_KHR -DVK_USE_PLATFORM_XLIB_XRANDR_EXT)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
       add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif()

    if (BUILD_WSI_MIR_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
        include_directories(${MIR_INCLUDE_DIR})
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

if (WIN32)
	FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-${MAJOR}.def DEF_FILE)
	FILE(READ ${DEF_FILE} DEF_TEXT)
	STRING(REGEX REPLACE "vulkan-${MAJOR}.dll" "${API_LOWERCASE}-${MAJOR}.dll" DEF_EDITED "${DEF_TEXT}" )
	FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${API_LOWERCASE}-${MAJOR}.def" "${DEF_EDITED}")
endif()

# DEBUG enables runtime loader ICD verification
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(NORMAL_LOADER_SRCS
    loader.c
    loader.h
    vk_loader_platform.h
    vk_loader_layer.h
    trampoline.c
    wsi.c
    wsi.h
    extensions.c
    extensions.h
    debug_report.c
    debug_report.h
    table_ops.h
    gpa_helper.h
    cJSON.c
    cJSON.h
    murmurhash.c
    murmurhash.h
)

set (OPT_LOADER_SRCS
    dev_ext_trampoline.c
    phys_dev_ext.c
)

set (LOADER_SRCS ${NORMAL_LOADER_SRCS} ${OPT_LOADER_SRCS})
add_definitions(-DAPI_NAME="${API_NAME}")

if (WIN32)
    # Use static MSVCRT libraries
    foreach(configuration in CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
                             CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${configuration} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${configuration} "${${configuration}}")
        endif()
    endforeach()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    # Build dev_ext_trampoline.c with -O2 to allow tail-call optimization.
    # Build other C files with normal options
    # setup two Cmake targets (loader-norm and loader-opt) for the different compilation flags
    separate_arguments(LOCAL_C_FLAGS_DBG WINDOWS_COMMAND ${CMAKE_C_FLAGS_DEBUG})
    set(CMAKE_C_FLAGS_DEBUG " ")
    separate_arguments(LOCAL_C_FLAGS_REL WINDOWS_COMMAND ${CMAKE_C_FLAGS_RELEASE})

    add_library(loader-norm OBJECT ${NORMAL_LOADER_SRCS} dirent_on_windows.c)
    target_compile_options(loader-norm PUBLIC "$<$<CONFIG:DEBUG>:${LOCAL_C_FLAGS_DBG}>")
    add_library(loader-opt OBJECT ${OPT_LOADER_SRCS})
    target_compile_options(loader-opt PUBLIC "$<$<CONFIG:DEBUG>:${LOCAL_C_FLAGS_REL}>")
    add_library(${API_LOWERCASE}-${MAJOR} SHARED $<TARGET_OBJECTS:loader-opt> $<TARGET_OBJECTS:loader-norm> ${CMAKE_CURRENT_BINARY_DIR}/${API_LOWERCASE}-${MAJOR}.def ${CMAKE_CURRENT_SOURCE_DIR}/loader.rc)
    add_library(VKstatic.${MAJOR} STATIC $<TARGET_OBJECTS:loader-opt> $<TARGET_OBJECTS:loader-norm>)
    # Suppress conflicting libs warning for debug builds.
    set_target_properties(${API_LOWERCASE}-${MAJOR} PROPERTIES LINK_FLAGS_DEBUG /ignore:4098)
    set_target_properties(VKstatic.${MAJOR} PROPERTIES OUTPUT_NAME VKstatic.${MAJOR})
    target_link_libraries(${API_LOWERCASE}-${MAJOR} shlwapi)
    target_link_libraries(VKstatic.${MAJOR} shlwapi)
    if (CMAKE_GENERATOR MATCHES "^Visual Studio.*")
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${API_LOWERCASE}-${MAJOR}.dll COPY_SRC_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../demos/$<CONFIGURATION>/ COPY_DST_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../tests/$<CONFIGURATION>/ COPY_DST_TEST_PATH)
    else()
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${API_LOWERCASE}-${MAJOR}.dll COPY_SRC_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../demos/ COPY_DST_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../tests/ COPY_DST_TEST_PATH)
    endif()
    add_custom_command(TARGET ${API_LOWERCASE}-${MAJOR} POST_BUILD
      COMMAND xcopy /Y /I ${COPY_SRC_PATH} ${COPY_DST_PATH})
    add_custom_command(TARGET ${API_LOWERCASE}-${MAJOR} POST_BUILD
      COMMAND xcopy /Y /I ${COPY_SRC_PATH} ${COPY_DST_TEST_PATH})

else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")

    add_library(${API_LOWERCASE} SHARED ${LOADER_SRCS})
    set_target_properties(${API_LOWERCASE} PROPERTIES SOVERSION "1" VERSION "1.0.39")
    target_link_libraries(${API_LOWERCASE} -ldl -lpthread -lm)
    install(TARGETS ${API_LOWERCASE} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
