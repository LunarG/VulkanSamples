#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <vulkan.h>

#include "gtest/gtest.h"
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "vktestbinding.h"

#define ASSERT_VK_SUCCESS(err) ASSERT_EQ(VK_SUCCESS, err) << vk_result_string(err)

static inline const char *vk_result_string(VK_RESULT err)
{
    switch (err) {
#define STR(r) case r: return #r
    STR(VK_SUCCESS);
    STR(VK_UNSUPPORTED);
    STR(VK_NOT_READY);
    STR(VK_TIMEOUT);
    STR(VK_EVENT_SET);
    STR(VK_EVENT_RESET);
    STR(VK_ERROR_UNKNOWN);
    STR(VK_ERROR_UNAVAILABLE);
    STR(VK_ERROR_INITIALIZATION_FAILED);
    STR(VK_ERROR_OUT_OF_MEMORY);
    STR(VK_ERROR_OUT_OF_GPU_MEMORY);
    STR(VK_ERROR_DEVICE_ALREADY_CREATED);
    STR(VK_ERROR_DEVICE_LOST);
    STR(VK_ERROR_INVALID_POINTER);
    STR(VK_ERROR_INVALID_VALUE);
    STR(VK_ERROR_INVALID_HANDLE);
    STR(VK_ERROR_INVALID_ORDINAL);
    STR(VK_ERROR_INVALID_MEMORY_SIZE);
    STR(VK_ERROR_INVALID_EXTENSION);
    STR(VK_ERROR_INVALID_FLAGS);
    STR(VK_ERROR_INVALID_ALIGNMENT);
    STR(VK_ERROR_INVALID_FORMAT);
    STR(VK_ERROR_INVALID_IMAGE);
    STR(VK_ERROR_INVALID_DESCRIPTOR_SET_DATA);
    STR(VK_ERROR_INVALID_QUEUE_TYPE);
    STR(VK_ERROR_INVALID_OBJECT_TYPE);
    STR(VK_ERROR_UNSUPPORTED_SHADER_IL_VERSION);
    STR(VK_ERROR_BAD_SHADER_CODE);
    STR(VK_ERROR_BAD_PIPELINE_DATA);
    STR(VK_ERROR_TOO_MANY_MEMORY_REFERENCES);
    STR(VK_ERROR_NOT_MAPPABLE);
    STR(VK_ERROR_MEMORY_MAP_FAILED);
    STR(VK_ERROR_MEMORY_UNMAP_FAILED);
    STR(VK_ERROR_INCOMPATIBLE_DEVICE);
    STR(VK_ERROR_INCOMPATIBLE_DRIVER);
    STR(VK_ERROR_INCOMPLETE_COMMAND_BUFFER);
    STR(VK_ERROR_BUILDING_COMMAND_BUFFER);
    STR(VK_ERROR_MEMORY_NOT_BOUND);
    STR(VK_ERROR_INCOMPATIBLE_QUEUE);
    STR(VK_ERROR_NOT_SHAREABLE);
#undef STR
    default: return "UNKNOWN_RESULT";
    }
}

static inline void test_error_callback(const char *expr, const char *file,
                                       unsigned int line, const char *function)
{
    ADD_FAILURE_AT(file, line) << "Assertion: `" << expr << "'";
}

#endif // TEST_COMMON_H
