#!/usr/bin/python
# Copyright 2014 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import shutil
import sys
from os import walk

def replace_string(file, original_string, new_string):
  f = open(file,'r')
  filedata = f.read()
  f.close()

  newdata = filedata.replace(original_string, new_string)
  f = open(file,'w')
  f.write(newdata)
  f.close()

samples = [
	"vk0.10-allocdescriptorsets",
  "vk0.10-copyblitimage",
  "vk0.10-dbgcreatemsgcallback",
	"vk0.10-depthbuffer",
	"vk0.10-descriptor_pipeline_layouts",
	#"vk0.10-device",     # Already converted.
	#"vk0.10-drawcube",   # Already converted.
  "vk0.10-drawsubpasses",
	"vk0.10-drawtexturedcube",
	"vk0.10-dynamicuniform",
	"vk0.10-enable_validation_with_callback",
	"vk0.10-enumerate-adv",
	"vk0.10-enumerate",
	"vk0.10-immutable_sampler",
	"vk0.10-initcommandbuffer",
	"vk0.10-initframebuffers",
	"vk0.10-initpipeline",
	"vk0.10-initrenderpass",
	"vk0.10-initshaders",
	"vk0.10-initswapchain",
	"vk0.10-inittexture",
	"vk0.10-instance",
	"vk0.10-instance_extension_properties",
	"vk0.10-instance_layer_extension_properties",
	"vk0.10-instance_layer_properties",
	"vk0.10-multiple_sets",
	"vk0.10-multithreadcmdbuf",
	"vk0.10-occlusion_query",
	"vk0.10-pipeline_cache",
	"vk0.10-pipeline_derivative",
	"vk0.10-push_constants",
	"vk0.10-secondarycmd",
	"vk0.10-separate_image_sampler",
	"vk0.10-spirv_assembly",
	"vk0.10-template",
	"vk0.10-texelbuffer",
	"vk0.10-uniformbuffer",
	"vk0.10-vertexbuffer"
  ]

template_path = "./sample_template"

for sample in samples:
  ## Copy template tree to destination.
  to_path = sample
  if os.path.exists(to_path):
    print "Skip converting sample: %s" % sample
    continue
  print "Converting sample: %s" % sample
  shutil.copytree(template_path, to_path)

  ## Copy sample body file to the destination.
  sample_file = sample + ".cpp"
  file_src_path =  "../"
  file_dest_path = to_path +  "/app/src/main/jni/"

  print file_dest_path
  os.mkdir(file_dest_path)
  shutil.copyfile(file_src_path + sample_file, file_dest_path + sample_file)

  ## Replace parameter strings in the template.
  files = []
  for (dirpath, dirnames, filenames) in walk(to_path):
    for file in filenames:
      files.append(os.path.join(dirpath, file))

  ## Replace strings in templates
  #%%SAMPLE_NAME%% sample name
  #%%SAMPLE_DESCRIPTION%% #Sample description in README.md
  #%%PACKAGE_NAME%% #package name e.g. "org.khronos.vulkan.tutorials.vkdrawcube"
  #%%SHADER_NAME%% #Shader folder name. e.g. "pass-through"

  ## Package name can not include '-', nor numbers.
  package_name = "org.khronos.vulkan.tutorials." + sample.split("-")[1]
  for file in files:
    replace_string(file, "%%SAMPLE_NAME%%", sample)
    replace_string(file, "%%SAMPLE_DESCRIPTION%%", sample)
    replace_string(file, "%%PACKAGE_NAME%%", package_name)
    replace_string(file, "%%SHADER_NAME%%", "pass-through")






  


