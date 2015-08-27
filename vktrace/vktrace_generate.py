#!/usr/bin/env python3
#
# Vulkan
#
# Copyright (C) 2014 LunarG, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

import os, sys


# add main repo directory so vulkan.py can be imported. This needs to be a complete path.
glv_scripts_path = os.path.dirname(os.path.abspath(__file__))
main_path = os.path.abspath(glv_scripts_path + "/../")
sys.path.append(main_path)
from source_line_info import sourcelineinfo

import vulkan

# vulkan.py doesn't include all the extensions (debug_report missing)
headers = []
objects = []
protos = []
for ext in vulkan.extensions_all:
    headers.extend(ext.headers)
    objects.extend(ext.objects)
    protos.extend(ext.protos)

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.extName = argv
        self.headers = headers
        self.objects = objects
        self.protos = protos
        self.lineinfo = sourcelineinfo()

    def run(self):
        print(self.generate(self.extName))

    def generate(self, extName):
        copyright = self.generate_copyright()
        header = self.generate_header(extName)
        body = self.generate_body()
        footer = self.generate_footer()
        contents = []
        if copyright:
            contents.append(copyright)
        if header:
            contents.append(header)
        if body:
            contents.append(body)
        if footer:
            contents.append(footer)

        return "\n\n".join(contents)

    def generate_copyright(self):
        return """/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */"""

    def generate_header(self, extName):
        return "\n".join(["#include <" + h + ">" for h in self.headers])

    def generate_body(self):
        pass

    def generate_footer(self):
        pass

    def _generate_trace_func_ptrs(self):
        func_ptrs = []
        func_ptrs.append('// Pointers to real functions and declarations of hooked functions')
        func_ptrs.append('#ifdef WIN32')
        func_ptrs.append('extern INIT_ONCE gInitOnce;')
        for proto in vulkan.core.protos:
            func_ptrs.append('#define __HOOKED_vk%s hooked_vk%s' % (proto.name, proto.name))

        func_ptrs.append('\n#elif defined(PLATFORM_LINUX)')
        func_ptrs.append('extern pthread_once_t gInitOnce;')
        for proto in vulkan.core.protos:
            func_ptrs.append('#define __HOOKED_vk%s vk%s' % (proto.name, proto.name))

        func_ptrs.append('#endif\n')
        return "\n".join(func_ptrs)

    def _generate_trace_func_ptrs_ext(self, extName):
        func_ptrs = []
        func_ptrs.append('#ifdef WIN32')
        for ext in vulkan.extensions_all:
            if (extName.lower() == ext.name.lower()):
                for proto in ext.protos:
                    func_ptrs.append('#define __HOOKED_vk%s hooked_vk%s' % (proto.name, proto.name))

        func_ptrs.append('#elif defined(__linux__)')
        for ext in vulkan.extensions_all:
            if (extName.lower() == ext.name.lower()):
                for proto in ext.protos:
                    func_ptrs.append('#define __HOOKED_vk%s vk%s' % (proto.name, proto.name))

        func_ptrs.append('#endif\n')
        return "\n".join(func_ptrs)

    def _generate_trace_func_protos(self):
        func_protos = []
        func_protos.append('// Hooked function prototypes\n')
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'WSI' not in proto.name:
                func_protos.append('GLVTRACER_EXPORT %s;' % proto.c_func(prefix="__HOOKED_vk", attr="VKAPI"))

        return "\n".join(func_protos)

    def _generate_trace_func_protos_ext(self, extName):
        func_protos = []
        func_protos.append('// Hooked function prototypes\n')
        for ext in vulkan.extensions_all:
            if (extName.lower() == ext.name.lower()):
                for proto in ext.protos:
                    func_protos.append('GLVTRACER_EXPORT %s;' % proto.c_func(prefix="__HOOKED_vk", attr="VKAPI"))

        return "\n".join(func_protos)


    def _generate_trace_real_func_ptr_protos(self):
        func_ptr_assign = []
        func_ptr_assign.append('')
        for proto in self.protos:
            func_ptr_assign.append('extern %s( VKAPI * real_vk%s)(' % (proto.ret, proto.name))
            for p in proto.params:
                func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
            func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ');\n')
        return "\n".join(func_ptr_assign)

    def _generate_func_ptr_assignments(self):
        func_ptr_assign = []
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'WSI' not in proto.name:
                func_ptr_assign.append('%s( VKAPI * real_vk%s)(' % (proto.ret, proto.name))
                for p in proto.params:
                    func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
                func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ') = vk%s;\n' % (proto.name))
        return "\n".join(func_ptr_assign)


    def _generate_func_ptr_assignments_ext(self, extName):
        func_ptr_assign = []
        for ext in vulkan.extensions_all:
            if ext.name.lower() == extName.lower():
                for proto in ext.protos:
                    func_ptr_assign.append('%s( VKAPI * real_vk%s)(' % (proto.ret, proto.name))
                    for p in proto.params:
                        func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
                    func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ');\n')
        return "\n".join(func_ptr_assign)

    def _generate_attach_hooks(self):
        hooks_txt = []
        hooks_txt.append('// declared as extern in vktrace_lib_helpers.h')
        hooks_txt.append('BOOL isHooked = FALSE;\n')
        hooks_txt.append('void AttachHooks()\n{\n   BOOL hookSuccess = TRUE;\n#if defined(WIN32)')
        hooks_txt.append('    Mhook_BeginMultiOperation(FALSE);')
        hooks_txt.append('    if (real_vkCreateInstance != NULL)')
        hooks_txt.append('    {\n        isHooked = TRUE;')
        hook_operator = '='
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'WSI' not in proto.name:
                hooks_txt.append('        hookSuccess %s Mhook_SetHook((PVOID*)&real_vk%s, hooked_vk%s);' % (hook_operator, proto.name, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }\n')
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook Vulkan.");\n    }\n')
        hooks_txt.append('    Mhook_EndMultiOperation();\n')
        hooks_txt.append('#elif defined(__linux__)')
        hooks_txt.append('    if (real_vkCreateInstance == vkCreateInstance)')
        hooks_txt.append('        hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_vkCreateInstance,"vkCreateInstance");')
        hooks_txt.append('    isHooked = TRUE;')
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'WSI' not in proto.name and 'CreateInstance' not in proto.name:
                hooks_txt.append('    hookSuccess %s glv_platform_get_next_lib_sym((PVOID*)&real_vk%s, "vk%s");' % (hook_operator, proto.name, proto.name))
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook Vulkan.");\n    }\n')
        hooks_txt.append('#endif\n}\n')
        return "\n".join(hooks_txt)

    def _generate_detach_hooks(self):
        hooks_txt = []
        hooks_txt.append('void DetachHooks()\n{\n#ifdef __linux__\n    return;\n#elif defined(WIN32)')
        hooks_txt.append('    BOOL unhookSuccess = TRUE;\n    if (real_vkGetPhysicalDeviceProperties != NULL)\n    {')
        hook_operator = '='
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'WSI' not in proto.name:
                hooks_txt.append('        unhookSuccess %s Mhook_Unhook((PVOID*)&real_vk%s);' % (hook_operator, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }\n    isHooked = FALSE;')
        hooks_txt.append('    if (!unhookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to unhook Vulkan.");\n    }')
        hooks_txt.append('#endif\n}')
        hooks_txt.append('#ifdef WIN32\nINIT_ONCE gInitOnce = INIT_ONCE_STATIC_INIT;\n#elif defined(PLATFORM_LINUX)\npthread_once_t gInitOnce = PTHREAD_ONCE_INIT;\n#endif\n')
        return "\n".join(hooks_txt)

    def _generate_init_funcs(self):
        init_tracer = []
        init_tracer.append('void send_vk_api_version_packet()\n{')
        init_tracer.append('    packet_vkApiVersion* pPacket;')
        init_tracer.append('    glv_trace_packet_header* pHeader;')
        init_tracer.append('    pHeader = glv_create_trace_packet(GLV_TID_VULKAN, GLV_TPI_VK_vkApiVersion, sizeof(packet_vkApiVersion), 0);')
        init_tracer.append('    pPacket = interpret_body_as_vkApiVersion(pHeader);')
        init_tracer.append('    pPacket->version = VK_API_VERSION;')
        init_tracer.append('    glv_set_packet_entrypoint_end_time(pHeader);')
        init_tracer.append('    FINISH_TRACE_PACKET();\n}\n')

        init_tracer.append('extern GLV_CRITICAL_SECTION g_memInfoLock;')
        init_tracer.append('void InitTracer(void)\n{')
        init_tracer.append('    const char *ipAddr = glv_get_global_var("GLVLIB_TRACE_IPADDR");')
        init_tracer.append('    if (ipAddr == NULL)')
        init_tracer.append('        ipAddr = "127.0.0.1";')
        init_tracer.append('    gMessageStream = glv_MessageStream_create(FALSE, ipAddr, GLV_BASE_PORT + GLV_TID_VULKAN);')
        init_tracer.append('    glv_trace_set_trace_file(glv_FileLike_create_msg(gMessageStream));')
        init_tracer.append('    glv_tracelog_set_tracer_id(GLV_TID_VULKAN);')
        init_tracer.append('    glv_create_critical_section(&g_memInfoLock);')
        init_tracer.append('    send_vk_api_version_packet();\n}\n')
        return "\n".join(init_tracer)

    # Take a list of params and return a list of dicts w/ ptr param details
    def _get_packet_ptr_param_list(self, params):
        ptr_param_list = []
        # TODO : This is a slightly nicer way to handle custom cases than initial code, however
        #   this can still be further generalized to eliminate more custom code
        #   big case to handle is when ptrs to structs have embedded data that needs to be accounted for in packet
        custom_ptr_dict = {'VkDeviceCreateInfo': {'add_txt': 'add_VkDeviceCreateInfo_to_packet(pHeader, (VkDeviceCreateInfo**) &(pPacket->pCreateInfo), pCreateInfo)',
                                                  'finalize_txt': ''},
                           'VkApplicationInfo': {'add_txt': 'add_VkApplicationInfo_to_packet(pHeader, (VkApplicationInfo**)&(pPacket->pAppInfo), pAppInfo)',
                                                 'finalize_txt': ''},
                           'VkPhysicalDevice': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpus), *pGpuCount*sizeof(VkPhysicalDevice), pGpus)',
                                                'finalize_txt': 'default'},
                           'pDataSize': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize)',
                                         'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize))'},
#                           'pData': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData)',
#                                     'finalize_txt': 'default'},
                           'pName': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pName), ((pName != NULL) ? strlen(pName) + 1 : 0), pName)',
                                     'finalize_txt': 'default'},
                           'pMarker': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMarker), ((pMarker != NULL) ? strlen(pMarker) + 1 : 0), pMarker)',
                                       'finalize_txt': 'default'},
                           'pExtName': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pExtName), ((pExtName != NULL) ? strlen(pExtName) + 1 : 0), pExtName)',
                                        'finalize_txt': 'default'},
                           'pDescriptorSets': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), customSize, pDescriptorSets)',
                                               'finalize_txt': 'default'},
                           'pSparseMemoryRequirements': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSparseMemoryRequirements), (*pNumRequirements) * sizeof(VkSparseImageMemoryRequirements), pSparseMemoryRequirements)',
                                               'finalize_txt': 'default'},
                           'VkSparseImageFormatProperties': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), (*pNumProperties) * sizeof(VkSparseImageFormatProperties), pProperties)',
                                               'finalize_txt': 'default'},
                           'VkSparseMemoryBindInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBindInfo), numBindings * sizeof(VkSparseMemoryBindInfo), pBindInfo)',
                                               'finalize_txt': 'default'},
                           'VkSparseImageMemoryBindInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBindInfo), numBindings * sizeof(VkSparseImageMemoryBindInfo), pBindInfo)',
                                               'finalize_txt': 'default'},
#                           'VkShaderCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkShaderCreateInfo), pCreateInfo);\n'
#                                                             '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), ((pCreateInfo != NULL) ? pCreateInfo->codeSize : 0), pCreateInfo->pCode)',
#                                             'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));\n'
#                                                             '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkFramebufferCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkFramebufferCreateInfo), pCreateInfo);\n'
                                                                  '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments), colorCount * sizeof(VkColorAttachmentBindInfo), pCreateInfo->pColorAttachments);\n'
                                                                  '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment), dsSize, pCreateInfo->pDepthStencilAttachment)',
                                                  'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments));\n'
                                                                  '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment));\n'
                                                                  '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkRenderPassCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkRenderPassCreateInfo), pCreateInfo);\n'
                                                                 '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorFormats), colorCount * sizeof(VkFormat), pCreateInfo->pColorFormats);\n'
                                                                 '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLayouts), colorCount * sizeof(VkImageLayout), pCreateInfo->pColorLayouts);\n'
                                                                 '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps), colorCount * sizeof(VkAttachmentLoadOp), pCreateInfo->pColorLoadOps);\n'
                                                                 '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps), colorCount * sizeof(VkAttachmentStoreOp), pCreateInfo->pColorStoreOps);\n'
                                                                 '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues), colorCount * sizeof(VkClearColor), pCreateInfo->pColorLoadClearValues)',
                                                 'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorFormats));\n'
                                                                 '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLayouts));\n'
                                                                 '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps));\n'
                                                                 '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps));\n'
                                                                 '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues));\n'
                                                                 '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkPipelineLayoutCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkPipelineLayoutCreateInfo), pCreateInfo);\n'
                                                                     '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pSetLayouts), pCreateInfo->descriptorSetCount * sizeof(VkDescriptorSetLayout), pCreateInfo->pSetLayouts);',
                                                     'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pSetLayouts));\n'
                                                                     '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkDynamicViewportStateCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicViewportStateCreateInfo), pCreateInfo);\n'
                                                                     '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(VkViewport), pCreateInfo->pViewports);\n'
                                                                     '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(VkRect2D), pCreateInfo->pScissors)',
                                                     'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));\n'
                                                                     '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));\n'
                                                                     '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkMemoryAllocInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(VkMemoryAllocInfo), pAllocInfo);\n'
                                                            '    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext)',
                                            'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocInfo))'},
#                          'VkGraphicsPipelineCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfos), count*sizeof(VkGraphicsPipelineCreateInfo), pCreateInfos);\n'
#                                                                      '    add_VkGraphicsPipelineCreateInfos_to_trace_packet(pHeader, (VkGraphicsPipelineCreateInfo*)pPacket->pCreateInfos, pCreateInfos, count)',
#                                                      'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfos))'},
#                          'VkComputePipelineCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfos), count*sizeof(VkComputePipelineCreateInfo), pCreateInfos);\n'
#                                                                      '    add_VkComputePipelineCreateInfos_to_trace_packet(pHeader, (VkComputePipelineCreateInfo*)pPacket->pCreateInfos, pCreateInfos, count)',
#                                                      'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfos))'},
                           'VkDescriptorPoolCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDescriptorPoolCreateInfo), pCreateInfo);\n'
                                                                     '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), pCreateInfo->count * sizeof(VkDescriptorTypeCount), pCreateInfo->pTypeCount)',
                                                     'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));\n'
                                                                     '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkDescriptorSetLayoutCreateInfo': {'add_txt': 'add_create_ds_layout_to_trace_packet(pHeader, &pPacket->pCreateInfo, pCreateInfo)',
                                                          'finalize_txt': '// pCreateInfo finalized in add_create_ds_layout_to_trace_packet'},
                           'VkSwapChainCreateInfoWSI': {'add_txt':      'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkSwapChainCreateInfoWSI), pCreateInfo);\n'
                                                                        '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pSurfaceDescription), sizeof(VkSurfaceDescriptionWSI), pCreateInfo->pSurfaceDescription)',
                                                        'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pSurfaceDescription));\n'
                                                                        '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkShaderModuleCreateInfo': {'add_txt':      'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkShaderModuleCreateInfo), pCreateInfo);\n'
                                                                        '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), pPacket->pCreateInfo->codeSize, pCreateInfo->pCode)',
                                                        'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));\n'
                                                                        '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkShaderCreateInfo': {'add_txt':    'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkShaderModuleCreateInfo), pCreateInfo);\n'
                                                                '    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pName), strlen(pPacket->pCreateInfo->pName), pCreateInfo->pName)',
                                                                'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pName));\n'
                                                                '    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                          }

        for p in params:
            pp_dict = {}
            if '*' in p.ty and p.name not in ['pSysMem', 'pReserved']:
# LUGMAL        if 'const' in p.ty.lower() and 'count' in params[params.index(p)-1].name.lower() and p.name != 'pCreateInfos':
                if 'const' in p.ty.lower() and 'count' in params[params.index(p)-1].name.lower():
                    pp_dict['add_txt'] = 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s*sizeof(%s), %s)' % (p.name, params[params.index(p)-1].name, p.ty.strip('*').replace('const ', ''), p.name)
                elif 'pOffsets' == p.name: # TODO : This is a custom case for BindVertexBuffers last param, need to clean this up
                    pp_dict['add_txt'] = 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s*sizeof(%s), %s)' % (p.name, params[params.index(p)-2].name, p.ty.strip('*').replace('const ', ''), p.name)
                elif p.ty.strip('*').replace('const ', '') in custom_ptr_dict:
                    pp_dict['add_txt'] = custom_ptr_dict[p.ty.strip('*').replace('const ', '')]['add_txt']
                    pp_dict['finalize_txt'] = custom_ptr_dict[p.ty.strip('*').replace('const ', '')]['finalize_txt']
                elif p.name in custom_ptr_dict:
                    pp_dict['add_txt'] = custom_ptr_dict[p.name]['add_txt']
                    pp_dict['finalize_txt'] = custom_ptr_dict[p.name]['finalize_txt']
                    # TODO : This is custom hack to account for 2 pData items with dataSize param for sizing
                    if 'pData' == p.name and 'dataSize' == params[params.index(p)-1].name:
                        pp_dict['add_txt'] = pp_dict['add_txt'].replace('_dataSize', 'dataSize')
                else:
                    pp_dict['add_txt'] = 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(%s), %s)' % (p.name, p.ty.strip('*').replace('const ', ''), p.name)
                if 'finalize_txt' not in pp_dict or 'default' == pp_dict['finalize_txt']:
                    pp_dict['finalize_txt'] = 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s))' % (p.name)
                pp_dict['index'] = params.index(p)
                ptr_param_list.append(pp_dict)
        return ptr_param_list

    # Take a list of params and return a list of packet size elements
    def _get_packet_size(self, extName, params):
        ps = [] # List of elements to be added together to account for packet size for given params
        skip_list = [] # store params that are already accounted for so we don't count them twice
        # Dict of specific params with unique custom sizes
        custom_size_dict = {'pSetBindPoints': '(VK_SHADER_STAGE_COMPUTE * sizeof(uint32_t))', # Accounting for largest possible array
                            'VkSwapChainCreateInfoWSI' : 'vk_wsi_device_swapchain_size_vkswapchaincreateinfowsi(pCreateInfo)',
                            }
        size_func_suffix = ''
        if extName.lower() != "vk_core":
            size_func_suffix = '_%s' % extName.lower()
        for p in params:
            #First handle custom cases
            if p.name in ['pCreateInfo', 'pSetLayoutInfoList', 'pBeginInfo', 'pAllocInfo'] and 'wsi' not in p.ty.lower():
                ps.append('get_struct_chain_size%s((void*)%s)' % (size_func_suffix, p.name))
                skip_list.append(p.name)
            elif p.name in custom_size_dict:
                ps.append(custom_size_dict[p.name])
                skip_list.append(p.name)
            elif p.ty.strip('*').replace('const ', '') in custom_size_dict:
                tmp_ty = p.ty.strip('*').replace('const ', '')
                ps.append(custom_size_dict[tmp_ty])
                skip_list.append(p.name)
            # Skip any params already handled
            if p.name in skip_list:
                continue
            # Now check to identify dynamic arrays which depend on two params
            if 'count' in p.name.lower():
                next_idx = params.index(p)+1
                # If next element is a const *, then multiply count and array type
                if next_idx < len(params) and '*' in params[next_idx].ty and 'const' in params[next_idx].ty.lower():
                    if '*' in p.ty:
                        ps.append('*%s*sizeof(%s)' % (p.name, params[next_idx].ty.strip('*').replace('const ', '')))
                    else:
                        ps.append('%s*sizeof(%s)' % (p.name, params[next_idx].ty.strip('*').replace('const ', '')))
                    skip_list.append(params[next_idx].name)
                if 'bindingCount' == p.name: # TODO : This is custom case for CmdBindVertexBuffers, need to clean it up
                    ps.append('%s*sizeof(%s)' % (p.name, params[next_idx+1].ty.strip('*').replace('const ', '')))
                    skip_list.append(params[next_idx+1].name)
                elif '*' in p.ty: # Not a custom array size we're aware of, but ptr so need to account for its size
                    ps.append('sizeof(%s)' % (p.ty.strip('*').replace('const ', '')))
            elif '*' in p.ty and p.name not in ['pSysMem', 'pReserved']:
                if 'pData' == p.name:
                    if 'dataSize' == params[params.index(p)-1].name:
                        ps.append('dataSize')
                    elif 'counterCount' == params[params.index(p)-1].name:
                        ps.append('sizeof(%s)' % p.ty.strip('*').replace('const ', ''))
                    else:
                        #ps.append('((pDataSize != NULL && pData != NULL) ? *pDataSize : 0)')
                        ps.append('sizeof(void*)')
                elif '**' in p.ty and 'void' in p.ty:
                    ps.append('sizeof(void*)')
                elif 'void' in p.ty:
                    ps.append('sizeof(%s)' % p.name)
                elif 'char' in p.ty:
                    ps.append('((%s != NULL) ? strlen(%s) + 1 : 0)' % (p.name, p.name))
                elif 'pDataSize' in p.name:
                    ps.append('((pDataSize != NULL) ? sizeof(size_t) : 0)')
                elif 'IMAGE_SUBRESOURCE' in p.ty and 'pSubresource' == p.name:
                    ps.append('((pSubresource != NULL) ? sizeof(VkImage_SUBRESOURCE) : 0)')
                else:
                    ps.append('sizeof(%s)' % (p.ty.strip('*').replace('const ', '')))
        return ps

    # Generate functions used to trace API calls and store the input and result data into a packet
    # Here's the general flow of code insertion w/ option items flagged w/ "?"
    # Result decl?
    # Packet struct decl
    # ?Special case : setup call to function first and do custom API call time tracking
    # CREATE_PACKET
    # call real entrypoint and get return value (if there is one)
    # Assign packet values
    # FINISH packet
    # return result if needed
    def _generate_trace_funcs(self, extName):
        func_body = []
        manually_written_hooked_funcs = ['AllocMemory',
                                         'AllocDescriptorSets',
                                         'CreateDynamicViewportState',
                                         'CreateDescriptorPool',
                                         'CreateDevice',
                                         'CreateFramebuffer',
                                         'CreateInstance',
                                         'CreateRenderPass',
                                         'CreateGraphicsPipelines',
                                         'CreateComputePipelines',
                                         'CmdPipelineBarrier',
                                         'CmdWaitEvents',
                                         'CmdBeginRenderPass',
                                         'EnumeratePhysicalDevices',
                                         'FreeMemory',
                                         'FreeDescriptorSets',
                                         'FlushMappedMemoryRanges',
                                         'GetGlobalExtensionProperties',
                                         'GetPhysicalDeviceExtensionProperties',
                                         'GetGlobalLayerProperties',
                                         'GetPhysicalDeviceLayerProperties',
                                         'GetPhysicalDeviceQueueFamilyProperties',
                                         'GetQueryPoolResults',
                                         'MapMemory',
                                         'UnmapMemory',
                                         'UpdateDescriptorSets',
                                         'GetSurfacePropertiesWSI',
                                         'GetSurfaceFormatsWSI',
                                         'GetSurfacePresentModesWSI',
                                         'CreateSwapChainWSI',
                                         'GetSwapChainImagesWSI',
                                         'QueuePresentWSI',
                                         'CreateDynamicStencilState',
                                         ]

        # validate the manually_written_hooked_funcs list
        protoFuncs = [proto.name for proto in self.protos]
        for func in manually_written_hooked_funcs:
            if func not in protoFuncs:
                sys.exit("Entry '%s' in manually_written_hooked_funcs list is not in the vulkan function prototypes" % func)

        # process each of the entrypoint prototypes
        for ext in vulkan.extensions_all:
            if ext.name.lower() == extName.lower():
                for proto in ext.protos:
                    if proto.name in manually_written_hooked_funcs:
                        func_body.append( '// __HOOKED_vk%s is manually written. Look in vktrace_vk_trace.c\n' % proto.name)
                    else:
                        raw_packet_update_list = [] # non-ptr elements placed directly into packet
                        ptr_packet_update_list = [] # ptr elements to be updated into packet
                        return_txt = ''
                        packet_size = []
                        in_data_size = False # flag when we need to capture local input size variable for in/out size
                        func_body.append('%s' % self.lineinfo.get())
                        func_body.append('GLVTRACER_EXPORT %s VKAPI __HOOKED_vk%s(' % (proto.ret, proto.name))
                        for p in proto.params: # TODO : For all of the ptr types, check them for NULL and return 0 if NULL
                            func_body.append('    %s %s,' % (p.ty, p.name))
                            if '*' in p.ty and p.name not in ['pSysMem', 'pReserved']:
                                if 'pDataSize' in p.name:
                                    in_data_size = True;
                            elif 'pfnMsgCallback' == p.name:
                                raw_packet_update_list.append('    PFN_vkDbgMsgCallback* pNonConstCallback = (PFN_vkDbgMsgCallback*)&pPacket->pfnMsgCallback;')
                                raw_packet_update_list.append('    *pNonConstCallback = pfnMsgCallback;')
                            else:
                                raw_packet_update_list.append('    pPacket->%s = %s;' % (p.name, p.name))
                        # Get list of packet size modifiers due to ptr params
                        packet_size = self._get_packet_size(extName, proto.params)
                        ptr_packet_update_list = self._get_packet_ptr_param_list(proto.params)
                        func_body[-1] = func_body[-1].replace(',', ')')
                        # End of function declaration portion, begin function body
                        func_body.append('{\n    glv_trace_packet_header* pHeader;')
                        if 'void' not in proto.ret or '*' in proto.ret:
                            func_body.append('    %s result;' % proto.ret)
                            return_txt = 'result = '
                        if in_data_size:
                            func_body.append('    size_t _dataSize;')
                        func_body.append('    packet_vk%s* pPacket = NULL;' % proto.name)
                        if (0 == len(packet_size)):
                            func_body.append('    CREATE_TRACE_PACKET(vk%s, 0);' % (proto.name))
                        else:
                            func_body.append('    CREATE_TRACE_PACKET(vk%s, %s);' % (proto.name, ' + '.join(packet_size)))

                        # TODO: need a better way to indicate which extensions should be mapped to which Get*ProcAddr
                        if proto.name == 'GetInstanceProcAddr':
                            for iProto in self.protos:
                                if 'Dbg' in iProto.name or 'GetPhysicalDeviceSurfaceSupportWSI' in iProto.name:
                                    func_body.append('    if (strcmp(pName, "vk%s") == 0) {' % (iProto.name))
                                    func_body.append('        real_vk%s = (PFN_vk%s)real_vkGetInstanceProcAddr(instance, pName);' % (iProto.name, iProto.name))
                                    func_body.append('        glv_set_packet_entrypoint_end_time(pHeader);')
                                    func_body.append('        if (real_vk%s != NULL) {' % (iProto.name))
                                    func_body.append('            result = (PFN_vkVoidFunction)__HOOKED_vk%s;' % (iProto.name))
                                    func_body.append('        } else {')
                                    func_body.append('            result = NULL;')
                                    func_body.append('        }')
                                    func_body.append('    }')
                        elif proto.name == 'GetDeviceProcAddr':
                            for dProto in self.protos:
                                if 'WSI' in dProto.name:
                                    func_body.append('    if (strcmp(pName, "vk%s") == 0) {' % (dProto.name))
                                    func_body.append('        real_vk%s = (PFN_vk%s)real_vkGetDeviceProcAddr(device, pName);' % (dProto.name, dProto.name))
                                    func_body.append('        glv_set_packet_entrypoint_end_time(pHeader);')
                                    func_body.append('        if (real_vk%s != NULL) {' % (dProto.name))
                                    func_body.append('            result = (PFN_vkVoidFunction)__HOOKED_vk%s;' % (dProto.name))
                                    func_body.append('        } else {')
                                    func_body.append('            result = NULL;')
                                    func_body.append('        }')
                                    func_body.append('    }')
                        else:
                            # call real entrypoint and get return value (if there is one)
                            func_body.append('    %sreal_vk%s;' % (return_txt, proto.c_call()))
                            func_body.append('    glv_set_packet_entrypoint_end_time(pHeader);')

                        if in_data_size:
                            func_body.append('    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;')
                        func_body.append('    pPacket = interpret_body_as_vk%s(pHeader);' % proto.name)
                        func_body.append('\n'.join(raw_packet_update_list))
                        for pp_dict in ptr_packet_update_list: #buff_ptr_indices:
                            func_body.append('    %s;' % (pp_dict['add_txt']))
                        if 'void' not in proto.ret or '*' in proto.ret:
                            func_body.append('    pPacket->result = result;')
                        for pp_dict in ptr_packet_update_list:
                            if ('DeviceCreateInfo' not in proto.params[pp_dict['index']].ty):
                                func_body.append('    %s;' % (pp_dict['finalize_txt']))
                        # All buffers should be finalized by now, and the trace packet can be finished (which sends it over the socket)
                        func_body.append('    FINISH_TRACE_PACKET();')
                        # return result if needed
                        if 'void' not in proto.ret or '*' in proto.ret:
                            func_body.append('    return result;')
                        func_body.append('}\n')
        return "\n".join(func_body)

    def _generate_packet_id_enum(self):
        pid_enum = []
        pid_enum.append('enum GLV_TRACE_PACKET_ID_VK')
        pid_enum.append('{')
        first_func = True
        for proto in self.protos:
            if first_func:
                first_func = False
                pid_enum.append('    GLV_TPI_VK_vkApiVersion = GLV_TPI_BEGIN_API_HERE,')
                pid_enum.append('    GLV_TPI_VK_vk%s,' % proto.name)
            else:
                pid_enum.append('    GLV_TPI_VK_vk%s,' % proto.name)
        pid_enum.append('};\n')
        return "\n".join(pid_enum)

    def _generate_packet_id_name_func(self):
        func_body = []
        func_body.append('static const char *glv_vk_packet_id_name(const enum GLV_TRACE_PACKET_ID_VK id)')
        func_body.append('{')
        func_body.append('    switch(id) {')
        func_body.append('    case GLV_TPI_VK_vkApiVersion:')
        func_body.append('    {')
        func_body.append('        return "vkApiVersion";')
        func_body.append('    }')
        for proto in self.protos:
            func_body.append('    case GLV_TPI_VK_vk%s:' % proto.name)
            func_body.append('    {')
            func_body.append('        return "vk%s";' % proto.name)
            func_body.append('    }')
        func_body.append('    default:')
        func_body.append('        return NULL;')
        func_body.append('    }')
        func_body.append('}\n')
        return "\n".join(func_body)

    def _generate_interp_func(self):
        interp_func_body = []
        interp_func_body.append('%s' % self.lineinfo.get())
        interp_func_body.append('static glv_trace_packet_header* interpret_trace_packet_vk(glv_trace_packet_header* pHeader)')
        interp_func_body.append('{')
        interp_func_body.append('    if (pHeader == NULL)')
        interp_func_body.append('    {')
        interp_func_body.append('        return NULL;')
        interp_func_body.append('    }')
        interp_func_body.append('    switch (pHeader->packet_id)')
        interp_func_body.append('    {')
        interp_func_body.append('        case GLV_TPI_VK_vkApiVersion:')
        interp_func_body.append('        {')
        interp_func_body.append('            return interpret_body_as_vkApiVersion(pHeader)->header;')
        interp_func_body.append('        }')
        for proto in self.protos:
            interp_func_body.append('        case GLV_TPI_VK_vk%s:\n        {' % proto.name)
            header_prefix = 'h'
            if 'WSI' in proto.name or 'Dbg' in proto.name:
                header_prefix = 'pH'
            interp_func_body.append('            return interpret_body_as_vk%s(pHeader)->%seader;\n        }' % (proto.name, header_prefix))
        interp_func_body.append('        default:')
        interp_func_body.append('            return NULL;')
        interp_func_body.append('    }')
        interp_func_body.append('    return NULL;')
        interp_func_body.append('}')
        return "\n".join(interp_func_body)

    def _generate_struct_util_funcs(self):
        lineinfo = self.lineinfo
        pid_enum = []
        pid_enum.append('%s' % lineinfo.get())
        pid_enum.append('//=============================================================================')
        pid_enum.append('static void add_VkApplicationInfo_to_packet(glv_trace_packet_header*  pHeader, VkApplicationInfo** ppStruct, const VkApplicationInfo *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(VkApplicationInfo), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pAppName), strlen(pInStruct->pAppName) + 1, pInStruct->pAppName);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pEngineName), strlen(pInStruct->pEngineName) + 1, pInStruct->pEngineName);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pAppName));')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pEngineName));')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&*ppStruct);')
        pid_enum.append('};\n')
        pid_enum.append('%s' % lineinfo.get())
        pid_enum.append('static void add_VkInstanceCreateInfo_to_packet(glv_trace_packet_header* pHeader, VkInstanceCreateInfo** ppStruct, VkInstanceCreateInfo *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(VkInstanceCreateInfo), pInStruct);')
        pid_enum.append('    add_VkApplicationInfo_to_packet(pHeader, (VkApplicationInfo**)&((*ppStruct)->pAppInfo), pInStruct->pAppInfo);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pAllocCb), sizeof(VkAllocCallbacks), pInStruct->pAllocCb);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pAllocCb));')
        # TODO138 : This is an initial pass at getting the extension/layer arrays correct, needs to be validated.
        pid_enum.append('    uint32_t i, siz = 0;')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->ppEnabledLayerNames), pInStruct->layerCount * sizeof(char*), pInStruct->ppEnabledLayerNames);')
        pid_enum.append('    if (pInStruct->layerCount > 0) ')
        pid_enum.append('    {')
        pid_enum.append('        for (i = 0; i < pInStruct->layerCount; i++) {')
        pid_enum.append('            siz = (1 + strlen(pInStruct->ppEnabledLayerNames[i]));')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledLayerNames[i]), siz, pInStruct->ppEnabledLayerNames[i]);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledLayerNames[i]);')
        pid_enum.append('        }')
        pid_enum.append('    }')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledLayerNames);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->ppEnabledExtensionNames), pInStruct->extensionCount * sizeof(char*), pInStruct->ppEnabledExtensionNames);')
        pid_enum.append('    if (pInStruct->extensionCount > 0) ')
        pid_enum.append('    {')
        pid_enum.append('        for (i = 0; i < pInStruct->extensionCount; i++) {')
        pid_enum.append('            siz = (1 + strlen(pInStruct->ppEnabledExtensionNames[i]));')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledExtensionNames[i]), siz, pInStruct->ppEnabledExtensionNames[i]);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames[i]);')
        pid_enum.append('        }')
        pid_enum.append('    }')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)ppStruct);')
        pid_enum.append('}\n')
        pid_enum.append('%s' % lineinfo.get())
        pid_enum.append('static void add_VkDeviceCreateInfo_to_packet(glv_trace_packet_header*  pHeader, VkDeviceCreateInfo** ppStruct, const VkDeviceCreateInfo *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(VkDeviceCreateInfo), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pRequestedQueues, pInStruct->queueRecordCount*sizeof(VkDeviceQueueCreateInfo), pInStruct->pRequestedQueues);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&(*ppStruct)->pRequestedQueues);')
        # TODO138 : This is an initial pass at getting the extension/layer arrays correct, needs to be validated.
        pid_enum.append('    uint32_t i, siz = 0;')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->ppEnabledLayerNames), pInStruct->layerCount * sizeof(char*), pInStruct->ppEnabledLayerNames);')
        pid_enum.append('    if (pInStruct->layerCount > 0) ')
        pid_enum.append('    {')
        pid_enum.append('        for (i = 0; i < pInStruct->layerCount; i++) {')
        pid_enum.append('            siz = (1 + strlen(pInStruct->ppEnabledLayerNames[i]));')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledLayerNames[i]), siz, pInStruct->ppEnabledLayerNames[i]);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledLayerNames[i]);')
        pid_enum.append('        }')
        pid_enum.append('    }')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledLayerNames);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->ppEnabledExtensionNames), pInStruct->extensionCount * sizeof(char*), pInStruct->ppEnabledExtensionNames);')
        pid_enum.append('    if (pInStruct->extensionCount > 0) ')
        pid_enum.append('    {')
        pid_enum.append('        for (i = 0; i < pInStruct->extensionCount; i++) {')
        pid_enum.append('            siz = (1 + strlen(pInStruct->ppEnabledExtensionNames[i]));')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledExtensionNames[i]), siz, pInStruct->ppEnabledExtensionNames[i]);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames[i]);')
        pid_enum.append('        }')
        pid_enum.append('    }')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pEnabledFeatures, sizeof(VkPhysicalDeviceFeatures), pInStruct->pEnabledFeatures);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&(*ppStruct)->pEnabledFeatures);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)ppStruct);')
        pid_enum.append('}\n')
        pid_enum.append('%s' % lineinfo.get())
        pid_enum.append('//=============================================================================\n')
        pid_enum.append('static VkInstanceCreateInfo* interpret_VkInstanceCreateInfo(glv_trace_packet_header*  pHeader, intptr_t ptr_variable)')
        pid_enum.append('{')
        pid_enum.append('    VkInstanceCreateInfo* pVkInstanceCreateInfo = (VkInstanceCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)ptr_variable);\n')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    if (pVkInstanceCreateInfo != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        pVkInstanceCreateInfo->pAppInfo = (VkApplicationInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->pAppInfo);')
        pid_enum.append('        pVkInstanceCreateInfo->pAllocCb = (VkAllocCallbacks*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->pAllocCb);')
        pid_enum.append('        VkApplicationInfo** ppAppInfo = (VkApplicationInfo**) &pVkInstanceCreateInfo->pAppInfo;')
        pid_enum.append('        (*ppAppInfo)->pAppName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->pAppInfo->pAppName);')
        pid_enum.append('        (*ppAppInfo)->pEngineName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->pAppInfo->pEngineName);')
        pid_enum.append('        if (pVkInstanceCreateInfo->layerCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            pVkInstanceCreateInfo->ppEnabledLayerNames = (const char* const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->ppEnabledLayerNames);')
        pid_enum.append('            for (i = 0; i < pVkInstanceCreateInfo->layerCount; i++) {')
        pid_enum.append('                char** ppTmp = (char**)&pVkInstanceCreateInfo->ppEnabledLayerNames[i];')
        pid_enum.append('                *ppTmp = (char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->ppEnabledLayerNames[i]);')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('        if (pVkInstanceCreateInfo->extensionCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            pVkInstanceCreateInfo->ppEnabledExtensionNames = (const char* const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->ppEnabledExtensionNames);')
        pid_enum.append('            for (i = 0; i < pVkInstanceCreateInfo->extensionCount; i++) {')
        pid_enum.append('                char** ppTmp = (char**)&pVkInstanceCreateInfo->ppEnabledExtensionNames[i];')
        pid_enum.append('                *ppTmp = (char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkInstanceCreateInfo->ppEnabledExtensionNames[i]);')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('    }\n')
        pid_enum.append('    return pVkInstanceCreateInfo;')
        pid_enum.append('}\n')
        pid_enum.append('%s' % lineinfo.get())
        pid_enum.append('static VkDeviceCreateInfo* interpret_VkDeviceCreateInfo(glv_trace_packet_header*  pHeader, intptr_t ptr_variable)')
        pid_enum.append('{')
        pid_enum.append('    VkDeviceCreateInfo* pVkDeviceCreateInfo = (VkDeviceCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)ptr_variable);\n')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    if (pVkDeviceCreateInfo != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        pVkDeviceCreateInfo->pRequestedQueues = (const VkDeviceQueueCreateInfo *)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->pRequestedQueues);\n')
        pid_enum.append('        if (pVkDeviceCreateInfo->layerCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            pVkDeviceCreateInfo->ppEnabledLayerNames = (const char* const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->ppEnabledLayerNames);')
        pid_enum.append('            for (i = 0; i < pVkDeviceCreateInfo->layerCount; i++) {')
        pid_enum.append('                char** ppTmp = (char**)&pVkDeviceCreateInfo->ppEnabledLayerNames[i];')
        pid_enum.append('                *ppTmp = (char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->ppEnabledLayerNames[i]);')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('        if (pVkDeviceCreateInfo->extensionCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            pVkDeviceCreateInfo->ppEnabledExtensionNames = (const char* const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->ppEnabledExtensionNames);')
        pid_enum.append('            for (i = 0; i < pVkDeviceCreateInfo->extensionCount; i++) {')
        pid_enum.append('                char** ppTmp = (char**)&pVkDeviceCreateInfo->ppEnabledExtensionNames[i];')
        pid_enum.append('                *ppTmp = (char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->ppEnabledExtensionNames[i]);')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('        pVkDeviceCreateInfo->pEnabledFeatures = (const VkPhysicalDeviceFeatures*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->pEnabledFeatures);\n')
        pid_enum.append('    }\n')
        pid_enum.append('    return pVkDeviceCreateInfo;')
        pid_enum.append('}\n')
        pid_enum.append('%s' % lineinfo.get())
        pid_enum.append('static void interpret_VkPipelineShaderStageCreateInfo(glv_trace_packet_header*  pHeader, VkPipelineShaderStageCreateInfo* pShader)')
        pid_enum.append('{')
        pid_enum.append('    if (pShader != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        // specialization info')
        pid_enum.append('        pShader->pSpecializationInfo = (const VkSpecializationInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pSpecializationInfo);')
        pid_enum.append('        if (pShader->pSpecializationInfo != NULL)')
        pid_enum.append('        {')
        pid_enum.append('            VkSpecializationInfo* pInfo = (VkSpecializationInfo*)pShader->pSpecializationInfo;')
        pid_enum.append('            pInfo->pMap = (const VkSpecializationMapEntry*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pSpecializationInfo->pMap);')
        pid_enum.append('            pInfo->pData = (const void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pSpecializationInfo->pData);')
        pid_enum.append('        }')
        pid_enum.append('    }')
        pid_enum.append('}\n')
        pid_enum.append('//=============================================================================')
        return "\n".join(pid_enum)

    # Interpret functions used on replay to read in packets and interpret their contents
    #  This code gets generated into vktrace_vk_vk_packets.h file
    def _generate_interp_funcs(self):
        # Custom txt for given function and parameter.  First check if param is NULL, then insert txt if not
        # First some common code used by both CmdWaitEvents & CmdPipelineBarrier
        mem_barrier_interp = ['uint32_t i = 0;\n',
                              'for (i = 0; i < pPacket->memBarrierCount; i++) {\n',
                              '    void** ppMB = (void**)&(pPacket->ppMemBarriers[i]);\n',
                              '    *ppMB = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->ppMemBarriers[i]);\n',
                              '    //VkMemoryBarrier* pBarr = (VkMemoryBarrier*)pPacket->ppMemBarriers[i];\n',
                              '    // TODO : Could fix up the pNext ptrs here if they were finalized and if we cared by switching on Barrier type and remapping\n',
                              '}']
        create_rp_interp = ['VkRenderPassCreateInfo* pInfo = (VkRenderPassCreateInfo*)pPacket->pCreateInfo;\n',
                            'uint32_t i = 0;\n',
                            'VkAttachmentDescription **ppAD = (VkAttachmentDescription **)&(pInfo->pAttachments);\n',
                            '*ppAD = (VkAttachmentDescription*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->pAttachments);\n',
                            'VkSubpassDescription** ppSP = (VkSubpassDescription**)&(pInfo->pSubpasses);\n',
                            '*ppSP = (VkSubpassDescription*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->pSubpasses);\n',
                            'for (i=0; i<pInfo->subpassCount; i++) {\n',
                            '    VkAttachmentReference** pAR = (VkAttachmentReference**)&(pInfo->pSubpasses[i].pInputAttachments);\n',
                            '    *pAR = (VkAttachmentReference*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->pSubpasses[i].pInputAttachments);\n',
                            '    pAR = (VkAttachmentReference**)&(pInfo->pSubpasses[i].pColorAttachments);\n',
                            '    *pAR = (VkAttachmentReference*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->pSubpasses[i].pColorAttachments);\n',
                            '    pAR = (VkAttachmentReference**)&(pInfo->pSubpasses[i].pResolveAttachments);\n',
                            '    *pAR = (VkAttachmentReference*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->pSubpasses[i].pResolveAttachments);\n',
                            '    pAR = (VkAttachmentReference**)&(pInfo->pSubpasses[i].pPreserveAttachments);\n',
                            '    *pAR = (VkAttachmentReference*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->pSubpasses[i].pPreserveAttachments);\n',
                            '}\n',
                            'VkSubpassDependency** ppSD = (VkSubpassDependency**)&(pInfo->pDependencies);\n',
                            '*ppSD = (VkSubpassDependency*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->pDependencies);\n']
        create_gfx_pipe = ['uint32_t i;\n',
                           'uint32_t j;\n',
                           'for (i=0; i<pPacket->count; i++) {\n',
                            'if (pPacket->pCreateInfos[i].sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {\n',
                            '// need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                            'VkGraphicsPipelineCreateInfo* pNonConst = (VkGraphicsPipelineCreateInfo*)&(pPacket->pCreateInfos[i]);\n',
                            '// shader stages array\n',
                            'pNonConst->pStages = (VkPipelineShaderStageCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pStages);\n',
                            'for (j = 0; j < pPacket->pCreateInfos[i].stageCount; j++)\n',
                            '{\n',
                            '    interpret_VkPipelineShaderStageCreateInfo(pHeader, (VkPipelineShaderStageCreateInfo*)&pPacket->pCreateInfos[i].pStages[j]);\n',
                            '}\n',
                            '// Vertex Input State\n',
                            'pNonConst->pVertexInputState = (VkPipelineVertexInputStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pVertexInputState);\n',
                            'VkPipelineVertexInputStateCreateInfo* pNonConstVIState = (VkPipelineVertexInputStateCreateInfo*)pNonConst->pVertexInputState;\n',
                            'if (pNonConstVIState) {\n',
                            '    pNonConstVIState->pVertexBindingDescriptions = (const VkVertexInputBindingDescription*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pVertexInputState->pVertexBindingDescriptions);\n',
                            '    pNonConstVIState->pVertexAttributeDescriptions = (const VkVertexInputAttributeDescription*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pVertexInputState->pVertexAttributeDescriptions);\n',
                            '}\n',
                            '// Input Assembly State\n',
                            'pNonConst->pInputAssemblyState = (const VkPipelineInputAssemblyStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pInputAssemblyState);\n',
                            '// Tesselation State\n',
                            'pNonConst->pTessellationState = (const VkPipelineTessellationStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pTessellationState);\n',
                            '// Viewport State\n',
                            'pNonConst->pViewportState = (const VkPipelineViewportStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pViewportState);\n',
                            '// Raster State\n',
                            'pNonConst->pRasterState = (const VkPipelineRasterStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pRasterState);\n',
                            '// MultiSample State\n',
                            'pNonConst->pMultisampleState = (const VkPipelineMultisampleStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pMultisampleState);\n',
                            '// DepthStencil State\n',
                            'pNonConst->pDepthStencilState = (const VkPipelineDepthStencilStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pDepthStencilState);\n',
                            '// ColorBuffer State\n',
                            'pNonConst->pColorBlendState = (const VkPipelineColorBlendStateCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pColorBlendState);\n',
                            'VkPipelineColorBlendStateCreateInfo* pNonConstCbState = (VkPipelineColorBlendStateCreateInfo*)pNonConst->pColorBlendState;\n',
                            'if (pNonConstCbState)\n',
                            '    pNonConstCbState->pAttachments = (const VkPipelineColorBlendAttachmentState*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfos[i].pColorBlendState->pAttachments);\n',
                            '} else {\n',
                            '    // This is unexpected.\n',
                            '    glv_LogError("CreateGraphicsPipelines must have CreateInfo stype of VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO.");\n',
                            '    pPacket->header = NULL;\n',
                            '}\n',
                            '}\n']
        # TODO : This code is now too large and complex, need to make codegen smarter for pointers embedded in struct params to handle those cases automatically
                              # TODO138 : Just ripped out a bunch of custom code here that was out of date. Need to scrub these function and verify they're correct
        custom_case_dict = { #'CreateShader' : {'param': 'pCreateInfo', 'txt': ['VkShaderCreateInfo* pInfo = (VkShaderCreateInfo*)pPacket->pCreateInfo;\n',
                              #                 'pInfo->pCode = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pCode);']},
                             'CreateDynamicViewportState' : {'param': 'pCreateInfo', 'txt': ['VkDynamicViewportStateCreateInfo* pInfo = (VkDynamicViewportStateCreateInfo*)pPacket->pCreateInfo;\n',
                                                                                             'pInfo->pViewports = (VkViewport*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pViewports);\n',
                                                                                             'pInfo->pScissors = (VkRect2D*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pScissors);']},
                             #'CreateFramebuffer' : {'param': 'pCreateInfo', 'txt': ['VkFramebufferCreateInfo* pInfo = (VkFramebufferCreateInfo*)pPacket->pCreateInfo;\n',
                              #                      'pInfo->pColorAttachments = (VkColorAttachmentBindInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorAttachments);\n',
                               #                     'pInfo->pDepthStencilAttachment = (VkDepthStencilBindInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pDepthStencilAttachment);\n']},
                             'CreateRenderPass' : {'param': 'pCreateInfo', 'txt': create_rp_interp},
                             'CreatePipelineLayout' : {'param': 'pCreateInfo', 'txt': ['VkPipelineLayoutCreateInfo* pInfo = (VkPipelineLayoutCreateInfo*)pPacket->pCreateInfo;\n',
                                                       'pInfo->pSetLayouts = (VkDescriptorSetLayout*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pSetLayouts);\n']},
                             'CreateDescriptorPool' : {'param': 'pCreateInfo', 'txt': ['VkDescriptorPoolCreateInfo* pInfo = (VkDescriptorPoolCreateInfo*)pPacket->pCreateInfo;\n',
                                                       'pInfo->pTypeCount = (VkDescriptorTypeCount*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pTypeCount);\n']},
                             'CmdWaitEvents' : {'param': 'ppMemBarriers', 'txt': mem_barrier_interp},
                             'CmdPipelineBarrier' : {'param': 'ppMemBarriers', 'txt': mem_barrier_interp},
                             'CreateDescriptorSetLayout' : {'param': 'pCreateInfo', 'txt': ['if (pPacket->pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {\n',
                                                                                         '    VkDescriptorSetLayoutCreateInfo* pNext = (VkDescriptorSetLayoutCreateInfo*)pPacket->pCreateInfo;\n',
                                                                                         '    do\n','    {\n',
                                                                                         '        // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         '        void** ppNextVoidPtr = (void**)&(pNext->pNext);\n',
                                                                                         '        *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:\n',
                                                                                         '            {\n' ,
                                                                                         '                unsigned int i = 0;\n',
                                                                                         '                pNext->pBinding = (VkDescriptorSetLayoutBinding*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pBinding);\n',
                                                                                         '                for (i = 0; i < pNext->count; i++)\n','                {\n',
                                                                                         '                    VkSampler** ppSamplers = (VkSampler**)&(pNext->pBinding[i].pImmutableSamplers);\n',
                                                                                         '                    *ppSamplers = (VkSampler*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pBinding[i].pImmutableSamplers);\n',
                                                                                         '                }\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '                glv_LogError("Encountered an unexpected type in descriptor set layout create list.");\n',
                                                                                         '                pPacket->header = NULL;\n',
                                                                                         '                pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (VkDescriptorSetLayoutCreateInfo*)pNext->pNext;\n',
                                                                                         '     }  while (NULL != pNext);\n',
                                                                                         '} else {\n',
                                                                                         '     // This is unexpected.\n',
                                                                                         '     glv_LogError("CreateDescriptorSetLayout must have pCreateInfo->stype of VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO.");\n',
                                                                                         '     pPacket->header = NULL;\n',
                                                                                         '}']},
#                             'BeginCommandBuffer' : {'param': 'pBeginInfo', 'txt': ['if (pPacket->pBeginInfo->sType == VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO) {\n',
#                                                                                         '    // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
#                                                                                         '    VkCmdBufferGraphicsBeginInfo** ppNext = (VkCmdBufferGraphicsBeginInfo**)&(pPacket->pBeginInfo->pNext);\n',
#                                                                                         '    *ppNext = (VkCmdBufferGraphicsBeginInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBeginInfo->pNext);\n',
#                                                                                         '    VkCmdBufferGraphicsBeginInfo* pNext = *ppNext;\n',
#                                                                                         '    while (NULL != pNext)\n', '    {\n',
#                                                                                         '        switch(pNext->sType)\n', '        {\n',
#                                                                                         '            case VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:\n',
#                                                                                         '            {\n',
#                                                                                         '                ppNext = (VkCmdBufferGraphicsBeginInfo**) &pNext->pNext;\n',
#                                                                                         '                *ppNext = (VkCmdBufferGraphicsBeginInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
#                                                                                         '                break;\n',
#                                                                                         '            }\n',
#                                                                                         '            default:\n',
#                                                                                         '            {\n',
#                                                                                         '                glv_LogError("Encountered an unexpected type in begin command buffer list.");\n',
#                                                                                         '                pPacket->header = NULL;\n',
#                                                                                         '                pNext->pNext = NULL;\n',
#                                                                                         '            }\n',
#                                                                                         '        }\n',
#                                                                                         '        pNext = (VkCmdBufferGraphicsBeginInfo*)pNext->pNext;\n',
#                                                                                         '    }\n',
#                                                                                         '} else {\n',
#                                                                                         '    // This is unexpected.\n',
#                                                                                         '    glv_LogError("BeginCommandBuffer must have BeginInfo stype of VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO.");\n',
#                                                                                         '    pPacket->header = NULL;\n',
#                                                                                         '}']},
                             'AllocMemory' : {'param': 'pAllocInfo', 'txt': ['if (pPacket->pAllocInfo->sType == VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO) {\n',
                                                                                         '    VkMemoryAllocInfo** ppNext = (VkMemoryAllocInfo**) &(pPacket->pAllocInfo->pNext);\n',
                                                                                         '    *ppNext = (VkMemoryAllocInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocInfo->pNext);\n',
                                                                                         '    VkMemoryAllocInfo* pNext = (VkMemoryAllocInfo*) *ppNext;\n',
                                                                                         '    while (NULL != pNext)\n', '    {\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '               glv_LogError("Encountered an unexpected type alloc memory list.");\n',
                                                                                         '               pPacket->header = NULL;\n',
                                                                                         '               pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (VkMemoryAllocInfo*)pNext->pNext;\n',
                                                                                         '    }\n',
                                                                                         '} else {\n',
                                                                                         '    // This is unexpected.\n',
                                                                                         '    glv_LogError("AllocMemory must have AllocInfo stype of VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO.");\n',
                                                                                         '    pPacket->header = NULL;\n',
                                                                                         '}']},
                             'UpdateDescriptorSets' : {'param': 'pDescriptorWrites', 'txt':
                                                                               [ 'uint32_t i;\n',
                                                                                 'for (i = 0; i < pPacket->writeCount; i++) {\n',
                                                                                 '   VkDescriptorInfo** ppDescriptors = (VkDescriptorInfo**)&pPacket->pDescriptorWrites[i].pDescriptors;\n',
                                                                                 '   *ppDescriptors = (VkDescriptorInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDescriptorWrites[i].pDescriptors);\n',
                                                                                 '}'
                                                                               ]},
                             'CreateGraphicsPipelines' : {'param': 'pCreateInfos', 'txt': create_gfx_pipe},
                             'CreateComputePipeline' : {'param': 'pCreateInfo', 'txt': ['interpret_VkPipelineShaderStageCreateInfo(pHeader, (VkPipelineShaderStageCreateInfo*)(&pPacket->pCreateInfo->cs));']},
                             'CreateFramebuffer' : {'param': 'pCreateInfo', 'txt': ['VkAttachmentView** ppAV = (VkAttachmentView**)&(pPacket->pCreateInfo->pAttachments);\n',
                                                                                    '*ppAV = (VkAttachmentView*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pPacket->pCreateInfo->pAttachments));']},
                             'CmdBeginRenderPass' : {'param': 'pRenderPassBegin', 'txt': ['VkClearValue** ppCV = (VkClearValue**)&(pPacket->pRenderPassBegin->pClearValues);\n',
                                                                                          '*ppCV = (VkClearValue*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pPacket->pRenderPassBegin->pClearValues));']},
                             'CreateShaderModule' : {'param': 'pCreateInfo', 'txt': ['void** ppCode = (void**)&(pPacket->pCreateInfo->pCode);\n',
                                                                                     '*ppCode = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pCode);']},
                             'CreateShader' : {'param': 'pCreateInfo', 'txt': ['void** ppName = (void**)&(pPacket->pCreateInfo->pName);\n',
                                                                               '*ppName = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pName);']},
                             'FlushMappedMemoryRanges' : {'param': 'ppData', 'txt': ['uint32_t i = 0;\n',
                                                                                     'for (i = 0; i < pPacket->memRangeCount; i++)\n',
                                                                                     '{\n',
                                                                                     '    pPacket->ppData[i] = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->ppData[i]);\n',
                                                                                     '}']}}
        if_body = []
        if_body.append('typedef struct packet_vkApiVersion {')
        if_body.append('    glv_trace_packet_header* header;')
        if_body.append('    uint32_t version;')
        if_body.append('} packet_vkApiVersion;\n')
        if_body.append('static packet_vkApiVersion* interpret_body_as_vkApiVersion(glv_trace_packet_header* pHeader)')
        if_body.append('{')
        if_body.append('    packet_vkApiVersion* pPacket = (packet_vkApiVersion*)pHeader->pBody;')
        if_body.append('    pPacket->header = pHeader;')
        if_body.append('    return pPacket;')
        if_body.append('}\n')
        for proto in self.protos:
            if 'WSI' not in proto.name and 'Dbg' not in proto.name:
                if 'UnmapMemory' == proto.name:
                    proto.params.append(vulkan.Param("void*", "pData"))
                elif 'FlushMappedMemoryRanges' == proto.name:
                    proto.params.append(vulkan.Param("void**", "ppData"))
                if_body.append('%s' % self.lineinfo.get())
                if_body.append('typedef struct packet_vk%s {' % proto.name)
                if_body.append('    glv_trace_packet_header* header;')
                for p in proto.params:
                    if_body.append('    %s %s;' % (p.ty, p.name))
                if 'void' != proto.ret:
                    if_body.append('    %s result;' % proto.ret)
                if_body.append('} packet_vk%s;\n' % proto.name)
                if_body.append('static packet_vk%s* interpret_body_as_vk%s(glv_trace_packet_header* pHeader)' % (proto.name, proto.name))
                if_body.append('{')
                if_body.append('    packet_vk%s* pPacket = (packet_vk%s*)pHeader->pBody;' % (proto.name, proto.name))
                if_body.append('    pPacket->header = pHeader;')
                for p in proto.params:
                    if '*' in p.ty:
                        if 'DeviceCreateInfo' in p.ty:
                            if_body.append('    pPacket->%s = interpret_VkDeviceCreateInfo(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.name))
                        elif 'InstanceCreateInfo' in p.ty:
                            if_body.append('    pPacket->%s = interpret_VkInstanceCreateInfo(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.name))
                        else:
                            if_body.append('    pPacket->%s = (%s)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.ty, p.name))
                        # TODO : Generalize this custom code to kill dict data struct above.
                        #  Really the point of this block is to catch params w/ embedded ptrs to structs and chains of structs
                        if proto.name in custom_case_dict and p.name == custom_case_dict[proto.name]['param']:
                            if_body.append('    if (pPacket->%s != NULL)' % custom_case_dict[proto.name]['param'])
                            if_body.append('    {')
                            if_body.append('        %s' % "        ".join(custom_case_dict[proto.name]['txt']))
                            if_body.append('    }')
                if_body.append('    return pPacket;')
                if_body.append('}\n')
        return "\n".join(if_body)

    def _generate_interp_funcs_ext(self, extName):
        if_body = []
        custom_case_dict = { 'QueuePresentWSI' : {'param': 'pPresentInfo', 'txt': ['pPacket->pPresentInfo->swapChains = (VkSwapChainWSI*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pPacket->pPresentInfo->swapChains));\n',
                                                                                   'pPacket->pPresentInfo->imageIndices = (uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pPacket->pPresentInfo->imageIndices));']},
                             'CreateSwapChainWSI' : {'param': 'pCreateInfo', 'txt': ['VkSurfaceDescriptionWSI **ppSurfDescp = (VkSurfaceDescriptionWSI**)&pPacket->pCreateInfo->pSurfaceDescription;\n',
                                                     'uint32_t **ppQFI = (uint32_t**)&pPacket->pCreateInfo->pQueueFamilyIndices;\n',
                                                     '(*ppSurfDescp) = (VkSurfaceDescriptionWSI*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pPacket->pCreateInfo->pSurfaceDescription));\n',
                                                     '(*ppQFI) = (uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pPacket->pCreateInfo->pQueueFamilyIndices));']},
                            }
        for ext in vulkan.extensions_all:
            if ext.name.lower() == extName.lower():
                for proto in ext.protos:
                    if_body.append('typedef struct packet_vk%s {' % proto.name)
                    if_body.append('    glv_trace_packet_header* pHeader;')
                    for p in proto.params:
                        if_body.append('    %s %s;' % (p.ty, p.name))
                    if 'void' != proto.ret:
                        if_body.append('    %s result;' % proto.ret)
                    if_body.append('} packet_vk%s;\n' % proto.name)
                    if_body.append('static packet_vk%s* interpret_body_as_vk%s(glv_trace_packet_header* pHeader)' % (proto.name, proto.name))
                    if_body.append('{')
                    if_body.append('    packet_vk%s* pPacket = (packet_vk%s*)pHeader->pBody;' % (proto.name, proto.name))
                    if_body.append('    pPacket->pHeader = pHeader;')
                    for p in proto.params:
                        if '*' in p.ty:
                            if_body.append('    pPacket->%s = (%s)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.ty, p.name))
                            # TODO : Generalize this custom code to kill dict data struct above.
                            #  Really the point of this block is to catch params w/ embedded ptrs to structs and chains of structs
                            if proto.name in custom_case_dict and p.name == custom_case_dict[proto.name]['param']:
                                if_body.append('    if (pPacket->%s != NULL)' % custom_case_dict[proto.name]['param'])
                                if_body.append('    {')
                                if_body.append('        %s' % "        ".join(custom_case_dict[proto.name]['txt']))
                                if_body.append('    }')
                    if_body.append('    return pPacket;')
                    if_body.append('}\n')
        return "\n".join(if_body)

    def _generate_replay_func_ptrs(self):
        xf_body = []
        xf_body.append('struct vkFuncs {')
        xf_body.append('    void init_funcs(void * libHandle);')
        xf_body.append('    void *m_libHandle;\n')
        for proto in self.protos:
            xf_body.append('    typedef %s( VKAPI * type_vk%s)(' % (proto.ret, proto.name))
            for p in proto.params:
                xf_body.append('        %s %s,' % (p.ty, p.name))
            xf_body[-1] = xf_body[-1].replace(',', ');')
            xf_body.append('    type_vk%s real_vk%s;' % (proto.name, proto.name))
        xf_body.append('};')
        return "\n".join(xf_body)

    def _map_decl(self, type1, type2, name):
        return '    std::map<%s, %s> %s;' % (type1, type2, name)

    def _add_to_map_decl(self, type1, type2, name):
        txt = '    void add_to_%s_map(%s pTraceVal, %s pReplayVal)\n    {\n' % (name[2:], type1, type2)
        #TODO138 : These checks need to vary between disp & non-disp objects
        #txt += '        assert(pTraceVal != 0);\n'
        #txt += '        assert(pReplayVal != 0);\n'
        txt += '        %s[pTraceVal] = pReplayVal;\n    }\n' % name
        return txt

    def _rm_from_map_decl(self, ty, name):
        txt = '    void rm_from_%s_map(const %s& key)\n    {\n' % (name[2:], ty)
        txt += '        %s.erase(key);\n    }\n' % name
        return txt

    def _remap_decl(self, ty, name):
        txt = '    %s remap_%s(const %s& value)\n    {\n' % (ty, name[2:], ty)
        txt += '        if (value == 0) { return 0; }\n'
        txt += '        std::map<%s, %s>::const_iterator q = %s.find(value);\n' % (ty, ty, name)
        txt += '        if (q == %s.end()) { glv_LogError("Failed to remap %s."); return value; }\n' % (name, ty)
        txt += '        return q->second;\n    }\n'
        return txt

    def _generate_replay_objMemory_funcs(self):
        rof_body = []
        # Custom code for memory mapping functions for app writes into mapped memory
        rof_body.append('// memory mapping functions for app writes into mapped memory')
        rof_body.append('    bool isPendingAlloc()')
        rof_body.append('    {')
        rof_body.append('        return m_pendingAlloc;')
        rof_body.append('    }')
        rof_body.append('')
        rof_body.append('    void setAllocInfo(const VkMemoryAllocInfo *info, const bool pending)')
        rof_body.append('    {')
        rof_body.append('        m_pendingAlloc = pending;')
        rof_body.append('        m_allocInfo = *info;')
        rof_body.append('    }')
        rof_body.append('')
        rof_body.append('    void setMemoryDataAddr(void *pBuf)')
        rof_body.append('    {')
        rof_body.append('        if (m_mapRange.empty())')
        rof_body.append('        {')
        rof_body.append('            glv_LogError("gpuMemory::setMemoryDataAddr() m_mapRange is empty.");')
        rof_body.append('            return;')
        rof_body.append('        }')
        rof_body.append('        MapRange mr = m_mapRange.back();')
        rof_body.append('        if (mr.pData != NULL)')
        rof_body.append('            glv_LogWarning("gpuMemory::setMemoryDataAddr() data already mapped overwrite old mapping.");')
        rof_body.append('        else if (pBuf == NULL)')
        rof_body.append('            glv_LogWarning("gpuMemory::setMemoryDataAddr() adding NULL pointer.");')
        rof_body.append('        mr.pData = (uint8_t *) pBuf;')
        rof_body.append('    }')
        rof_body.append('')
        rof_body.append('    void setMemoryMapRange(void *pBuf, const size_t size, const size_t offset, const bool pending)')
        rof_body.append('    {')
        rof_body.append('        MapRange mr;')
        rof_body.append('        mr.pData = (uint8_t *) pBuf;')
        rof_body.append('        if (size == 0)')
        rof_body.append('            mr.size = m_allocInfo.allocationSize - offset;')
        rof_body.append('        else')
        rof_body.append('            mr.size = size;')
        rof_body.append('        mr.offset = offset;')
        rof_body.append('        mr.pending = pending;')
        rof_body.append('        m_mapRange.push_back(mr);')
        rof_body.append('        assert(m_allocInfo.allocationSize >= (size + offset));')
        rof_body.append('    }')
        rof_body.append('')
        rof_body.append('    void copyMappingData(const void* pSrcData, bool entire_map, size_t size, size_t offset)')
        rof_body.append('    {')
        rof_body.append('        if (m_mapRange.empty())')
        rof_body.append('        {')
        rof_body.append('            glv_LogError("gpuMemory::copyMappingData() m_mapRange is empty.");')
        rof_body.append('            return;')
        rof_body.append('        }')
        rof_body.append('        MapRange mr = m_mapRange.back();')
        rof_body.append('        if (!pSrcData || !mr.pData)')
        rof_body.append('        {')
        rof_body.append('            if (!pSrcData)')
        rof_body.append('                glv_LogError("gpuMemory::copyMappingData() null src pointer.");')
        rof_body.append('            else')
        rof_body.append('                glv_LogError("gpuMemory::copyMappingData() null dest pointer totalSize=%u.", m_allocInfo.allocationSize);')
        rof_body.append('            m_mapRange.pop_back();')
        rof_body.append('            return;')
        rof_body.append('        }')
        rof_body.append('        if (entire_map)')
        rof_body.append('        {')
        rof_body.append('            size = mr.size;')
        rof_body.append('            offset = mr.offset;')
        rof_body.append('        }')
        rof_body.append('        else')
        rof_body.append('        {')
        rof_body.append('            assert(offset >= mr.offset);')
        rof_body.append('            assert(size <= mr.size && (size + offset) <= mr.size);')
        rof_body.append('        }')
        rof_body.append('        memcpy(mr.pData + offset, pSrcData, size);')
        rof_body.append('        if (!mr.pending && entire_map)')
        rof_body.append('            m_mapRange.pop_back();')
        rof_body.append('    }')
        rof_body.append('')
        rof_body.append('    size_t getMemoryMapSize()')
        rof_body.append('    {')
        rof_body.append('        return (!m_mapRange.empty()) ? m_mapRange.back().size : 0;')
        rof_body.append('    }\n')
        return "\n".join(rof_body)

    def _generate_replay_objmapper_class(self):
        # Create dict mapping member var names to VK type (i.e. 'm_imageViews' : 'VkImage_VIEW')
        obj_map_dict = {}
        for obj in vulkan.object_type_list:
            if (obj.startswith('Vk')):
                mem_var = obj.replace('Vk', '').lower()
            mem_var_list = mem_var.split('_')
            mem_var = 'm_%s%ss' % (mem_var_list[0], "".join([m.title() for m in mem_var_list[1:]]))
            obj_map_dict[mem_var] = obj
        rc_body = []
        rc_body.append('#define GLV_VK_OBJECT_TYPE_UNKNOWN (VkObjectType)-1')
        rc_body.append('')
        rc_body.append('typedef struct _VKAllocInfo {')
        rc_body.append('    VkDeviceSize size;')
        rc_body.append('    uint8_t *pData;')
        rc_body.append('    bool rangeUpdated;')
        rc_body.append('} VKAllocInfo;')
        rc_body.append('')
        rc_body.append('class objMemory {')
        rc_body.append('public:')
        rc_body.append('    objMemory() : m_numAllocations(0), m_pMemReqs(NULL) {}')
        rc_body.append('    ~objMemory() { free(m_pMemReqs);}')
        rc_body.append('    void setCount(const uint32_t num)')
        rc_body.append('    {')
        rc_body.append('        m_numAllocations = num;')
        rc_body.append('    }\n')
        rc_body.append('    void setReqs(const VkMemoryRequirements *pReqs, const uint32_t num)')
        rc_body.append('    {')
        rc_body.append('        if (m_numAllocations != num && m_numAllocations != 0)')
        rc_body.append('            glv_LogError("objMemory::setReqs, internal mismatch on number of allocations.");')
        rc_body.append('        if (m_pMemReqs == NULL && pReqs != NULL)')
        rc_body.append('        {')
        rc_body.append('            m_pMemReqs = (VkMemoryRequirements *) glv_malloc(num * sizeof(VkMemoryRequirements));')
        rc_body.append('            if (m_pMemReqs == NULL)')
        rc_body.append('            {')
        rc_body.append('                glv_LogError("objMemory::setReqs out of memory.");')
        rc_body.append('                return;')
        rc_body.append('            }')
        rc_body.append('            memcpy(m_pMemReqs, pReqs, num * sizeof(VkMemoryRequirements));')
        rc_body.append('        }')
        rc_body.append('    }\n')
        rc_body.append('private:')
        rc_body.append('    uint32_t m_numAllocations;')
        rc_body.append('    VkMemoryRequirements *m_pMemReqs;')
        rc_body.append('};')
        rc_body.append('')
        rc_body.append('class gpuMemory {')
        rc_body.append('public:')
        rc_body.append('    gpuMemory() : m_pendingAlloc(false) {m_allocInfo.allocationSize = 0;}')
        rc_body.append('    ~gpuMemory() {}')
        rc_body.append(self._generate_replay_objMemory_funcs())
        rc_body.append('private:')
        rc_body.append('    bool m_pendingAlloc;')
        rc_body.append('    struct MapRange {')
        rc_body.append('        bool pending;')
        rc_body.append('        size_t size;')
        rc_body.append('        size_t offset;')
        rc_body.append('        uint8_t* pData;')
        rc_body.append('    };')
        rc_body.append('    std::vector<MapRange> m_mapRange;')
        rc_body.append('    VkMemoryAllocInfo m_allocInfo;')
        rc_body.append('};')
        rc_body.append('')
        rc_body.append('typedef struct _imageObj {')
        rc_body.append('     objMemory imageMem;')
        rc_body.append('     VkImage replayImage;')
        rc_body.append(' } imageObj;')
        rc_body.append('')
        rc_body.append('typedef struct _bufferObj {')
        rc_body.append('     objMemory bufferMem;')
        rc_body.append('     VkBuffer replayBuffer;')
        rc_body.append(' } bufferObj;')
        rc_body.append('')
        rc_body.append('typedef struct _gpuMemObj {')
        rc_body.append('     gpuMemory *pGpuMem;')
        rc_body.append('     VkDeviceMemory replayGpuMem;')
        rc_body.append(' } gpuMemObj;')
        rc_body.append('')
        rc_body.append('')
        rc_body.append('class vkReplayObjMapper {')
        rc_body.append('public:')
        rc_body.append('    vkReplayObjMapper() {}')
        rc_body.append('    ~vkReplayObjMapper() {}')
        rc_body.append('')
        rc_body.append(' bool m_adjustForGPU; // true if replay adjusts behavior based on GPU')
        # Code for memory objects for handling replay GPU != trace GPU object memory requirements
        rc_body.append('void init_objMemCount(const uint64_t handle, const VkDbgObjectType objectType, const uint32_t &num)\n {')
        rc_body.append('    switch (objectType) {')
        rc_body.append('        case VK_OBJECT_TYPE_BUFFER:')
        rc_body.append('        {')
        rc_body.append('            std::map<uint64_t, bufferObj>::iterator it = m_buffers.find(handle);')
        rc_body.append('            if (it != m_buffers.end()) {')
        rc_body.append('                objMemory obj = it->second.bufferMem;')
        rc_body.append('                obj.setCount(num);')
        rc_body.append('                return;')
        rc_body.append('            }')
        rc_body.append('            break;')
        rc_body.append('        }')
        rc_body.append('        case VK_OBJECT_TYPE_IMAGE:')
        rc_body.append('        {')
        rc_body.append('            std::map<uint64_t, imageObj>::iterator it = m_images.find(handle);')
        rc_body.append('            if (it != m_images.end()) {')
        rc_body.append('                objMemory obj = it->second.imageMem;')
        rc_body.append('                obj.setCount(num);')
        rc_body.append('                return;')
        rc_body.append('            }')
        rc_body.append('            break;')
        rc_body.append('        }')
        rc_body.append('        default:')
        rc_body.append('            break;')
        rc_body.append('    }')
        rc_body.append('    return;')
        rc_body.append('}\n')
        rc_body.append('void init_objMemReqs(const uint64_t handle, const VkDbgObjectType objectType, const VkMemoryRequirements *pMemReqs, const unsigned int num)\n    {')
        rc_body.append('    switch (objectType) {')
        rc_body.append('        case VK_OBJECT_TYPE_BUFFER:')
        rc_body.append('        {')
        rc_body.append('            std::map<uint64_t, bufferObj>::iterator it = m_buffers.find(handle);')
        rc_body.append('            if (it != m_buffers.end()) {')
        rc_body.append('                objMemory obj = it->second.bufferMem;')
        rc_body.append('                obj.setReqs(pMemReqs, num);')
        rc_body.append('                return;')
        rc_body.append('            }')
        rc_body.append('            break;')
        rc_body.append('        }')
        rc_body.append('        case VK_OBJECT_TYPE_IMAGE:')
        rc_body.append('        {')
        rc_body.append('            std::map<uint64_t, imageObj>::iterator it = m_images.find(handle);')
        rc_body.append('            if (it != m_images.end()) {')
        rc_body.append('                objMemory obj = it->second.imageMem;')
        rc_body.append('                obj.setReqs(pMemReqs, num);')
        rc_body.append('                return;')
        rc_body.append('            }')
        rc_body.append('            break;')
        rc_body.append('        }')
        rc_body.append('        default:')
        rc_body.append('            break;')
        rc_body.append('    }')
        rc_body.append('    return;')
        rc_body.append('    }')
        rc_body.append('')
        rc_body.append('    void clear_all_map_handles()\n    {')
        for var in sorted(obj_map_dict):
            rc_body.append('        %s.clear();' % var)
        rc_body.append('    }\n')
        disp_obj_types = [obj for obj in vulkan.object_dispatch_list]
        for var in sorted(obj_map_dict):
            # Disp objs are pts so the obj can be map key, for non-disp objs, use uint64_t handle as map key
            if obj_map_dict[var] in disp_obj_types:
                rc_body.append(self._map_decl(obj_map_dict[var], obj_map_dict[var], var))
                rc_body.append(self._add_to_map_decl(obj_map_dict[var], obj_map_dict[var], var))
                rc_body.append(self._rm_from_map_decl(obj_map_dict[var], var))
                rc_body.append(self._remap_decl(obj_map_dict[var], var))
            elif obj_map_dict[var] == 'VkImage':
                rc_body.append(self._map_decl('uint64_t', 'imageObj', var))
                rc_body.append(self._add_to_map_decl('uint64_t', 'imageObj', var))
                rc_body.append(self._rm_from_map_decl('uint64_t', var))
                rc_body.append('    uint64_t remap_images(const uint64_t& value)')
                rc_body.append('    {')
                rc_body.append('        if (value == 0) { return 0; }')
                rc_body.append('')
                rc_body.append('        std::map<uint64_t, imageObj>::const_iterator q = m_images.find(value);')
                rc_body.append('        if (q == m_images.end()) { glv_LogError("Failed to remap VkImage."); return value; }\n')
                rc_body.append('        return q->second.replayImage.handle;')
                rc_body.append('    }\n')
            elif obj_map_dict[var] == 'VkBuffer':
                rc_body.append(self._map_decl('uint64_t', 'bufferObj', var))
                rc_body.append(self._add_to_map_decl('uint64_t', 'bufferObj', var))
                rc_body.append(self._rm_from_map_decl('uint64_t', var))
                rc_body.append('    uint64_t remap_buffers(const uint64_t& value)')
                rc_body.append('    {')
                rc_body.append('        if (value == 0) { return 0; }')
                rc_body.append('')
                rc_body.append('        std::map<uint64_t, bufferObj>::const_iterator q = m_buffers.find(value);')
                rc_body.append('        if (q == m_buffers.end()) { glv_LogError("Failed to remap VkBuffer."); return value; }\n')
                rc_body.append('        return q->second.replayBuffer.handle;')
                rc_body.append('    }\n')
            elif obj_map_dict[var] == 'VkDeviceMemory':
                rc_body.append(self._map_decl('uint64_t', 'gpuMemObj', var))
                rc_body.append(self._add_to_map_decl('uint64_t', 'gpuMemObj', var))
                rc_body.append(self._rm_from_map_decl('uint64_t', var))
                rc_body.append('    uint64_t remap_devicememorys(const uint64_t& value)')
                rc_body.append('    {')
                rc_body.append('        if (value == 0) { return 0; }')
                rc_body.append('')
                rc_body.append('        std::map<uint64_t, gpuMemObj>::const_iterator q = m_devicememorys.find(value);')
                rc_body.append('        if (q == m_devicememorys.end()) { glv_LogError("Failed to remap VkDeviceMemory."); return value; }')
                rc_body.append('        return q->second.replayGpuMem.handle;')
                rc_body.append('    }\n')
            else:
                rc_body.append(self._map_decl('uint64_t', 'uint64_t', var))
                rc_body.append(self._add_to_map_decl('uint64_t', 'uint64_t', var))
                rc_body.append(self._rm_from_map_decl('uint64_t', var))
                rc_body.append(self._remap_decl('uint64_t', var))
        # VkDynamicStateObject code
# TODO138 : Each dynamic state object is now unique so need to make sure their re-mapping is being handled correctly
#        state_obj_remap_types = vulkan.object_dynamic_state_list
#        state_obj_bindings = vulkan.object_dynamic_state_bind_point_list
#        rc_body.append('    VkDynamicStateObject remap(const VkDynamicStateObject& state, const VkStateBindPoint& bindPoint)\n    {')
#        rc_body.append('        VkDynamicStateObject obj;')
#        index = 0
#        while index < len(state_obj_remap_types):
#            obj = state_obj_remap_types[index]
#            type = state_obj_bindings[index]
#            rc_body.append('        if (bindPoint == %s) {' % type)
#            rc_body.append('            if ((obj = remap(static_cast <%s> (state))) != VK_NULL_HANDLE)' % obj.type)
#            rc_body.append('                return obj;')
#            rc_body.append('        }')
#            index += 1
#        for obj in state_obj_remap_types:
#            rc_body.append('//        if ((obj = remap(static_cast <%s> (state))) != VK_NULL_HANDLE)' % obj.type)
#            rc_body.append('//            return obj;')
#        rc_body.append('        glv_LogWarning("Failed to remap VkDynamicStateObject.");')
#        rc_body.append('        return VK_NULL_HANDLE;\n    }')
#        rc_body.append('    void rm_from_map(const VkDynamicStateObject& state)\n    {')
#        for obj in state_obj_remap_types:
#            rc_body.append('        rm_from_map(static_cast <%s> (state));' % obj.type)
#        rc_body.append('    }')
#        rc_body.append('')
        # OBJECT code
# TODO138 : VkObject construct is now dead, and I believe this code can die with it
#        rc_body.append('    VkObject remap(const VkObject& object, VkObjectType objectType)\n    {')
#        rc_body.append('        VkObject obj = VK_NULL_HANDLE;')
#        obj_remap_types = vulkan.object_type_list
#        rc_body.append('        switch ((unsigned int)objectType) {')
#        for obj in obj_remap_types:
#            if obj.type not in vulkan.object_parent_list:
#                rc_body.append('        case %s:' % obj.enum)
#                rc_body.append('            obj = remap(static_cast <%s> (object));' % obj.type)
#                rc_body.append('            break;')
#        rc_body.append('        case GLV_VK_OBJECT_TYPE_UNKNOWN:')
#        rc_body.append('        default:')
#        rc_body.append('            obj = VK_NULL_HANDLE;')
#        rc_body.append('            break;')
#        rc_body.append('        }\n')
#        rc_body.append('        if (obj == VK_NULL_HANDLE)')
#        rc_body.append('        {')
#        for obj in obj_remap_types:
#            if obj.type not in vulkan.object_parent_list:
#                rc_body.append('            if ((obj = remap(static_cast <%s> (object))) != VK_NULL_HANDLE) return obj;' % obj.type)
#        rc_body.append('            glv_LogError("Failed to remap VkObject.");')
#        rc_body.append('        }')
#        rc_body.append('        return obj;')
#        rc_body.append('    }')
#        rc_body.append('')
#        rc_body.append('    void rm_from_map(const VkObject& objKey, VkObjectType objectType)\n    {')
#        rc_body.append('        switch ((unsigned int)objectType) {')
#        for obj in obj_remap_types:
#            if obj.type not in vulkan.object_parent_list:
#                rc_body.append('        case %s:' % obj.enum)
#                rc_body.append('           rm_from_map(static_cast <%s> (objKey));' % (obj.type))
#                rc_body.append('           break;')
#        rc_body.append('        default:')
#        rc_body.append('           assert(!"Unhandled or invalid VkObjectType passed into rm_from_map(..)");')
#        rc_body.append('           break;')
#        rc_body.append('        }')
#        rc_body.append('    }')
        rc_body.append('};')
        return "\n".join(rc_body)

    def _generate_replay_init_funcs(self):
        rif_body = []
        rif_body.append('void vkFuncs::init_funcs(void * handle)\n{\n    m_libHandle = handle;')
        for proto in self.protos:
            if 'WSI' not in proto.name and 'Dbg' not in proto.name:
                rif_body.append('    real_vk%s = (type_vk%s)(glv_platform_get_library_entrypoint(handle, "vk%s"));' % (proto.name, proto.name, proto.name))
            else: # These func ptrs get assigned at GetProcAddr time
                rif_body.append('    real_vk%s = (type_vk%s)NULL;' % (proto.name, proto.name))
        rif_body.append('}')
        return "\n".join(rif_body)

    def _remap_packet_param(self, funcName, paramType, paramName):
        remap_list = vulkan.object_type_list
        param_exclude_list = ['pDescriptorSets', 'pFences']
        cleanParamType = paramType.strip('*').replace('const ', '')
        VulkNonDispObjects = [o for o in vulkan.object_non_dispatch_list]
        for obj in remap_list:
            if obj == cleanParamType and paramName not in param_exclude_list:
                objectTypeRemapParam = ''
                if 'VkDynamicStateObject' == cleanParamType:
                    objectTypeRemapParam = ', pPacket->stateBindPoint'
                elif 'object' == paramName:
                    if 'DbgSetObjectTag' == funcName:
                        objectTypeRemapParam = ', GLV_VK_OBJECT_TYPE_UNKNOWN'
                    else:
                        objectTypeRemapParam = ', pPacket->objType'
                elif 'srcObject' == paramName and 'Callback' in funcName:
                    objectTypeRemapParam = ', pPacket->objType'
                if '*' in paramType:
                    if 'const ' not in paramType:
                        result = '            %s remapped%s = m_objMapper.remap_%ss(*pPacket->%s%s);\n' % (cleanParamType, paramName, paramName.lower(), paramName, objectTypeRemapParam)
                        result += '            if (pPacket->%s != VK_NULL_HANDLE && remapped%s == VK_NULL_HANDLE)\n' % (paramName, paramName)
                        result += '            {\n'
                        result += '                return glv_replay::GLV_REPLAY_ERROR;\n'
                        result += '            }\n'
                        return result
                    else: # TODO : Don't remap array ptrs?
                        return '            // pPacket->%s should have been remapped with special case code' % (paramName)
                if paramType in VulkNonDispObjects:
                    result = '            %s remapped%s;\n' % (paramType, paramName)
                    result += '            remapped%s.handle = m_objMapper.remap_%ss(pPacket->%s%s.handle);\n' % (paramName, cleanParamType.lower()[2:], paramName, objectTypeRemapParam)
                    result += '%s\n' % self.lineinfo.get()
                    result += '            if (pPacket->%s.handle != 0 && remapped%s.handle == 0)\n' % (paramName, paramName)
                    result += '            {\n'
                    result += '                return glv_replay::GLV_REPLAY_ERROR;\n'
                    result += '            }\n'
                else:
                    result = '            %s remapped%s = m_objMapper.remap_%ss(pPacket->%s%s);\n' % (paramType, paramName, cleanParamType.lower()[2:], paramName, objectTypeRemapParam)
                    result += '%s\n' % self.lineinfo.get()
                    result += '            if (pPacket->%s != VK_NULL_HANDLE && remapped%s == VK_NULL_HANDLE)\n' % (paramName, paramName)
                    result += '            {\n'
                    result += '                return glv_replay::GLV_REPLAY_ERROR;\n'
                    result += '            }\n'
                return result
        return '            // No need to remap %s' % (paramName)

    def _get_packet_param(self, funcName, paramType, paramName):
        # list of types that require remapping
        remap_list = vulkan.object_type_list
        param_exclude_list = ['pDescriptorSets', 'pFences']
        cleanParamType = paramType.strip('*').replace('const ', '')
        for obj in remap_list:
            if obj == cleanParamType and paramName not in param_exclude_list:
                objectTypeRemapParam = ''
                if 'object' == paramName:
                    if 'DbgSetObjectTag' == funcName:
                        objectTypeRemapParam = ', GLV_VK_OBJECT_TYPE_UNKNOWN'
                    else:
                        objectTypeRemapParam = ', pPacket->objType'
                if '*' in paramType:
                    if 'const ' not in paramType:
                        return 'remapped%s' % (paramName)
                    else: # TODO : Don't remap array ptrs?
                        return 'pPacket->%s' % (paramName)
                return 'remapped%s' % (paramName)
        return 'pPacket->%s' % (paramName)

    def _gen_replay_create_image(self):
        ci_body = []
        ci_body.append('            imageObj local_imageObj;')
        ci_body.append('            VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);')
        ci_body.append('            if (remappedDevice == VK_NULL_HANDLE)')
        ci_body.append('            {')
        ci_body.append('                return glv_replay::GLV_REPLAY_ERROR;')
        ci_body.append('            }')
        ci_body.append('            replayResult = m_vkFuncs.real_vkCreateImage(remappedDevice, pPacket->pCreateInfo, &local_imageObj.replayImage);')
        ci_body.append('            if (replayResult == VK_SUCCESS)')
        ci_body.append('            {')
        ci_body.append('                m_objMapper.add_to_images_map(pPacket->pImage->handle, local_imageObj);')
        ci_body.append('            }')
        return "\n".join(ci_body)

    def _gen_replay_create_buffer(self):
        cb_body = []
        cb_body.append('            bufferObj local_bufferObj;')
        cb_body.append('            VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);')
        cb_body.append('            if (remappedDevice == VK_NULL_HANDLE)')
        cb_body.append('            {')
        cb_body.append('                return glv_replay::GLV_REPLAY_ERROR;')
        cb_body.append('            }')
        cb_body.append('            replayResult = m_vkFuncs.real_vkCreateBuffer(remappedDevice, pPacket->pCreateInfo, &local_bufferObj.replayBuffer);')
        cb_body.append('            if (replayResult == VK_SUCCESS)')
        cb_body.append('            {')
        cb_body.append('                m_objMapper.add_to_buffers_map(pPacket->pBuffer->handle, local_bufferObj);')
        cb_body.append('            }')
        return "\n".join(cb_body)

    # Generate main replay case statements where actual replay API call is dispatched based on input packet data
    def _generate_replay(self):
        manually_replay_funcs = ['AllocMemory',
                                 'BeginCommandBuffer',
                                 'CreateDescriptorSetLayout',
                                 'CreateDevice',
                                 'CreateFramebuffer',
                                 'CreateGraphicsPipelines',
                                 'CreateInstance',
                                 'CreatePipelineLayout',
                                 'CreateRenderPass',
                                 'CreateShader',
                                 'CmdBeginRenderPass',
                                 'CmdBindDescriptorSets',
                                 'CmdBindVertexBuffers',
                                 'CmdPipelineBarrier',
                                 'QueuePresentWSI',
                                 'CmdWaitEvents',
                                 #'DestroyObject',
                                 'EnumeratePhysicalDevices',
                                 'FreeMemory',
                                 'FreeDescriptorSets',
                                 'FlushMappedMemoryRanges',
                                 #'GetGlobalExtensionInfo',
                                 #'GetImageSubresourceInfo',
                                 #'GetObjectInfo',
                                 #'GetPhysicalDeviceExtensionInfo',
                                 'GetPhysicalDeviceSurfaceSupportWSI',
                                 'GetSurfacePropertiesWSI',
                                 'GetSurfaceFormatsWSI',
                                 'GetSurfacePresentModesWSI',
                                 'CreateSwapChainWSI',
                                 'GetSwapChainImagesWSI',
                                 #'GetPhysicalDeviceInfo',
                                 'MapMemory',
                                 #'QueuePresentWSI',
                                 'QueueSubmit',
                                 #'StorePipeline',
                                 'UnmapMemory',
                                 'UpdateDescriptorSets',
                                 'WaitForFences',
                                 'DbgCreateMsgCallback',
                                 'DbgDestroyMsgCallback',
                                 'CreateCommandBuffer',
                                 ]

        # validate the manually_replay_funcs list
        protoFuncs = [proto.name for proto in self.protos]
        for func in manually_replay_funcs:
            if func not in protoFuncs:
                sys.exit("Entry '%s' in manually_replay_funcs list is not in the vulkan function prototypes" % func)

        # map protos to custom functions if body is fully custom
        custom_body_dict = {'CreateImage': self._gen_replay_create_image,
                            'CreateBuffer': self._gen_replay_create_buffer }
        # multi-gpu Open funcs w/ list of local params to create
        custom_open_params = {'OpenSharedMemory': (-1,),
                              'OpenSharedSemaphore': (-1,),
                              'OpenPeerMemory': (-1,),
                              'OpenPeerImage': (-1, -2,)}
        # Functions that create views are unique from other create functions
        create_view_list = ['CreateBufferView', 'CreateImageView', 'CreateAttachmentView', 'CreateComputePipeline']
        # Functions to treat as "Create' that don't have 'Create' in the name
        special_create_list = ['LoadPipeline', 'LoadPipelineDerivative', 'AllocMemory', 'GetDeviceQueue', 'PinSystemMemory', 'AllocDescriptorSets']
        # A couple funcs use do while loops
        do_while_dict = {'GetFenceStatus': 'replayResult != pPacket->result  && pPacket->result == VK_SUCCESS', 'GetEventStatus': '(pPacket->result == VK_EVENT_SET || pPacket->result == VK_EVENT_RESET) && replayResult != pPacket->result'}
        rbody = []
        rbody.append('%s' % self.lineinfo.get())
        rbody.append('glv_replay::GLV_REPLAY_RESULT vkReplay::replay(glv_trace_packet_header *packet)')
        rbody.append('{')
        rbody.append('    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;')
        rbody.append('    VkResult replayResult = VK_ERROR_UNKNOWN;')
        rbody.append('    switch (packet->packet_id)')
        rbody.append('    {')
        rbody.append('        case GLV_TPI_VK_vkApiVersion:')
        rbody.append('        {')
        rbody.append('            packet_vkApiVersion* pPacket = (packet_vkApiVersion*)(packet->pBody);')
        rbody.append('            if (pPacket->version != VK_API_VERSION)')
        rbody.append('            {')
        rbody.append('                glv_LogError("Trace file is from Vulkan version 0x%x (%u.%u.%u), but the glave plugin only supports version 0x%x (%u.%u.%u).", pPacket->version, (pPacket->version & 0xFFC00000) >> 22, (pPacket->version & 0x003FF000) >> 12, (pPacket->version & 0x00000FFF), VK_API_VERSION, (VK_API_VERSION & 0xFFC00000) >> 22, (VK_API_VERSION & 0x003FF000) >> 12, (VK_API_VERSION & 0x00000FFF));')
        rbody.append('                returnValue = glv_replay::GLV_REPLAY_ERROR;')
        rbody.append('            }')
        rbody.append('            break;')
        rbody.append('        }')
        for proto in self.protos:
            ret_value = False
            create_view = False
            create_func = False
            # TODO : How to handle void* return of GetProcAddr?
            if ('void' not in proto.ret.lower()) and ('size_t' not in proto.ret) and (proto.name not in custom_body_dict):
                ret_value = True
            if proto.name in create_view_list:
                create_view = True
            elif 'Create' in proto.name or proto.name in special_create_list:
                create_func = True
            rbody.append('        case GLV_TPI_VK_vk%s:' % proto.name)
            rbody.append('        {')
            rbody.append('            packet_vk%s* pPacket = (packet_vk%s*)(packet->pBody);' % (proto.name, proto.name))
            if proto.name in manually_replay_funcs:
                if ret_value == True:
                    rbody.append('            replayResult = manually_replay_vk%s(pPacket);' % proto.name)
                else:
                    rbody.append('            manually_replay_vk%s(pPacket);' % proto.name)
            elif proto.name in custom_body_dict:
                rbody.append(custom_body_dict[proto.name]())
            else:
                if proto.name in custom_open_params:
                    for pidx in custom_open_params[proto.name]:
                        rbody.append('            %s local_%s;' % (proto.params[pidx].ty.replace('const ', '').strip('*'), proto.params[pidx].name))
                elif create_view:
                    rbody.append('            %s createInfo;' % (proto.params[1].ty.strip('*').replace('const ', '')))
                    rbody.append('            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(%s));' % (proto.params[1].ty.strip('*').replace('const ', '')))
                    if 'CreateComputePipeline' == proto.name:
                        rbody.append('            createInfo.cs.shader.handle = m_objMapper.remap_shaders(pPacket->pCreateInfo->cs.shader.handle);')
                    elif 'CreateBufferView' == proto.name:
                        rbody.append('            createInfo.buffer.handle = m_objMapper.remap_buffers(pPacket->pCreateInfo->buffer.handle);')
                    else:
                        rbody.append('            createInfo.image.handle = m_objMapper.remap_images(pPacket->pCreateInfo->image.handle);')
                    rbody.append('            %s local_%s;' % (proto.params[-1].ty.strip('*').replace('const ', ''), proto.params[-1].name))
                elif create_func: # Declare local var to store created handle into
                    if 'AllocDescriptorSets' == proto.name:
                        p_ty = proto.params[-1].ty.strip('*').replace('const ', '')
                        rbody.append('            %s* local_%s = (%s*)malloc(pPacket->count * sizeof(%s));' % (p_ty, proto.params[-1].name, p_ty, p_ty))
                        rbody.append('            VkDescriptorSetLayout* local_pSetLayouts = (VkDescriptorSetLayout*)malloc(pPacket->count * sizeof(VkDescriptorSetLayout));')
                        rbody.append('            for (uint32_t i = 0; i < pPacket->count; i++)')
                        rbody.append('            {')
                        rbody.append('                local_pSetLayouts[i].handle = m_objMapper.remap_descriptorsetlayouts(pPacket->%s[i].handle);' % (proto.params[-2].name))
                        rbody.append('            }')
                    else:
                        rbody.append('            %s local_%s;' % (proto.params[-1].ty.strip('*').replace('const ', ''), proto.params[-1].name))
                elif proto.name == 'ResetFences':
                    rbody.append('            VkFence* fences = GLV_NEW_ARRAY(VkFence, pPacket->fenceCount);')
                    rbody.append('            for (uint32_t i = 0; i < pPacket->fenceCount; i++)')
                    rbody.append('            {')
                    rbody.append('                fences[i].handle = m_objMapper.remap_fences(pPacket->%s[i].handle);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name in do_while_dict:
                    rbody.append('            do {')

                for p in proto.params:
                    if create_func or create_view:
                        if p.name != proto.params[-1].name:
                            rbody.append(self._remap_packet_param(proto.name, p.ty, p.name))
                    else:
                        rbody.append(self._remap_packet_param(proto.name, p.ty, p.name))

                if proto.name == 'DestroyInstance':
                    rbody.append('            if (m_vkFuncs.real_vkDbgDestroyMsgCallback != NULL)')
                    rbody.append('            {')
                    rbody.append('                m_vkFuncs.real_vkDbgDestroyMsgCallback(remappedinstance, m_dbgMsgCallbackObj);')
                    rbody.append('            }')
                # TODO: need a better way to indicate which extensions should be mapped to which Get*ProcAddr
                elif proto.name == 'GetInstanceProcAddr':
                    for iProto in self.protos:
                        if 'Dbg' in iProto.name or 'GetPhysicalDeviceSurfaceSupportWSI' in iProto.name:
                            rbody.append('            if (strcmp(pPacket->pName, "vk%s") == 0) {' % (iProto.name))
                            rbody.append('               m_vkFuncs.real_vk%s = (PFN_vk%s)vk%s(remappedinstance, pPacket->pName);' % (iProto.name, iProto.name, proto.name))
                            rbody.append('            }')
                elif proto.name == 'GetDeviceProcAddr':
                    for dProto in self.protos:
                        if 'WSI' in dProto.name:
                            rbody.append('            if (strcmp(pPacket->pName, "vk%s") == 0) {' % (dProto.name))
                            rbody.append('               m_vkFuncs.real_vk%s = (PFN_vk%s)vk%s(remappeddevice, pPacket->pName);' % (dProto.name, dProto.name, proto.name))
                            rbody.append('            }')

                # build the call to the "real_" entrypoint
                rr_string = '            '
                if ret_value:
                    rr_string = '            replayResult = '
                rr_string += 'm_vkFuncs.real_vk%s(' % proto.name
                for p in proto.params:
                    # For last param of Create funcs, pass address of param
                    if create_func:
                        if proto.name == 'AllocDescriptorSets' and ((p.name == proto.params[-2].name) or (p.name == proto.params[-1].name)):
                            rr_string += 'local_%s, ' % p.name
                        elif p.name == proto.params[-1].name:
                            rr_string += '&local_%s, ' % p.name
                        else:
                            rr_string += '%s, ' % self._get_packet_param(proto.name, p.ty, p.name)
                    else:
                        rr_string += '%s, ' % self._get_packet_param(proto.name, p.ty, p.name)
                rr_string = '%s);' % rr_string[:-2]
                if proto.name in custom_open_params:
                    rr_list = rr_string.split(', ')
                    for pidx in custom_open_params[proto.name]:
                        rr_list[pidx] = '&local_%s' % proto.params[pidx].name
                    rr_string = ', '.join(rr_list)
                    rr_string += ');'
                elif create_view:
                    rr_list = rr_string.split(', ')
                    rr_list[-2] = '&createInfo'
                    rr_list[-1] = '&local_%s);' % proto.params[-1].name
                    rr_string = ', '.join(rr_list)
                    # this is a sneaky shortcut to use generic create code below to add_to_map
                    create_func = True
                elif proto.name == 'AllocDescriptorSets':
                    rr_string = rr_string.replace('pPacket->pSetLayouts', 'pLocalDescSetLayouts')
                elif proto.name == 'ResetFences':
                   rr_string = rr_string.replace('pPacket->pFences', 'fences')

                # insert the real_*(..) call
                rbody.append(rr_string)

                # handle return values or anything that needs to happen after the real_*(..) call
                if 'DestroyDevice' in proto.name:
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                m_pCBDump = NULL;')
                    rbody.append('                m_pDSDump = NULL;')
                    #TODO138 : disabling snapshot
                    #rbody.append('                m_pGlvSnapshotPrint = NULL;')
                    rbody.append('                m_objMapper.rm_from_devices_map(pPacket->device);')
                    rbody.append('                m_display->m_initedVK = false;')
                    rbody.append('            }')
                elif 'DestroySwapChainWSI' in proto.name:
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                m_objMapper.rm_from_swapchainwsis_map(pPacket->swapChain.handle);')
                    rbody.append('            }')
                elif 'DestroyInstance' in proto.name:
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                // TODO need to handle multiple instances and only clearing maps within an instance.')
                    rbody.append('                // TODO this only works with a single instance used at any given time.')
                    rbody.append('                m_objMapper.clear_all_map_handles();')
                    rbody.append('            }')
                elif 'AllocDescriptorSets' in proto.name:
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                for (uint32_t i = 0; i < pPacket->count; i++) {')
                    rbody.append('                    m_objMapper.add_to_descriptorsets_map(pPacket->%s[i].handle, local_%s[i].handle);' % (proto.params[-1].name, proto.params[-1].name))
                    rbody.append('                }')
                    rbody.append('            }')
                    rbody.append('            free(local_pSetLayouts);')
                    rbody.append('            free(local_pDescriptorSets);')
                elif proto.name == 'ResetFences':
                    rbody.append('            GLV_DELETE(fences);')
                elif create_func: # save handle mapping if create successful
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    clean_type = proto.params[-1].ty.strip('*').replace('const ', '')
                    VkNonDispObjType = [o for o in vulkan.object_non_dispatch_list]
                    if clean_type in VkNonDispObjType:
                        rbody.append('                m_objMapper.add_to_%ss_map(pPacket->%s->handle, local_%s.handle);' % (clean_type.lower()[2:], proto.params[-1].name, proto.params[-1].name))
                    else:
                        rbody.append('                m_objMapper.add_to_%ss_map(*(pPacket->%s), local_%s);' % (clean_type.lower()[2:], proto.params[-1].name, proto.params[-1].name))
                    if 'AllocMemory' == proto.name:
                        rbody.append('                m_objMapper.add_entry_to_mapData(local_%s, pPacket->pAllocInfo->allocationSize);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name in do_while_dict:
                    rbody[-1] = '    %s' % rbody[-1]
                    rbody.append('            } while (%s);' % do_while_dict[proto.name])
                    rbody.append('            if (pPacket->result != VK_NOT_READY || replayResult != VK_SUCCESS)')
            if ret_value:
                rbody.append('            CHECK_RETURN_VALUE(vk%s);' % proto.name)
            rbody.append('            break;')
            rbody.append('        }')
        rbody.append('        default:')
        rbody.append('            glv_LogWarning("Unrecognized packet_id %u, skipping.", packet->packet_id);')
        rbody.append('            returnValue = glv_replay::GLV_REPLAY_INVALID_ID;')
        rbody.append('            break;')
        rbody.append('    }')
        rbody.append('    return returnValue;')
        rbody.append('}')
        return "\n".join(rbody)

class GlaveTraceHeader(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#include "vktrace_vk_vk_packets.h"')
        header_txt.append('#include "vktrace_vk_packet_id.h"\n')
        header_txt.append('void AttachHooks();')
        header_txt.append('void DetachHooks();')
        header_txt.append('void InitTracer(void);\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_trace_func_ptrs(),
                self._generate_trace_func_protos(),
                self._generate_trace_real_func_ptr_protos()]

        return "\n".join(body)

class GlaveTraceC(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#include "vktrace_platform.h"')
        header_txt.append('#include "vktrace_common.h"')
        header_txt.append('#include "vktrace_lib_helpers.h"')
        header_txt.append('#include "vktrace_vk_vk.h"')
        header_txt.append('#include "vktrace_vk_vk_debug_report_lunarg.h"')
        header_txt.append('#include "vktrace_vk_vk_debug_marker_lunarg.h"')
        header_txt.append('#include "vktrace_vk_vk_wsi_swapchain.h"')
        header_txt.append('#include "vktrace_vk_vk_wsi_device_swapchain.h"')
        header_txt.append('#include "vktrace_interconnect.h"')
        header_txt.append('#include "vktrace_filelike.h"')
        header_txt.append('#include "vk_struct_size_helper.h"')
        header_txt.append('#ifdef WIN32')
        header_txt.append('#include "mhook/mhook-lib/mhook.h"')
        header_txt.append('#else')
        header_txt.append('#include <pthread.h>\n')
        header_txt.append('#endif')
        header_txt.append('#include "vktrace_trace_packet_utils.h"')
        header_txt.append('#include <stdio.h>')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_func_ptr_assignments(),
                self._generate_attach_hooks(),
                self._generate_detach_hooks(),
                self._generate_init_funcs(),
                self._generate_trace_funcs(self.extName)]

        return "\n".join(body)

class GlavePacketID(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "vktrace_trace_packet_utils.h"')
        header_txt.append('#include "vktrace_trace_packet_identifiers.h"')
        header_txt.append('#include "vktrace_interconnect.h"')
        header_txt.append('#include "vktrace_vk_vk_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_debug_report_lunarg_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_debug_marker_lunarg_packets.h"')
        #header_txt.append('#include "vktrace_vk_vk_wsi_lunarg_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_wsi_swapchain_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_wsi_device_swapchain_packets.h"')
        #header_txt.append('#include "vk_enum_string_helper.h"')
        header_txt.append('#ifndef _WIN32')
        header_txt.append(' #pragma GCC diagnostic ignored "-Wwrite-strings"')
        header_txt.append('#endif')
        #header_txt.append('#include "vk_struct_string_helper.h"')
        #header_txt.append('#include "vk_wsi_swapchain_struct_string_helper.h"')
        #header_txt.append('#include "vk_wsi_device_swapchain_struct_string_helper.h"')
        header_txt.append('#ifndef _WIN32')
        header_txt.append(' #pragma GCC diagnostic warning "-Wwrite-strings"')
        header_txt.append('#endif')
        #header_txt.append('#include "vk_wsi_swapchain_enum_string_helper.h"')
        #header_txt.append('#include "vk_wsi_device_swapchain_enum_string_helper.h"')
        header_txt.append('#if defined(WIN32)')
        header_txt.append('#define snprintf _snprintf')
        header_txt.append('#endif')
        header_txt.append('#define SEND_ENTRYPOINT_ID(entrypoint) ;')
        header_txt.append('//#define SEND_ENTRYPOINT_ID(entrypoint) glv_TraceInfo(#entrypoint);\n')
        header_txt.append('#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) ;')
        header_txt.append('//#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) glv_TraceInfo(entrypoint, __VA_ARGS__);\n')
        header_txt.append('#define CREATE_TRACE_PACKET(entrypoint, buffer_bytes_needed) \\')
        header_txt.append('    pHeader = glv_create_trace_packet(GLV_TID_VULKAN, GLV_TPI_VK_##entrypoint, sizeof(packet_##entrypoint), buffer_bytes_needed);\n')
        header_txt.append('#define FINISH_TRACE_PACKET() \\')
        header_txt.append('    glv_finalize_trace_packet(pHeader); \\')
        header_txt.append('    glv_write_trace_packet(pHeader, glv_trace_get_trace_file()); \\')
        header_txt.append('    glv_delete_trace_packet(&pHeader);')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_packet_id_enum(),
                self._generate_packet_id_name_func(),
#                self._generate_stringify_func(),
                self._generate_interp_func()]

        return "\n".join(body)

class GlaveCoreTracePackets(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "vktrace_trace_packet_utils.h"\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_struct_util_funcs(),
                self._generate_interp_funcs()]

        return "\n".join(body)

class GlaveExtTraceHeader(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "%s.h"' % extName.lower())
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_trace_func_ptrs_ext(self.extName),
                self._generate_trace_func_protos_ext(self.extName)]

        return "\n".join(body)

class GlaveExtTraceC(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#include "vktrace_platform.h"')
        header_txt.append('#include "vktrace_common.h"')
        if extName == "vk_wsi_device_swapchain":
            header_txt.append('#include "vk_wsi_swapchain.h"')
        header_txt.append('#include "vktrace_vk_%s.h"' % extName.lower())
        header_txt.append('#include "vktrace_vk_%s_packets.h"' % extName.lower())
        header_txt.append('#include "vktrace_vk_packet_id.h"')
        header_txt.append('#include "vk_struct_size_helper.h"')
        header_txt.append('#include "%s_struct_size_helper.h"' % extName.lower())
        header_txt.append('#ifdef WIN32')
        header_txt.append('#include "mhook/mhook-lib/mhook.h"')
        header_txt.append('#endif')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_func_ptr_assignments_ext(self.extName),
                self._generate_trace_funcs(self.extName)]

        return "\n".join(body)

class GlaveExtTracePackets(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "%s.h"' % extName.lower())
        header_txt.append('#include "vktrace_trace_packet_utils.h"\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_interp_funcs_ext(self.extName)]

        return "\n".join(body)

class GlaveReplayVkFuncPtrs(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include <xcb/xcb.h>\n')
        header_txt.append('#endif')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "vk_debug_report_lunarg.h"')
        header_txt.append('#include "vk_debug_marker_lunarg.h"')
        header_txt.append('#include "vk_wsi_swapchain.h"')
        header_txt.append('#include "vk_wsi_device_swapchain.h"')

    def generate_body(self):
        body = [self._generate_replay_func_ptrs()]
        return "\n".join(body)

class GlaveReplayObjMapperHeader(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include <set>')
        header_txt.append('#include <map>')
        header_txt.append('#include <vector>')
        header_txt.append('#include <string>')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "vk_debug_report_lunarg.h"')
        header_txt.append('#include "vk_debug_marker_lunarg.h"')
        header_txt.append('#include "vk_wsi_swapchain.h"')
        header_txt.append('#include "vk_wsi_device_swapchain.h"')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_objmapper_class()]
        return "\n".join(body)

class GlaveReplayC(Subcommand):
    def generate_header(self, extName):
        header_txt = []
        header_txt.append('#include "vkreplay_vkreplay.h"\n')
        header_txt.append('#include "vkreplay.h"\n')
        header_txt.append('#include "vkreplay_main.h"\n')
        header_txt.append('#include <algorithm>')
        header_txt.append('#include <queue>')
        header_txt.append('\n')
        header_txt.append('extern "C" {')
        header_txt.append('#include "vktrace_vk_vk_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_debug_report_lunarg_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_debug_marker_lunarg_packets.h"')
        #header_txt.append('#include "vktrace_vk_vk_wsi_lunarg_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_wsi_swapchain_packets.h"')
        header_txt.append('#include "vktrace_vk_vk_wsi_device_swapchain_packets.h"')
        header_txt.append('#include "vktrace_vk_packet_id.h"')
        #header_txt.append('#include "vk_enum_string_helper.h"\n}\n')

        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_init_funcs(),
                self._generate_replay()]
        body.append("}")
        return "\n".join(body)

def main():
    subcommands = {
            "glave-trace-h" : GlaveTraceHeader,
            "glave-trace-c" : GlaveTraceC,
            "glave-packet-id" : GlavePacketID,
            "glave-core-trace-packets" : GlaveCoreTracePackets,
            "glave-ext-trace-h" : GlaveExtTraceHeader,
            "glave-ext-trace-c" : GlaveExtTraceC,
            "glave-ext-trace-packets" : GlaveExtTracePackets,
            "glave-replay-vk-funcs" : GlaveReplayVkFuncPtrs,
            "glave-replay-obj-mapper-h" : GlaveReplayObjMapperHeader,
            "glave-replay-c" : GlaveReplayC,
    }

    if len(sys.argv) < 2 or sys.argv[1] not in subcommands:
        print("Usage: %s <subcommand> [options]" % sys.argv[0])
        print
        print("Available subcommands are: %s" % " ".join(subcommands))
        exit(1)

    subcmd = subcommands[sys.argv[1]](sys.argv[2])
    subcmd.run()

if __name__ == "__main__":
    main()
