#!/bin/bash

# Copyright 2015 The Android Open Source Project
# Copyright (C) 2015 Valve Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

dir=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
cd $dir

rm -rf generated
mkdir -p generated/include generated/common

( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_safe_struct.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_safe_struct.cpp )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_struct_size_helper.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_struct_size_helper.c )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_enum_string_helper.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_object_types.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_dispatch_table_helper.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml thread_check.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml parameter_validation.cpp )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml unique_objects_wrappers.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_loader_extensions.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_loader_extensions.c )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_layer_dispatch_table.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_extension_helper.h )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml object_tracker.cpp )
( cd generated/include; python3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_typemap_helper.h )
( cd generated/include; python3 ../../../scripts/external_revision_generator.py ../../third_party/shaderc/third_party/spirv-tools SPIRV_TOOLS_COMMIT_ID spirv_tools_commit_id.h )

exit 0
