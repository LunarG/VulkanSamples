find_package(XCB REQUIRED)

if(NOT EXISTS /usr/include/glm/glm.hpp)
    message(FATAL_ERROR "Necessary libglm-dev headers cannot be found: sudo apt-get install libglm-dev")
endif()
if(NOT EXISTS /usr/include/png.h)
    message(FATAL_ERROR "Necessary png12-dev headers cannot be found: sudo apt-get install libpng12-dev")
endif()

file(GLOB TEXTURES
  "${PROJECT_SOURCE_DIR}/demos/*.png"
  )
file(COPY ${TEXTURES} DESTINATION ${CMAKE_BINARY_DIR}/demos)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES")

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-vert.bil
   COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -b ${PROJECT_SOURCE_DIR}/demos/cube.vert 
   COMMAND mv vert.bil ${CMAKE_BINARY_DIR}/demos/cube-vert.bil
   DEPENDS cube.vert ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
   )

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-frag.bil
   COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -b ${PROJECT_SOURCE_DIR}/demos/cube.frag 
   COMMAND mv frag.bil ${CMAKE_BINARY_DIR}/demos/cube-frag.bil
   DEPENDS cube.frag ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
   )

include_directories (
   ${XCB_INCLUDE_DIRS}
   "${PROJECT_SOURCE_DIR}/icd/common"
   )

link_libraries(${XCB_LIBRARIES} XGL png m)

add_executable(xglinfo xglinfo.c)

add_executable(tri tri.c)
target_link_libraries(tri)

add_executable(cube cube.c ${CMAKE_BINARY_DIR}/demos/cube-vert.bil ${CMAKE_BINARY_DIR}/demos/cube-frag.bil)
target_link_libraries(cube)
