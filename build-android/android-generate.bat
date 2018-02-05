@echo off
REM # Copyright 2015 The Android Open Source Project
REM # Copyright (C) 2015 Valve Corporation
REM
REM # Licensed under the Apache License, Version 2.0 (the "License");
REM # you may not use this file except in compliance with the License.
REM # You may obtain a copy of the License at
REM
REM #      http://www.apache.org/licenses/LICENSE-2.0
REM
REM # Unless required by applicable law or agreed to in writing, software
REM # distributed under the License is distributed on an "AS IS" BASIS,
REM # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM # See the License for the specific language governing permissions and
REM # limitations under the License.

if exist generated (
  rmdir /s /q generated
)
mkdir generated\include generated\common

cd generated/include
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_safe_struct.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_safe_struct.cpp
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_struct_size_helper.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_struct_size_helper.c
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_enum_string_helper.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_object_types.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_dispatch_table_helper.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml thread_check.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml parameter_validation.cpp
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml unique_objects_wrappers.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_layer_dispatch_table.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_extension_helper.h
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml object_tracker.cpp
py -3 ../../../scripts/lvl_genvk.py -registry ../../../scripts/vk.xml vk_typemap_helper.h

set SPIRV_TOOLS_PATH=../../third_party/shaderc/third_party/spirv-tools
set SPIRV_TOOLS_UUID=spirv_tools_uuid.txt

if exist %SPIRV_TOOLS_PATH% (

  echo Found spirv-tools, using git_dir for external_revision_generator.py
  py -3 ../../../scripts/external_revision_generator.py ^
    --git_dir %SPIRV_TOOLS_PATH% ^
    -s SPIRV_TOOLS_COMMIT_ID ^
    -o spirv_tools_commit_id.h

) else (

  echo No spirv-tools git_dir found, generating UUID for external_revision_generator.py

  REM Ensure uuidgen is installed, this should error if not found
  uuidgen.exe -v

  uuidgen.exe > %SPIRV_TOOLS_UUID%
  type %SPIRV_TOOLS_UUID%
  py -3 ../../../scripts/external_revision_generator.py ^
    --rev_file %SPIRV_TOOLS_UUID% ^
    -s SPIRV_TOOLS_COMMIT_ID ^
    -o spirv_tools_commit_id.h

)

cd ../..

