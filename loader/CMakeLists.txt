include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

if (WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vulkan-${MAJOR}.def
	    COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/loader/vk-loader-generate.py ${DisplayServer} win-def-file vulkan-${MAJOR}.dll all > ${CMAKE_CURRENT_BINARY_DIR}/vulkan-${MAJOR}.def
	    DEPENDS ${PROJECT_SOURCE_DIR}/loader/vk-loader-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)
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
)

set (LOADER_SRCS ${NORMAL_LOADER_SRCS} ${OPT_LOADER_SRCS})

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
    add_library(vulkan-${MAJOR} SHARED $<TARGET_OBJECTS:loader-opt> $<TARGET_OBJECTS:loader-norm> ${CMAKE_CURRENT_BINARY_DIR}/vulkan-${MAJOR}.def ${CMAKE_CURRENT_SOURCE_DIR}/loader.rc)
    set_target_properties(vulkan-${MAJOR} PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_BINARY_DIR}/vulkan-${MAJOR}.def")
    add_library(VKstatic.${MAJOR} STATIC $<TARGET_OBJECTS:loader-opt> $<TARGET_OBJECTS:loader-norm>)
    set_target_properties(VKstatic.${MAJOR} PROPERTIES OUTPUT_NAME VKstatic.${MAJOR})
    target_link_libraries(vulkan-${MAJOR} shlwapi)
    if (CMAKE_GENERATOR MATCHES "^Visual Studio.*")
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/vulkan-${MAJOR}.dll COPY_SRC_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../demos/$<CONFIGURATION>/ COPY_DST_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../tests/$<CONFIGURATION>/ COPY_DST_TEST_PATH)
    else()
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/vulkan-${MAJOR}.dll COPY_SRC_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../demos/ COPY_DST_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../tests/ COPY_DST_TEST_PATH)
    endif()
    add_custom_command(TARGET vulkan-${MAJOR} POST_BUILD
      COMMAND xcopy /Y /I ${COPY_SRC_PATH} ${COPY_DST_PATH})
    add_custom_command(TARGET vulkan-${MAJOR} POST_BUILD
      COMMAND xcopy /Y /I ${COPY_SRC_PATH} ${COPY_DST_TEST_PATH})

else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")

    add_library(vulkan SHARED ${LOADER_SRCS})
    set_target_properties(vulkan PROPERTIES SOVERSION "1" VERSION "1.0.13")
    target_link_libraries(vulkan -ldl -lpthread -lm)
endif()
