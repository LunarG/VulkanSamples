# - FindDRM
#
# Copyright 2015 LunarG, Inc.

find_package(PkgConfig)

if(NOT DRM_FIND_COMPONENTS)
    set(DRM_FIND_COMPONENTS libdrm)
endif()

include(FindPackageHandleStandardArgs)
set(DRM_FOUND true)
set(DRM_INCLUDE_DIRS "")
set(DRM_LIBRARIES "")
foreach(comp ${DRM_FIND_COMPONENTS})
    # component name
    string(REPLACE "lib" "" compname ${comp})
    string(TOUPPER ${compname} compname)
    # header name
    if(comp STREQUAL "libdrm")
        set(headername xf86drm.h)
    elseif(comp STREQUAL "libdrm_intel")
        set(headername intel_bufmgr.h)
    else()
        string(REPLACE "libdrm_" "" headername ${comp})
    endif()
    # library name
    string(REPLACE "lib" "" libname ${comp})

    pkg_check_modules(PC_${comp} QUIET ${comp})

    find_path(${compname}_INCLUDE_DIR NAMES ${headername}
        HINTS
        ${PC_${comp}_INCLUDEDIR}
        ${PC_${comp}_INCLUDE_DIRS}
        )

    find_library(${compname}_LIBRARY NAMES ${libname}
        HINTS
        ${PC_${comp}_LIBDIR}
        ${PC_${comp}_LIBRARY_DIRS}
        )

    find_package_handle_standard_args(${comp}
        FOUND_VAR ${comp}_FOUND
        REQUIRED_VARS ${compname}_INCLUDE_DIR ${compname}_LIBRARY)
    mark_as_advanced(${compname}_INCLUDE_DIR ${compname}_LIBRARY)

    list(APPEND DRM_INCLUDE_DIRS ${${compname}_INCLUDE_DIR})
    list(APPEND DRM_LIBRARIES ${${compname}_LIBRARY})

    if(NOT ${comp}_FOUND)
        set(DRM_FOUND false)
    endif()
endforeach()

list(REMOVE_DUPLICATES DRM_INCLUDE_DIRS)
