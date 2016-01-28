# Copyright 2015 Google Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Materials.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

if (NOT WIN32)
    set (CMAKE_CXX_FLAGS "-std=c++11")
endif()
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../loader
	${CMAKE_CURRENT_SOURCE_DIR}/../../include/vulkan
	)

add_library(vkjson STATIC vkjson.cc vkjson_device.cc ../../loader/cJSON.c)

if(UNIX)
    add_executable(vkjson_unittest vkjson_unittest.cc)
    add_executable(vkjson_info vkjson_info.cc)
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    add_executable(vkjson_unittest vkjson_unittest.cc)
    add_executable(vkjson_info vkjson_info.cc)
endif()
target_link_libraries(vkjson_unittest vkjson)
target_link_libraries(vkjson_info vkjson vulkan-${MAJOR})
