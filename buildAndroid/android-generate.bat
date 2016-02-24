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
mkdir generated

python ../vk-generate.py dispatch-table-ops layer > generated/vk_dispatch_table_helper.h

python ../vk_helper.py --gen_enum_string_helper ../include/vulkan/vulkan.h --abs_out_dir generated
python ../vk_helper.py --gen_struct_wrappers ../include/vulkan/vulkan.h --abs_out_dir generated

python ../vk-layer-generate.py object_tracker ../include/vulkan/vulkan.h > generated/object_tracker.cpp
python ../vk-layer-generate.py unique_objects ../include/vulkan/vulkan.h > generated/unique_objects.cpp

cd generated
python ../../genvk.py threading -registry ../../vk.xml thread_check.h
python ../../genvk.py paramchecker -registry ../../vk.xml param_check.h
cd ..