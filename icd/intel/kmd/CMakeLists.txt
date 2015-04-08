set(sources "")
set(include_dirs "")
set(definitions "")
set(libraries icd)

if(UNIX)
    set(libdrm_sources
        libdrm/xf86drm.c
        libdrm/xf86drmHash.c
        libdrm/xf86drmMode.c
        libdrm/xf86drmRandom.c
        libdrm/intel/intel_bufmgr.c
        libdrm/intel/intel_bufmgr_gem.c
        libdrm/intel/intel_decode.c)

    list(APPEND sources
        winsys_drm.c
        ${libdrm_sources})

    list(APPEND include_dirs
        libdrm
        libdrm/include/drm
        libdrm/intel)

    list(APPEND definitions
        -D_GNU_SOURCE
        -DHAVE_LIBDRM_ATOMIC_PRIMITIVES=1)

    set_source_files_properties(${libdrm_sources} PROPERTIES COMPILE_FLAGS
        "-std=gnu99 -Wno-type-limits -Wno-unused-variable")

    find_package(PthreadStubs REQUIRED)
    list(APPEND include_dirs ${PTHREADSTUBS_INCLUDE_DIRS})
    list(APPEND libraries ${PTHREADSTUBS_LIBRARIES})

    find_package(PCIAccess REQUIRED)
    list(APPEND include_dirs ${PCIACCESS_INCLUDE_DIRS})
    list(APPEND libraries ${PCIACCESS_LIBRARIES})

    find_package(Valgrind)
    if(VALGRIND_FOUND AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND definitions -DHAVE_VALGRIND=1)
        list(APPEND include_dirs ${VALGRIND_INCLUDE_DIRS})
        list(APPEND libraries ${VALGRIND_LIBRARIES})
    endif()
endif()

add_library(intelkmd STATIC ${sources})
target_include_directories(intelkmd PRIVATE ${include_dirs})
target_include_directories(intelkmd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(intelkmd PRIVATE ${definitions})
target_link_libraries(intelkmd ${libraries})
set_target_properties(intelkmd PROPERTIES POSITION_INDEPENDENT_CODE ON)
