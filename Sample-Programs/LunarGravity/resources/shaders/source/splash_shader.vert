/*
 * Copyright (c) 2017 Valve Corporation
 * Copyright (c) 2017 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Vertex shader used by splash screen scene
 */
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct VtxData
{
  vec4 position;
  vec2 texcoord;
};

layout(std140, binding = 0) uniform buf {
        mat4 modelviewprojectmat;
        VtxData vertexdata[6];
} ubuf;

layout (location = 0) out vec2 texcoord;

void main() 
{
   texcoord = ubuf.vertexdata[gl_VertexIndex].texcoord;
   gl_Position = ubuf.modelviewprojectmat * ubuf.vertexdata[gl_VertexIndex].position;
}
