cmake_minimum_required(VERSION 2.8)

project(glvdebug_vk)

include("${SRC_DIR}/build_options.cmake")

find_package(Qt5 COMPONENTS Widgets Gui Core Svg QUIET)

if(NOT Qt5_FOUND)
# After Qt5.3 is installed, you may need to add the following to the cmake command line:
# -DQt5_DIR=/home/<username>/Qt/5.3/gcc_64/lib/cmake/Qt5
message(WARNING "WARNING: glvdebug_vk will be excluded because Qt5 was not found.")
else()

set(SRC_LIST
    ${SRC_LIST}
    glvdebug_vk.cpp
    glvdebug_vk_settings.cpp
    glvdebug_vk_qcontroller.cpp
    glvdebug_vk_qfile_model.cpp
    ${SRC_DIR}/glvdebug/glvdebug_QReplayWorker.cpp
    ${SRC_DIR}/glvreplay/glvreplay_factory.cpp
#    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_struct_string_helper.c
#    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_wsi_lunarg_struct_string_helper.c
)

# This should only contain headers that define a QOBJECT
# Typically that means just headers for UI objects
set(UI_HEADER_LIST
    glvdebug_vk_qcontroller.h
    glvdebug_vk_qfile_model.h
    glvdebug_vk_qgroupframesproxymodel.h
    ${SRC_DIR}/glvdebug/glvdebug_qgroupthreadsproxymodel.h
    ${SRC_DIR}/glvdebug/glvdebug_qimageviewer.h
    ${SRC_DIR}/glvdebug/glvdebug_qsvgviewer.h
    ${SRC_DIR}/glvdebug/glvdebug_QTraceFileModel.h
    ${SRC_DIR}/glvdebug/glvdebug_QReplayWidget.h
    ${SRC_DIR}/glvdebug/glvdebug_QReplayWorker.h
)

set(HDR_LIST
    glvdebug_vk_settings.h
    glvdebug_vk_qgroupframesproxymodel.h
    ${SRC_DIR}/glvdebug/glvdebug_qgroupthreadsproxymodel.h
    ${SRC_DIR}/glvdebug/glvdebug_controller.h
    ${SRC_DIR}/glvreplay/glvreplay_factory.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_enum_string_helper.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_struct_string_helper.h
#    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_wsi_lunarg_enum_string_helper.h
#    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_wsi_lunarg_struct_string_helper.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_wsi_swapchain_enum_string_helper.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_wsi_swapchain_struct_string_helper.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_wsi_device_swapchain_enum_string_helper.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_wsi_device_swapchain_struct_string_helper.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_debug_report_lunarg_enum_string_helper.h
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils/vk_debug_report_lunarg_struct_string_helper.h
    ${GLV_VULKAN_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_packet_id.h
    ${GLV_VULKAN_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_packets.h
    # ${GLV_VULKAN_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_lunarg_packets.h
    ${GLV_VULKAN_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_debug_report_lunarg_packets.h
)

include_directories(
    ${CODEGEN_GLVVK_DIR}
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/glvdebug
    ${SRC_DIR}/glvreplay
    ${SRC_DIR}/thirdparty
    ${GLV_VULKAN_DIR}/${CODEGEN_GLVVK_DIR}
    ${GLV_VULKAN_INCLUDE_DIR}
    ${GLV_VULKAN_DIR}/vulkan/codegen_utils
    ${Qt5Widgets_INCLUDE_DIRS}
)

QT5_WRAP_CPP(QT_GEN_HEADER_MOC_LIST ${UI_HEADER_LIST})

add_library(${PROJECT_NAME} SHARED ${SRC_LIST} ${HDR_LIST}
    ${QT_GEN_HEADER_MOC_LIST}
)

target_link_libraries(${PROJECT_NAME} 
    Qt5::Widgets
    Qt5::Core
    Qt5::Svg
    ${GLV_VULKAN_LIB}
    glvcommon
)

build_options_finalize()

endif(NOT Qt5_FOUND)
