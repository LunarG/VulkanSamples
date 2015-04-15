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
main_path = os.path.abspath(glv_scripts_path + "/../../../")
sys.path.append(main_path)

import vulkan

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.headers = vulkan.headers
        self.protos = vulkan.protos

    def run(self):
        print(self.generate())

    def generate(self):
        copyright = self.generate_copyright()
        header = self.generate_header()
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

    def generate_header(self):
        return "\n".join(["#include <" + h + ">" for h in self.headers])

    def generate_body(self):
        pass

    def generate_footer(self):
        pass

    # Return set of printf '%' qualifier, input to that qualifier, and any dereference
    def _get_printf_params(self, vk_type, name, output_param):
        deref = ""
        # TODO : Need ENUM and STRUCT checks here
        if "VkImage_LAYOUT" in vk_type:
            return ("%s", "string_%s(%s)" % (vk_type.replace('const ', '').strip('*'), name), deref)
        if "VkClearColor" in vk_type:
            return ("%p", "(void*)&%s" % name, deref)
        if "_type" in vk_type.lower(): # TODO : This should be generic ENUM check
            return ("%s", "string_%s(%s)" % (vk_type.replace('const ', '').strip('*'), name), deref)
        if "char*" == vk_type:
            return ("%s", name, "*")
        if "uint64_t" in vk_type:
            if '*' in vk_type:
                return ("%lu",  "(%s == NULL) ? 0 : *(%s)" % (name, name), "*")
            return ("%lu", name, deref)
        if "size_t" in vk_type:
            if '*' in vk_type:
                return ("%zu", "(%s == NULL) ? 0 : *(%s)" % (name, name), "*")
            return ("%zu", name, deref)
        if "float" in vk_type:
            if '[' in vk_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                return ("[%f, %f, %f, %f]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name), deref)
            return ("%f", name, deref)
        if "bool" in vk_type or 'xcb_randr_crtc_t' in vk_type:
            return ("%u", name, deref)
        if True in [t in vk_type.lower() for t in ["int", "flags", "mask", "xcb_window_t"]]:
            if '[' in vk_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                return ("[%i, %i, %i, %i]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name), deref)
            if '*' in vk_type:
                return ("%i", "(%s == NULL) ? 0 : *(%s)" % (name, name), "*")
            return ("%i", name, deref)
        if output_param:
            return ("%p", "(void*)%s" % name, deref)
        return ("%p", "(void*)(%s)" % name, deref)

    def _generate_trace_func_ptrs(self):
        func_ptrs = []
        func_ptrs.append('// Pointers to real functions and declarations of hooked functions')
        func_ptrs.append('#ifdef WIN32')
        func_ptrs.append('extern INIT_ONCE gInitOnce;')
        for proto in self.protos:
            if True not in [skip_str in proto.name for skip_str in ['Dbg', 'Wsi']]: #Dbg' not in proto.name and 'Wsi' not in proto.name:
                func_ptrs.append('#define __HOOKED_vk%s hooked_vk%s' % (proto.name, proto.name))

        func_ptrs.append('\n#elif defined(PLATFORM_LINUX)')
        func_ptrs.append('extern pthread_once_t gInitOnce;')
        for proto in self.protos:
            if True not in [skip_str in proto.name for skip_str in ['Dbg', 'Wsi']]:
                func_ptrs.append('#define __HOOKED_vk%s vk%s' % (proto.name, proto.name))

        func_ptrs.append('#endif\n')
        return "\n".join(func_ptrs)

    def _generate_trace_func_ptrs_ext(self, func_class='Wsi'):
        func_ptrs = []
        func_ptrs.append('#ifdef WIN32')
        for proto in self.protos:
            if func_class in proto.name:
                func_ptrs.append('#define __HOOKED_vk%s hooked_vk%s' % (proto.name, proto.name))

        func_ptrs.append('#elif defined(__linux__)')
        for proto in self.protos:
            if func_class in proto.name:
                func_ptrs.append('#define __HOOKED_vk%s vk%s' % (proto.name, proto.name))

        func_ptrs.append('#endif\n')
        return "\n".join(func_ptrs)

    def _generate_trace_func_protos(self):
        func_protos = []
        func_protos.append('// Hooked function prototypes\n')
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                func_protos.append('GLVTRACER_EXPORT %s;' % proto.c_func(prefix="__HOOKED_vk", attr="VKAPI"))

        return "\n".join(func_protos)

    def _generate_trace_func_protos_ext(self, func_class='Wsi'):
        func_protos = []
        func_protos.append('// Hooked function prototypes\n')
        for proto in self.protos:
            if func_class in proto.name:
                func_protos.append('GLVTRACER_EXPORT %s;' % proto.c_func(prefix="__HOOKED_vk", attr="VKAPI"))

        return "\n".join(func_protos)


    def _generate_trace_real_func_ptr_protos(self):
        func_ptr_assign = []
        func_ptr_assign.append('')
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                func_ptr_assign.append('extern %s( VKAPI * real_vk%s)(' % (proto.ret, proto.name))
                for p in proto.params:
                    func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
                func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ');\n')
        return "\n".join(func_ptr_assign)

    def _generate_func_ptr_assignments(self):
        func_ptr_assign = []
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                func_ptr_assign.append('%s( VKAPI * real_vk%s)(' % (proto.ret, proto.name))
                for p in proto.params:
                    func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
                func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ') = vk%s;\n' % (proto.name))
        return "\n".join(func_ptr_assign)


    def _generate_func_ptr_assignments_ext(self, func_class='Wsi'):
        func_ptr_assign = []
        for proto in self.protos:
            if func_class in proto.name:
                func_ptr_assign.append('static %s( VKAPI * real_vk%s)(' % (proto.ret, proto.name))
                for p in proto.params:
                    func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
                func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ') = vk%s;\n' % (proto.name))
        return "\n".join(func_ptr_assign)

    def _generate_attach_hooks(self):
        hooks_txt = []
        hooks_txt.append('// declared as extern in glvtrace_vk_helpers.h')
        hooks_txt.append('BOOL isHooked = FALSE;\n')
        hooks_txt.append('void AttachHooks()\n{\n   BOOL hookSuccess = TRUE;\n#if defined(WIN32)')
        hooks_txt.append('    Mhook_BeginMultiOperation(FALSE);')
        # TODO : Verify if CreateInstance is appropriate to key off of here
        hooks_txt.append('    if (real_vkCreateInstance != NULL)')
        hooks_txt.append('    {\n        isHooked = TRUE;')
        hook_operator = '='
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
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
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name and 'CreateInstance' not in proto.name:
                hooks_txt.append('    hookSuccess %s glv_platform_get_next_lib_sym((PVOID*)&real_vk%s, "vk%s");' % (hook_operator, proto.name, proto.name))
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook Vulkan.");\n    }\n')
        hooks_txt.append('#endif\n}\n')
        return "\n".join(hooks_txt)

    def _generate_attach_hooks_ext(self, func_class='Wsi'):
        func_ext_dict = {'Wsi': '_vkwsix11ext', 'Dbg': '_vkdbg'}
        first_proto_dict = {'Wsi': 'WsiX11AssociateConnection', 'Dbg': 'DbgSetValidationLevel'}
        hooks_txt = []
        hooks_txt.append('void AttachHooks%s()\n{\n    BOOL hookSuccess = TRUE;\n#if defined(WIN32)' % func_ext_dict[func_class])
        hooks_txt.append('    Mhook_BeginMultiOperation(FALSE);')
        hooks_txt.append('    if (real_vk%s != NULL)' % first_proto_dict[func_class])
        hooks_txt.append('    {')
        hook_operator = '='
        for proto in self.protos:
            if func_class in proto.name:
                hooks_txt.append('        hookSuccess %s Mhook_SetHook((PVOID*)&real_vk%s, hooked_vk%s);' % (hook_operator, proto.name, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }\n')
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook Vulkan ext %s.");\n    }\n' % func_class)
        hooks_txt.append('    Mhook_EndMultiOperation();\n')
        hooks_txt.append('#elif defined(__linux__)')
        hooks_txt.append('    hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_vk%s, "vk%s");' % (first_proto_dict[func_class], first_proto_dict[func_class]))
        for proto in self.protos:
            if func_class in proto.name and first_proto_dict[func_class] not in proto.name:
                hooks_txt.append('    hookSuccess %s glv_platform_get_next_lib_sym((PVOID*)&real_vk%s, "vk%s");' % (hook_operator, proto.name, proto.name))
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook Vulkan ext %s.");\n    }\n' % func_class)
        hooks_txt.append('#endif\n}\n')
        return "\n".join(hooks_txt)

    def _generate_detach_hooks(self):
        hooks_txt = []
        hooks_txt.append('void DetachHooks()\n{\n#ifdef __linux__\n    return;\n#elif defined(WIN32)')
        hooks_txt.append('    BOOL unhookSuccess = TRUE;\n    if (real_vkGetGpuInfo != NULL)\n    {')
        hook_operator = '='
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                hooks_txt.append('        unhookSuccess %s Mhook_Unhook((PVOID*)&real_vk%s);' % (hook_operator, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }\n    isHooked = FALSE;')
        hooks_txt.append('    if (!unhookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to unhook Vulkan.");\n    }')
        hooks_txt.append('#endif\n}')
        hooks_txt.append('#ifdef WIN32\nINIT_ONCE gInitOnce = INIT_ONCE_STATIC_INIT;\n#elif defined(PLATFORM_LINUX)\npthread_once_t gInitOnce = PTHREAD_ONCE_INIT;\n#endif\n')
        return "\n".join(hooks_txt)

    def _generate_detach_hooks_ext(self, func_class='Wsi'):
        func_ext_dict = {'Wsi': '_vkwsix11ext', 'Dbg': '_vkdbg'}
        first_proto_dict = {'Wsi': 'WsiX11AssociateConnection', 'Dbg': 'DbgSetValidationLevel'}
        hooks_txt = []
        hooks_txt.append('void DetachHooks%s()\n{\n#ifdef WIN32' % func_ext_dict[func_class])
        hooks_txt.append('    BOOL unhookSuccess = TRUE;\n    if (real_vk%s != NULL)\n    {' % first_proto_dict[func_class])
        hook_operator = '='
        for proto in self.protos:
            if func_class in proto.name:
                hooks_txt.append('        unhookSuccess %s Mhook_Unhook((PVOID*)&real_vk%s);' % (hook_operator, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }')
        hooks_txt.append('    if (!unhookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to unhook Vulkan ext %s.");\n    }' % func_class)
        hooks_txt.append('#elif defined(__linux__)\n    return;\n#endif\n}\n')
        return "\n".join(hooks_txt)

    def _generate_init_funcs(self):
        init_tracer = []
        init_tracer.append('void send_vk_api_version_packet()\n{')
        init_tracer.append('    struct_vkApiVersion* pPacket;')
        init_tracer.append('    glv_trace_packet_header* pHeader;')
        init_tracer.append('    pHeader = glv_create_trace_packet(GLV_TID_VULKAN, GLV_TPI_VK_vkApiVersion, sizeof(struct_vkApiVersion), 0);')
        init_tracer.append('    pPacket = interpret_body_as_vkApiVersion(pHeader, FALSE);')
        init_tracer.append('    pPacket->version = VK_API_VERSION;')
        init_tracer.append('    FINISH_TRACE_PACKET();\n}\n')

        init_tracer.append('extern GLV_CRITICAL_SECTION g_memInfoLock;')
        init_tracer.append('void InitTracer(void)\n{')
        init_tracer.append('    char *ipAddr = glv_get_global_var("GLVLIB_TRACE_IPADDR");')
        init_tracer.append('    if (ipAddr == NULL)')
        init_tracer.append('        ipAddr = "127.0.0.1";')
        init_tracer.append('    gMessageStream = glv_MessageStream_create(FALSE, ipAddr, GLV_BASE_PORT + GLV_TID_VULKAN);')
        init_tracer.append('    glv_trace_set_trace_file(glv_FileLike_create_msg(gMessageStream));')
        init_tracer.append('//    glv_tracelog_set_log_file(glv_FileLike_create_file(fopen("glv_log_traceside.txt","w")));')
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
                           'VK_PHYSICAL_GPU': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpus), *pGpuCount*sizeof(VK_PHYSICAL_GPU), pGpus)',
                                                'finalize_txt': 'default'},
                           'pDataSize': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize)',
                                         'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize))'},
                           'pData': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData)',
                                     'finalize_txt': 'default'},
                           'pName': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pName), ((pName != NULL) ? strlen(pName) + 1 : 0), pName)',
                                     'finalize_txt': 'default'},
                           'pExtName': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pExtName), ((pExtName != NULL) ? strlen(pExtName) + 1 : 0), pExtName)',
                                        'finalize_txt': 'default'},
                           'pDescriptorSets': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), customSize, pDescriptorSets)',
                                               'finalize_txt': 'default'},
                           'pUpdateChain': {'add_txt': 'add_update_descriptors_to_trace_packet(pHeader, (void**)&(pPacket->pUpdateChain), pUpdateChain)',
                                            'finalize_txt': 'default'},
                           'VkShaderCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkShaderCreateInfo), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), ((pCreateInfo != NULL) ? pCreateInfo->codeSize : 0), pCreateInfo->pCode)',
                                                      'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkFramebufferCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkFramebufferCreateInfo), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments), colorCount * sizeof(VkColorAttachmentBindInfo), pCreateInfo->pColorAttachments);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment), dsSize, pCreateInfo->pDepthStencilAttachment)',
                                                           'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkRenderPassCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkRenderPassCreateInfo), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps), colorCount * sizeof(VkAttachmentLoadOp), pCreateInfo->pColorLoadOps);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps), colorCount * sizeof(VkAttachmentStoreOp), pCreateInfo->pColorStoreOps);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues), colorCount * sizeof(VkClearColor), pCreateInfo->pColorLoadClearValues)',
                                                          'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkCmdBufferBeginInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo), sizeof(VkCmdBufferBeginInfo), pBeginInfo);\n    add_begin_cmdbuf_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo->pNext), pBeginInfo->pNext)',
                                                         'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBeginInfo))'},
                           'VkDynamicVpStateCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicVpStateCreateInfo), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(VkViewport), pCreateInfo->pViewports);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(VkRect), pCreateInfo->pScissors)',
                                                                'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkMemoryAllocInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(VkMemoryAllocInfo), pAllocInfo);\n    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext)',
                                                     'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocInfo))'},
                           'VkGraphicsPipelineCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkGraphicsPipelineCreateInfo), pCreateInfo);\n    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext)',
                                                                 'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkDescriptorPoolCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDescriptorPoolCreateInfo), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), pCreateInfo->count * sizeof(VkDescriptorTypeCount), pCreateInfo->pTypeCount)',
                                                                 'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'VkComputePipelineCreateInfo': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkComputePipelineCreateInfo), pCreateInfo);\n    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);\n    add_pipeline_shader_to_trace_packet(pHeader, (VkPipelineShader*)&pPacket->pCreateInfo->cs, &pCreateInfo->cs)',
                                                                'finalize_txt': 'finalize_pipeline_shader_address(pHeader, &pPacket->pCreateInfo->cs);\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                                                  }
        for p in params:
            pp_dict = {}
            if '*' in p.ty and p.name not in ['pSysMem', 'pReserved']:
                if 'const' in p.ty.lower() and 'count' in params[params.index(p)-1].name.lower():
                    pp_dict['add_txt'] = 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s*sizeof(%s), %s)' % (p.name, params[params.index(p)-1].name, p.ty.strip('*').replace('const ', ''), p.name)
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
    def _get_packet_size(self, params):
        ps = [] # List of elements to be added together to account for packet size for given params
        skip_list = [] # store params that are already accounted for so we don't count them twice
        # Dict of specific params with unique custom sizes
        custom_size_dict = {'pSetBindPoints': '(VK_SHADER_STAGE_COMPUTE * sizeof(uint32_t))', # Accounting for largest possible array
                            }
        for p in params:
            #First handle custom cases
            if p.name in ['pCreateInfo', 'pUpdateChain', 'pSetLayoutInfoList', 'pBeginInfo', 'pAllocInfo']:
                ps.append('get_struct_chain_size((void*)%s)' % p.name)
                skip_list.append(p.name)
            elif p.name in custom_size_dict:
                ps.append(custom_size_dict[p.name])
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
            elif '*' in p.ty and p.name not in ['pSysMem', 'pReserved']:
                if 'pData' == p.name:
                    if 'dataSize' == params[params.index(p)-1].name:
                        ps.append('dataSize')
                    elif 'counterCount' == params[params.index(p)-1].name:
                        ps.append('sizeof(%s)' % p.ty.strip('*').replace('const ', ''))
                    else:
                        ps.append('((pDataSize != NULL && pData != NULL) ? *pDataSize : 0)')
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
    # Call (w/ result?)
    # Assign packet values
    # FINISH packet
    # return result?
    def _generate_trace_funcs(self):
        func_body = []
        manually_written_hooked_funcs = ['CreateInstance', 'EnumerateLayers', 'EnumerateGpus',
                                         'AllocDescriptorSets', 'MapMemory', 'UnmapMemory',
                                         'CmdPipelineBarrier', 'CmdWaitEvents']
        for proto in self.protos:
            if proto.name in manually_written_hooked_funcs:
                func_body.append( '// __HOOKED_vk%s is manually written. Look in glvtrace_vk_trace.c\n' % proto.name)
            elif 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                raw_packet_update_list = [] # non-ptr elements placed directly into packet
                ptr_packet_update_list = [] # ptr elements to be updated into packet
                return_txt = ''
                packet_size = []
                in_data_size = False # flag when we need to capture local input size variable for in/out size
                func_body.append('GLVTRACER_EXPORT %s VKAPI __HOOKED_vk%s(' % (proto.ret, proto.name))
                for p in proto.params: # TODO : For all of the ptr types, check them for NULL and return 0 if NULL
                    if '[' in p.ty: # Correctly declare static arrays in function parameters
                        func_body.append('    %s %s[%s],' % (p.ty[:p.ty.find('[')], p.name, p.ty[p.ty.find('[')+1:p.ty.find(']')]))
                    else:
                        func_body.append('    %s %s,' % (p.ty, p.name))
                    if '*' in p.ty and p.name not in ['pSysMem', 'pReserved']:
                        if 'pDataSize' in p.name:
                            in_data_size = True;
                    else:
                        if '[' in p.ty:
                            array_str = p.ty[p.ty.find('[')+1:p.ty.find(']')]
                            raw_packet_update_list.append('    memcpy((void*)pPacket->color, color, %s * sizeof(%s));' % (array_str, p.ty.strip('*').replace('const ', '').replace('[%s]' % array_str, '')))
                        else:
                            raw_packet_update_list.append('    pPacket->%s = %s;' % (p.name, p.name))
                # Get list of packet size modifiers due to ptr params
                packet_size = self._get_packet_size(proto.params)
                ptr_packet_update_list = self._get_packet_ptr_param_list(proto.params)
                func_body[-1] = func_body[-1].replace(',', ')')
                # End of function declaration portion, begin function body
                func_body.append('{\n    glv_trace_packet_header* pHeader;')
                if 'void' not in proto.ret or '*' in proto.ret:
                    func_body.append('    %s result;' % proto.ret)
                    return_txt = 'result = '
                if in_data_size:
                    func_body.append('    size_t _dataSize;')
                func_body.append('    struct_vk%s* pPacket = NULL;' % proto.name)
                # functions that have non-standard sequence of  packet creation and calling real function
                # NOTE: Anytime we call the function before CREATE_TRACE_PACKET, need to add custom code for correctly tracking API call time
                if proto.name in ['CreateFramebuffer', 'CreateRenderPass', 'CreateDynamicViewportState',
                                  'CreateDescriptorRegion']:
                    # these are regular case as far as sequence of tracing but have some custom size element
                    if 'CreateFramebuffer' == proto.name:
                        func_body.append('    int dsSize = (pCreateInfo != NULL && pCreateInfo->pDepthStencilAttachment != NULL) ? sizeof(VkDepthStencilBindInfo) : 0;')
                        func_body.append('    uint32_t colorCount = (pCreateInfo != NULL && pCreateInfo->pColorAttachments != NULL) ? pCreateInfo->colorAttachmentCount : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(vkCreateFramebuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkFramebuffer));')
                    elif 'CreateRenderPass' == proto.name:
                        func_body.append('    uint32_t colorCount = (pCreateInfo != NULL && (pCreateInfo->pColorLoadOps != NULL || pCreateInfo->pColorStoreOps != NULL || pCreateInfo->pColorLoadClearValues != NULL)) ? pCreateInfo->colorAttachmentCount : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(vkCreateRenderPass, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkRenderPass));')
                    elif 'CreateDynamicViewportState' == proto.name:
                        func_body.append('    uint32_t vpsCount = (pCreateInfo != NULL && pCreateInfo->pViewports != NULL) ? pCreateInfo->viewportAndScissorCount : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(vkCreateDynamicViewportState,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicVpState));')
                    elif 'CreateDescriptorRegion' == proto.name:
                        func_body.append('    uint32_t rgCount = (pCreateInfo != NULL && pCreateInfo->pTypeCount != NULL) ? pCreateInfo->count : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(vkCreateDescriptorRegion,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDescriptorRegion));')
                    func_body.append('    %sreal_vk%s;' % (return_txt, proto.c_call()))
                else:
                    if (0 == len(packet_size)):
                        func_body.append('    CREATE_TRACE_PACKET(vk%s, 0);' % (proto.name))
                    else:
                        func_body.append('    CREATE_TRACE_PACKET(vk%s, %s);' % (proto.name, ' + '.join(packet_size)))
                    func_body.append('    %sreal_vk%s;' % (return_txt, proto.c_call()))
                if in_data_size:
                    func_body.append('    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;')
                func_body.append('    pPacket = interpret_body_as_vk%s(pHeader);' % proto.name)
                func_body.append('\n'.join(raw_packet_update_list))
                for pp_dict in ptr_packet_update_list: #buff_ptr_indices:
                    func_body.append('    %s;' % (pp_dict['add_txt']))
                if 'void' not in proto.ret or '*' in proto.ret:
                    func_body.append('    pPacket->result = result;')
                for pp_dict in ptr_packet_update_list:
                    if ('DeviceCreateInfo' not in proto.params[pp_dict['index']].ty) and ('APPLICATION_INFO' not in proto.params[pp_dict['index']].ty) and ('pUpdateChain' != proto.params[pp_dict['index']].name):
                        func_body.append('    %s;' % (pp_dict['finalize_txt']))
                func_body.append('    FINISH_TRACE_PACKET();')
                if 'AllocMemory' in proto.name:
                    func_body.append('    add_new_handle_to_mem_info(*pMem, pAllocInfo->allocationSize, NULL);')
                elif 'FreeMemory' in proto.name:
                    func_body.append('    rm_handle_from_mem_info(mem);')
                if 'void' not in proto.ret or '*' in proto.ret:
                    func_body.append('    return result;')
                func_body.append('}\n')
        return "\n".join(func_body)

    def _generate_trace_funcs_ext(self, func_class='Wsi'):
        thread_once_funcs = ['DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback', 'DbgSetGlobalOption']
        func_body = []
        for proto in self.protos:
            if func_class in proto.name:
                packet_update_txt = ''
                return_txt = ''
                packet_size = ''
                buff_ptr_indices = []
                func_body.append('GLVTRACER_EXPORT %s VKAPI __HOOKED_vk%s(' % (proto.ret, proto.name))
                for p in proto.params: # TODO : For all of the ptr types, check them for NULL and return 0 is NULL
                    func_body.append('    %s %s,' % (p.ty, p.name))
                    if 'Size' in p.name:
                        packet_size += p.name
                    if '*' in p.ty and 'pSysMem' != p.name:
                        if 'char' in p.ty:
                            packet_size += '((%s != NULL) ? strlen(%s) + 1 : 0) + ' % (p.name, p.name)
                        elif 'Size' not in packet_size:
                            packet_size += 'sizeof(%s) + ' % p.ty.strip('*').replace('const ', '')
                        buff_ptr_indices.append(proto.params.index(p))
                        if 'pConnectionInfo' in p.name:
                            packet_size += '((pConnectionInfo->pConnection != NULL) ? sizeof(void *) : 0)'
                    else:
                        packet_update_txt += '    pPacket->%s = %s;\n' % (p.name, p.name)
                if '' == packet_size:
                    packet_size = '0'
                else:
                    packet_size = packet_size.strip(' + ')
                func_body[-1] = func_body[-1].replace(',', ')')
                func_body.append('{\n    glv_trace_packet_header* pHeader;')
                if 'void' not in proto.ret or '*' in proto.ret:
                    func_body.append('    %s result;' % proto.ret)
                    return_txt = 'result = '
                func_body.append('    struct_vk%s* pPacket = NULL;' % proto.name)
                if proto.name in thread_once_funcs:
                    func_body.append('    glv_platform_thread_once(&gInitOnce, InitTracer);')
                func_body.append('    SEND_ENTRYPOINT_ID(vk%s);' % proto.name)
                if 'DbgRegisterMsgCallback' in proto.name:
                    func_body.append('    CREATE_TRACE_PACKET(vk%s, sizeof(char));' % proto.name)
                else:
                    func_body.append('    CREATE_TRACE_PACKET(vk%s, %s);' % (proto.name, packet_size))
                func_body.append('    %sreal_vk%s;' % (return_txt, proto.c_call()))
                func_body.append('    pPacket = interpret_body_as_vk%s(pHeader);' % proto.name)
                func_body.append(packet_update_txt.strip('\n'))
                for idx in buff_ptr_indices:
                    if 'char' in proto.params[idx].ty:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), ((%s != NULL) ? strlen(%s) + 1 : 0), %s);' % (proto.params[idx].name, proto.params[idx].name, proto.params[idx].name, proto.params[idx].name))
                    elif 'Size' in proto.params[idx-1].name:
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s, %s);' % (proto.params[idx].name, proto.params[idx-1].name, proto.params[idx].name))
                    elif 'DbgRegisterMsgCallback' in proto.name:
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(%s), %s);' % (proto.params[idx].name, 'char', proto.params[idx].name))
                    else:
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(%s), %s);' % (proto.params[idx].name, proto.params[idx].ty.strip('*').replace('const ', ''), proto.params[idx].name))
                if 'WsiX11AssociateConnection' in proto.name:
                    func_body.append('    if (pConnectionInfo->pConnection != NULL) {')
                    func_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection), sizeof(void *), pConnectionInfo->pConnection);')
                    func_body.append('        glv_finalize_buffer_address(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection));')
                    func_body.append('    }')
                if 'void' not in proto.ret or '*' in proto.ret:
                    func_body.append('    pPacket->result = result;')
                for idx in buff_ptr_indices:
                    func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s));' % (proto.params[idx].name))
                func_body.append('    FINISH_TRACE_PACKET();')
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

    def _generate_stringify_func(self):
        func_body = []
        func_body.append('static const char *stringify_vk_packet_id(const enum GLV_TRACE_PACKET_ID_VK id, const glv_trace_packet_header* pHeader)')
        func_body.append('{')
        func_body.append('    static char str[1024];')
        func_body.append('    switch(id) {')
        func_body.append('    case GLV_TPI_VK_vkApiVersion:')
        func_body.append('    {')
        func_body.append('        struct_vkApiVersion* pPacket = (struct_vkApiVersion*)(pHeader->pBody);')
        func_body.append('        snprintf(str, 1024, "vkApiVersion = 0x%x", pPacket->version);')
        func_body.append('        return str;')
        func_body.append('    }')
        for proto in self.protos:
            func_body.append('    case GLV_TPI_VK_vk%s:' % proto.name)
            func_body.append('    {')
            func_str = 'vk%s(' % proto.name
            print_vals = ''
            create_func = False
            if 'Create' in proto.name or 'Alloc' in proto.name or 'MapMemory' in proto.name:
                create_func = True
            for p in proto.params:
                last_param = False
                if (p.name == proto.params[-1].name):
                    last_param = True
                if last_param and create_func: # last param of create func
                    (pft, pfi, ptr) = self._get_printf_params(p.ty,'pPacket->%s' % p.name, True)
                else:
                    (pft, pfi, ptr) = self._get_printf_params(p.ty, 'pPacket->%s' % p.name, False)
                if last_param == True:
                    func_str += '%s%s = %s)' % (ptr, p.name, pft)
                    print_vals += ', %s' % (pfi)
                else:
                    func_str += '%s%s = %s, ' % (ptr, p.name, pft)
                    print_vals += ', %s' % (pfi)
            func_body.append('        struct_vk%s* pPacket = (struct_vk%s*)(pHeader->pBody);' % (proto.name, proto.name))
            func_body.append('        snprintf(str, 1024, "%s"%s);' % (func_str, print_vals))
            func_body.append('        return str;')
            func_body.append('    }')
        func_body.append('    default:')
        func_body.append('        return NULL;')
        func_body.append('    }')
        func_body.append('};\n')
        return "\n".join(func_body)

    def _generate_interp_func(self):
        interp_func_body = []
        interp_func_body.append('static glv_trace_packet_header* interpret_trace_packet_vk(glv_trace_packet_header* pHeader)')
        interp_func_body.append('{')
        interp_func_body.append('    if (pHeader == NULL)')
        interp_func_body.append('    {')
        interp_func_body.append('        return NULL;')
        interp_func_body.append('    }')
        interp_func_body.append('    switch (pHeader->packet_id)')
        interp_func_body.append('    {')
        interp_func_body.append('        case GLV_TPI_VK_vkApiVersion:\n        {')
        interp_func_body.append('            return interpret_body_as_vkApiVersion(pHeader, TRUE)->header;\n        }')
        for proto in self.protos:
            interp_func_body.append('        case GLV_TPI_VK_vk%s:\n        {' % proto.name)
            header_prefix = 'h'
            if 'Wsi' in proto.name or 'Dbg' in proto.name:
                header_prefix = 'pH'
            interp_func_body.append('            return interpret_body_as_vk%s(pHeader)->%seader;\n        }' % (proto.name, header_prefix))
        interp_func_body.append('        default:')
        interp_func_body.append('            return NULL;')
        interp_func_body.append('    }')
        interp_func_body.append('    return NULL;')
        interp_func_body.append('}')
        return "\n".join(interp_func_body)

    def _generate_struct_util_funcs(self):
        pid_enum = []
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
        pid_enum.append('//=============================================================================\n')
        # TODO : Create add_VkInstanceCreateInfo_to_packet() function here - used in glvtrace_vk_trace.c = add_VkInstanceCreateInfo_to_packet(pHeader, (VkInstanceCreateInfo**)&(pPacket->pCreateInfo), pCreateInfo);
        pid_enum.append('static void add_VkDeviceCreateInfo_to_packet(glv_trace_packet_header*  pHeader, VkDeviceCreateInfo** ppStruct, const VkDeviceCreateInfo *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(VkDeviceCreateInfo), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pRequestedQueues, pInStruct->queueRecordCount*sizeof(VkDeviceQueueCreateInfo), pInStruct->pRequestedQueues);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&(*ppStruct)->pRequestedQueues);')
        pid_enum.append('    if (pInStruct->extensionCount > 0) ')
        pid_enum.append('    {')
        pid_enum.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledExtensionNames), pInStruct->extensionCount * sizeof(char *), pInStruct->ppEnabledExtensionNames);')
        pid_enum.append('        for (i = 0; i < pInStruct->extensionCount; i++)')
        pid_enum.append('        {')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])), strlen(pInStruct->ppEnabledExtensionNames[i]) + 1, pInStruct->ppEnabledExtensionNames[i]);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])));')
        pid_enum.append('        }')
        pid_enum.append('        glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames);')
        pid_enum.append('    }')
        pid_enum.append('    VkLayerCreateInfo *pNext = ( VkLayerCreateInfo *) pInStruct->pNext;')
        pid_enum.append('    while (pNext != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        if ((pNext->sType == VK_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pNext->layerCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppStruct)->pNext)), sizeof(VkLayerCreateInfo), pNext);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void**)(&((*ppStruct)->pNext)));')
        pid_enum.append('            VkLayerCreateInfo **ppOutStruct = (VkLayerCreateInfo **) &((*ppStruct)->pNext);')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppOutStruct)->ppActiveLayerNames), pNext->layerCount * sizeof(char *), pNext->ppActiveLayerNames);')
        pid_enum.append('            for (i = 0; i < pNext->layerCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppOutStruct)->ppActiveLayerNames[i])), strlen(pNext->ppActiveLayerNames[i]) + 1, pNext->ppActiveLayerNames[i]);')
        pid_enum.append('                glv_finalize_buffer_address(pHeader, (void**)(&((*ppOutStruct)->ppActiveLayerNames[i])));')
        pid_enum.append('            }')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void **)&(*ppOutStruct)->ppActiveLayerNames);')
        pid_enum.append('        }')
        pid_enum.append('        pNext = ( VkLayerCreateInfo *) pNext->pNext;')
        pid_enum.append('    }')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)ppStruct);')
        pid_enum.append('}\n')
        pid_enum.append('static VkDeviceCreateInfo* interpret_VkDeviceCreateInfo(glv_trace_packet_header*  pHeader, intptr_t ptr_variable)')
        pid_enum.append('{')
        pid_enum.append('    VkDeviceCreateInfo* pVkDeviceCreateInfo = (VkDeviceCreateInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)ptr_variable);\n')
        pid_enum.append('    if (pVkDeviceCreateInfo != NULL)')
        pid_enum.append('    {')
        pid_enum.append('            uint32_t i;')
        pid_enum.append('            const char** pNames;')
        pid_enum.append('        pVkDeviceCreateInfo->pRequestedQueues = (const VkDeviceQueueCreateInfo *)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->pRequestedQueues);\n')
        pid_enum.append('        if (pVkDeviceCreateInfo->extensionCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            pVkDeviceCreateInfo->ppEnabledExtensionNames = (const char *const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->ppEnabledExtensionNames);')
        pid_enum.append('            pNames = (const char**)pVkDeviceCreateInfo->ppEnabledExtensionNames;')
        pid_enum.append('            for (i = 0; i < pVkDeviceCreateInfo->extensionCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                pNames[i] = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pVkDeviceCreateInfo->ppEnabledExtensionNames[i]));')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('        VkLayerCreateInfo *pNext = ( VkLayerCreateInfo *) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVkDeviceCreateInfo->pNext);')
        pid_enum.append('        while (pNext != NULL)')
        pid_enum.append('        {')
        pid_enum.append('            if ((pNext->sType == VK_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pNext->layerCount > 0)')
        pid_enum.append('            {')
        pid_enum.append('                pNext->ppActiveLayerNames = (const char**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pNext->ppActiveLayerNames));')
        pid_enum.append('                pNames = (const char**)pNext->ppActiveLayerNames;')
        pid_enum.append('                for (i = 0; i < pNext->layerCount; i++)')
        pid_enum.append('                {')
        pid_enum.append('                    pNames[i] = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pNext->ppActiveLayerNames[i]));')
        pid_enum.append('                }')
        pid_enum.append('            }')
        pid_enum.append('            pNext = ( VkLayerCreateInfo *) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);')
        pid_enum.append('        }')
        pid_enum.append('    }\n')
        pid_enum.append('    return pVkDeviceCreateInfo;')
        pid_enum.append('}\n')
        pid_enum.append('static void interpret_pipeline_shader(glv_trace_packet_header*  pHeader, VkPipelineShader* pShader)')
        pid_enum.append('{')
        pid_enum.append('    if (pShader != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        // constant buffers')
        pid_enum.append('        if (pShader->linkConstBufferCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            uint32_t i;')
        pid_enum.append('            pShader->pLinkConstBufferInfo = (const VkLinkConstBuffer*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo);')
        pid_enum.append('            for (i = 0; i < pShader->linkConstBufferCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                VkLinkConstBuffer* pBuffer = (VkLinkConstBuffer*)pShader->pLinkConstBufferInfo;')
        pid_enum.append('                pBuffer[i].pBufferData = (const void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo[i].pBufferData);')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('    }')
        pid_enum.append('}\n')
        pid_enum.append('//=============================================================================')
        return "\n".join(pid_enum)

    # Interpret functions used on replay to read in packets and interpret their contents
    #  This code gets generated into glv_vk_vk_structs.h file
    def _generate_interp_funcs(self):
        # Custom txt for given function and parameter.  First check if param is NULL, then insert txt if not
        # TODO : This code is now too large and complex, need to make codegen smarter for pointers embedded in struct params to handle those cases automatically
        custom_case_dict = { 'CreateInstance' : {'param': 'pAppInfo', 'txt': ['VkApplicationInfo* pInfo = (VkApplicationInfo*)pPacket->pAppInfo;\n',
                                                       'pInfo->pAppName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pAppName);\n',
                                                       'pInfo->pEngineName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pEngineName);']},
                             'CreateShader' : {'param': 'pCreateInfo', 'txt': ['VkShaderCreateInfo* pInfo = (VkShaderCreateInfo*)pPacket->pCreateInfo;\n',
                                               'pInfo->pCode = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pCode);']},
                             'CreateDynamicViewportState' : {'param': 'pCreateInfo', 'txt': ['VkDynamicVpStateCreateInfo* pInfo = (VkDynamicVpStateCreateInfo*)pPacket->pCreateInfo;\n',
                                                                                             'pInfo->pViewports = (VkViewport*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pViewports);\n',
                                                                                             'pInfo->pScissors = (VkRect*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pScissors);']},
                             'CreateFramebuffer' : {'param': 'pCreateInfo', 'txt': ['VkFramebufferCreateInfo* pInfo = (VkFramebufferCreateInfo*)pPacket->pCreateInfo;\n',
                                                    'pInfo->pColorAttachments = (VkColorAttachmentBindInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorAttachments);\n',
                                                    'pInfo->pDepthStencilAttachment = (VkDepthStencilBindInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pDepthStencilAttachment);\n']},
                             'CreateRenderPass' : {'param': 'pCreateInfo', 'txt': ['VkRenderPassCreateInfo* pInfo = (VkRenderPassCreateInfo*)pPacket->pCreateInfo;\n',
                                                   'pInfo->pColorLoadOps = (VkAttachmentLoadOp*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorLoadOps);\n',
                                                   'pInfo->pColorStoreOps = (VkAttachmentStoreOp*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorStoreOps);\n',
                                                   'pInfo->pColorLoadClearValues = (VkClearColor*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorLoadClearValues);\n']},
                             'CreateDescriptorPool' : {'param': 'pCreateInfo', 'txt': ['VkDescriptorPoolCreateInfo* pInfo = (VkDescriptorPoolCreateInfo*)pPacket->pCreateInfo;\n',
                                                                                             'pInfo->pTypeCount = (VkDescriptorTypeCount*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pTypeCount);\n']},
                             'CmdWaitEvents' : {'param': 'pWaitInfo', 'txt': ['VkEventWaitInfo* pInfo = (VkEventWaitInfo*)pPacket->pWaitInfo;\n',
                                                                          'pInfo->pEvents = (VkEvent*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pWaitInfo->pEvents);\n',
                                                                          'pInfo->ppMemBarriers = (const void**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pWaitInfo->ppMemBarriers);\n',
                                                                          'uint32_t i;\n',
                                                                          'for (i = 0; i < pInfo->memBarrierCount; i++) {\n',
                                                                          '    void** ppLocalMemBarriers = (void**)&pInfo->ppMemBarriers[i];\n',
                                                                          '    *ppLocalMemBarriers = (void*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->ppMemBarriers[i]);\n',
                                                                          '}']},
                             'CmdPipelineBarrier' : {'param': 'pBarrier', 'txt': ['VkPipelineBarrier* pBarrier = (VkPipelineBarrier*)pPacket->pBarrier;\n',
                                                                          'pBarrier->pEvents = (VkPipeEvent*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBarrier->pEvents);\n',
                                                                          'pBarrier->ppMemBarriers = (const void**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBarrier->ppMemBarriers);\n',
                                                                          'uint32_t i;\n',
                                                                          'for (i = 0; i < pBarrier->memBarrierCount; i++) {\n',
                                                                          '    void** ppLocalMemBarriers = (void**)&pBarrier->ppMemBarriers[i];\n',
                                                                          '    *ppLocalMemBarriers = (void*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pBarrier->ppMemBarriers[i]);\n',
                                                                          '}']},
                             'CreateDescriptorSetLayout' : {'param': 'pSetLayoutInfoList', 'txt': ['if (pPacket->pSetLayoutInfoList->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {\n',
                                                                                         '    // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         '    void** ppNextVoidPtr = (void**)&(pPacket->pSetLayoutInfoList->pNext);\n',
                                                                                         '    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetLayoutInfoList->pNext);\n',
                                                                                         '    VkDescriptorSetLayoutCreateInfo* pNext = (VkDescriptorSetLayoutCreateInfo*)pPacket->pSetLayoutInfoList->pNext;\n',
                                                                                         '    while (NULL != pNext)\n', '    {\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:\n',
                                                                                         '            {\n' ,
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '                glv_LogError("Encountered an unexpected type in descriptor set layout create list.\\n");\n',
                                                                                         '                pPacket->header = NULL;\n',
                                                                                         '                pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (VkDescriptorSetLayoutCreateInfo*)pNext->pNext;\n',
                                                                                         '     }\n',
                                                                                         '} else {\n',
                                                                                         '     // This is unexpected.\n',
                                                                                         '     glv_LogError("CreateDescriptorSetLayout must have LayoutInfoList stype of VK_STRCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO\\n");\n',
                                                                                         '     pPacket->header = NULL;\n',
                                                                                         '}']},
                             'BeginCommandBuffer' : {'param': 'pBeginInfo', 'txt': ['if (pPacket->pBeginInfo->sType == VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO) {\n',
                                                                                         '    // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         '    VkCmdBufferGraphicsBeginInfo** ppNext = (VkCmdBufferGraphicsBeginInfo**)&(pPacket->pBeginInfo->pNext);\n',
                                                                                         '    *ppNext = (VkCmdBufferGraphicsBeginInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBeginInfo->pNext);\n',
                                                                                         '    VkCmdBufferGraphicsBeginInfo* pNext = *ppNext;\n',
                                                                                         '    while (NULL != pNext)\n', '    {\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            case VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                ppNext = (VkCmdBufferGraphicsBeginInfo**) &pNext->pNext;\n',
                                                                                         '                *ppNext = (VkCmdBufferGraphicsBeginInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '                glv_LogError("Encountered an unexpected type in begin command buffer list.\\n");\n',
                                                                                         '                pPacket->header = NULL;\n',
                                                                                         '                pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (VkCmdBufferGraphicsBeginInfo*)pNext->pNext;\n',
                                                                                         '    }\n',
                                                                                         '} else {\n',
                                                                                         '    // This is unexpected.\n',
                                                                                         '    glv_LogError("BeginCommandBuffer must have BeginInfo stype of VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO.\\n");\n',
                                                                                         '    pPacket->header = NULL;\n',
                                                                                         '}']},
                             'AllocMemory' : {'param': 'pAllocInfo', 'txt': ['if (pPacket->pAllocInfo->sType == VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO) {\n',
                                                                                         '    VkMemoryAllocInfo** ppNext = (VkMemoryAllocInfo**) &(pPacket->pAllocInfo->pNext);\n',
                                                                                         '    *ppNext = (VkMemoryAllocInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocInfo->pNext);\n',
                                                                                         '    VkMemoryAllocInfo* pNext = (VkMemoryAllocInfo*) *ppNext;\n',
                                                                                         '    while (NULL != pNext)\n', '    {\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            case VK_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:\n',
                                                                                         '            case VK_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                ppNext = (VkMemoryAllocInfo **) &(pNext->pNext);\n',
                                                                                         '                *ppNext = (VkMemoryAllocInfo*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '               glv_LogError("Encountered an unexpected type alloc memory list.\\n");\n',
                                                                                         '               pPacket->header = NULL;\n',
                                                                                         '               pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (VkMemoryAllocInfo*)pNext->pNext;\n',
                                                                                         '    }\n',
                                                                                         '} else {\n',
                                                                                         '    // This is unexpected.\n',
                                                                                         '    glv_LogError("AllocMemory must have AllocInfo stype of VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO.\\n");\n',
                                                                                         '    pPacket->header = NULL;\n',
                                                                                         '}']},
                             'UpdateDescriptors' : {'param': 'pUpdateChain', 'txt': ['VkUpdateSamplers* pNext = (VkUpdateSamplers*)pPacket->pUpdateChain;\n',
                                                                                         'while ((NULL != pNext) && (VK_NULL_HANDLE != pNext))\n', '{\n',
                                                                                         '    switch(pNext->sType)\n', '    {\n',
                                                                                         '        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            VkUpdateSamplers* pUS = (VkUpdateSamplers*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            pUS->pSamplers = (VK_SAMPLER*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUS->pSamplers);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            VkUpdateSamplerTextures* pUST = (VkUpdateSamplerTextures*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            pUST->pSamplerImageViews = (VK_SAMPLER_IMAGE_VIEW_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUST->pSamplerImageViews);\n',
                                                                                         '            uint32_t i;\n',
                                                                                         '            for (i = 0; i < pUST->count; i++) {\n',
                                                                                         '                VkImageViewAttachInfo** ppLocalImageView = (VkImageViewAttachInfo**)&pUST->pSamplerImageViews[i].pImageView;\n',
                                                                                         '                *ppLocalImageView = (VkImageViewAttachInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUST->pSamplerImageViews[i].pImageView);\n',
                                                                                         '            }\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            VkUpdateImages* pUI = (VkUpdateImages*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            VkImageViewAttachInfo** ppLocalImageView = (VkImageViewAttachInfo**)&pUI->pImageViews;\n',
                                                                                         '            *ppLocalImageView = (VkImageViewAttachInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUI->pImageViews);\n',
                                                                                         '            uint32_t i;\n',
                                                                                         '            for (i = 0; i < pUI->count; i++) {\n',
                                                                                         '                VkImageViewAttachInfo** ppLocalImageViews = (VkImageViewAttachInfo**)&pUI->pImageViews[i];\n',
                                                                                         '                *ppLocalImageViews = (VkImageViewAttachInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUI->pImageViews[i]);\n',
                                                                                         '            }\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            VkUpdateBuffers* pUB = (VkUpdateBuffers*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            VkBufferViewAttachInfo** ppLocalBufferView = (VkBufferViewAttachInfo**)&pUB->pBufferViews;\n',
                                                                                         '            *ppLocalBufferView = (VkBufferViewAttachInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUB->pBufferViews);\n',
                                                                                         '            uint32_t i;\n',
                                                                                         '            for (i = 0; i < pUB->count; i++) {\n',
                                                                                         '                VkBufferViewAttachInfo** ppLocalBufferViews = (VkBufferViewAttachInfo**)&pUB->pBufferViews[i];\n',
                                                                                         '                *ppLocalBufferViews = (VkBufferViewAttachInfo*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUB->pBufferViews[i]);\n',
                                                                                         '            }\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        default:\n',
                                                                                         '        {\n',
                                                                                         '           glv_LogError("Encountered an unexpected type in update descriptors pUpdateChain.\\n");\n',
                                                                                         '           pPacket->header = NULL;\n',
                                                                                         '           pNext->pNext = NULL;\n',
                                                                                         '        }\n',
                                                                                         '    }\n',
                                                                                         '    pNext = (VkUpdateSamplers*)pNext->pNext;\n',
                                                                                         '}']},
                             'CreateGraphicsPipeline' : {'param': 'pCreateInfo', 'txt': ['if (pPacket->pCreateInfo->sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {\n',
                                                                                         '    // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         '    void** ppNextVoidPtr = (void**)&pPacket->pCreateInfo->pNext;\n',
                                                                                         '    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pNext);\n',
                                                                                         '    VkPipelineShaderStageCreateInfo* pNext = (VkPipelineShaderStageCreateInfo*)pPacket->pCreateInfo->pNext;\n',
                                                                                         '    while ((NULL != pNext) && (VK_NULL_HANDLE != pNext))\n', '{\n',
                                                                                         '        switch(pNext->sType)\n', '    {\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                VkPipelineCbStateCreateInfo *pCb = (VkPipelineCbStateCreateInfo *) pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                pCb->pAttachments = (VkPipelineCbAttachmentState*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pCb->pAttachments);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                interpret_pipeline_shader(pHeader, &pNext->shader);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                VkPipelineVertexInputCreateInfo *pVi = (VkPipelineVertexInputCreateInfo *) pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                pVi->pVertexBindingDescriptions = (VkVertexInputBindingDescription*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexBindingDescriptions);\n',
                                                                                         '                pVi->pVertexAttributeDescriptions = (VkVertexInputAttributeDescription*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexAttributeDescriptions);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '               glv_LogError("Encountered an unexpected type in pipeline state list.\\n");\n',
                                                                                         '               pPacket->header = NULL;\n',
                                                                                         '               pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (VkPipelineShaderStageCreateInfo*)pNext->pNext;\n',
                                                                                         '    }\n',
                                                                                         '} else {\n',
                                                                                         '    // This is unexpected.\n',
                                                                                         '    glv_LogError("CreateGraphicsPipeline must have CreateInfo stype of VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO.\\n");\n',
                                                                                         '    pPacket->header = NULL;\n',
                                                                                         '}']},
                             'CreateComputePipeline' : {'param': 'pCreateInfo', 'txt': ['interpret_pipeline_shader(pHeader, (VkPipelineShader*)(&pPacket->pCreateInfo->cs));']}}
        if_body = []
        if_body.append('typedef struct struct_vkApiVersion {')
        if_body.append('    glv_trace_packet_header* header;')
        if_body.append('    uint32_t version;')
        if_body.append('} struct_vkApiVersion;\n')
        if_body.append('static struct_vkApiVersion* interpret_body_as_vkApiVersion(glv_trace_packet_header* pHeader, BOOL check_version)')
        if_body.append('{')
        if_body.append('    struct_vkApiVersion* pPacket = (struct_vkApiVersion*)pHeader->pBody;')
        if_body.append('    pPacket->header = pHeader;')
        if_body.append('    if (check_version && pPacket->version != VK_API_VERSION)')
        if_body.append('        glv_LogError("Trace file from older VK version 0x%x, vk replayer built from version 0x%x, replayer may fail\\n", pPacket->version, VK_API_VERSION);')
        if_body.append('    return pPacket;')
        if_body.append('}\n')
        for proto in self.protos:
            if 'Wsi' not in proto.name and 'Dbg' not in proto.name:
                if 'UnmapMemory' == proto.name:
                    proto.params.append(vulkan.Param("void*", "pData"))
                if_body.append('typedef struct struct_vk%s {' % proto.name)
                if_body.append('    glv_trace_packet_header* header;')
                for p in proto.params:
                    if '[4]' in p.ty:
                        if_body.append('    %s %s[4];' % (p.ty.strip('[4]'), p.name))
                    else:
                        if_body.append('    %s %s;' % (p.ty, p.name))
                if 'void' != proto.ret:
                    if_body.append('    %s result;' % proto.ret)
                if_body.append('} struct_vk%s;\n' % proto.name)
                if_body.append('static struct_vk%s* interpret_body_as_vk%s(glv_trace_packet_header* pHeader)' % (proto.name, proto.name))
                if_body.append('{')
                if_body.append('    struct_vk%s* pPacket = (struct_vk%s*)pHeader->pBody;' % (proto.name, proto.name))
                if_body.append('    pPacket->header = pHeader;')
                for p in proto.params:
                    if '*' in p.ty:
                        if 'DeviceCreateInfo' in p.ty:
                            if_body.append('    pPacket->%s = interpret_VkDeviceCreateInfo(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.name))
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

    def _generate_interp_funcs_ext(self, func_class='Wsi'):
        if_body = []
        for proto in self.protos:
            if func_class in proto.name:
                if_body.append('typedef struct struct_vk%s {' % proto.name)
                if_body.append('    glv_trace_packet_header* pHeader;')
                for p in proto.params:
                    if_body.append('    %s %s;' % (p.ty, p.name))
                if 'void' != proto.ret:
                    if_body.append('    %s result;' % proto.ret)
                if_body.append('} struct_vk%s;\n' % proto.name)
                if_body.append('static struct_vk%s* interpret_body_as_vk%s(glv_trace_packet_header* pHeader)' % (proto.name, proto.name))
                if_body.append('{')
                if_body.append('    struct_vk%s* pPacket = (struct_vk%s*)pHeader->pBody;' % (proto.name, proto.name))
                if_body.append('    pPacket->pHeader = pHeader;')
                for p in proto.params:
                    if '*' in p.ty:
                        if_body.append('    pPacket->%s = (%s)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.ty, p.name))
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
                if '[4]' in p.ty:
                    xf_body.append('        %s %s[4],' % (p.ty.strip('[4]'), p.name))
                else:
                    xf_body.append('        %s %s,' % (p.ty, p.name))
            xf_body[-1] = xf_body[-1].replace(',', ');')
            xf_body.append('    type_vk%s real_vk%s;' % (proto.name, proto.name))
        xf_body.append('};')
        return "\n".join(xf_body)

    def _map_decl(self, type1, type2, name):
        return '    std::map<%s, %s> %s;' % (type1, type2, name)

    def _add_to_map_decl(self, type1, type2, name):
        txt = '    void add_to_map(%s* pTraceVal, %s* pReplayVal)\n    {\n' % (type1, type2)
        txt += '        assert(pTraceVal != NULL);\n'
        txt += '        assert(pReplayVal != NULL);\n'
        txt += '        %s[*pTraceVal] = *pReplayVal;\n    }\n' % name
        return txt

    def _rm_from_map_decl(self, ty, name):
        txt = '    void rm_from_map(const %s& key)\n    {\n' % (ty)
        txt += '        %s.erase(key);\n    }\n' % name
        return txt

    def _remap_decl(self, ty, name):
        txt = '    %s remap(const %s& value)\n    {\n' % (ty, ty)
        txt += '        std::map<%s, %s>::const_iterator q = %s.find(value);\n' % (ty, ty, name)
        txt += '        return (q == %s.end()) ? VK_NULL_HANDLE : q->second;\n    }\n' % name
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
        rof_body.append('            glv_LogError("gpuMemory::setMemoryDataAddr() m_mapRange is empty\\n");')
        rof_body.append('            return;')
        rof_body.append('        }')
        rof_body.append('        MapRange mr = m_mapRange.back();')
        rof_body.append('        if (mr.pData != NULL)')
        rof_body.append('            glv_LogWarn("gpuMemory::setMemoryDataAddr() data already mapped overwrite old mapping\\n");')
        rof_body.append('        else if (pBuf == NULL)')
        rof_body.append('            glv_LogWarn("gpuMemory::setMemoryDataAddr() adding NULL pointer\\n");')
        rof_body.append('        mr.pData = pBuf;')
        rof_body.append('    }')
        rof_body.append('')
        rof_body.append('    void setMemoryMapRange(void *pBuf, const size_t size, const size_t offset, const bool pending)')
        rof_body.append('    {')
        rof_body.append('        MapRange mr;')
        rof_body.append('        mr.pData = pBuf;')
        rof_body.append('        mr.size = size;')
        rof_body.append('        mr.offset = offset;')
        rof_body.append('        mr.pending = pending;')
        rof_body.append('        m_mapRange.push_back(mr);')
        rof_body.append('    }')
        rof_body.append('')
        rof_body.append('    void copyMappingData(const void* pSrcData)')
        rof_body.append('    {')
        rof_body.append('        if (m_mapRange.empty())')
        rof_body.append('        {')
        rof_body.append('            glv_LogError("gpuMemory::copyMappingData() m_mapRange is empty\\n");')
        rof_body.append('            return;')
        rof_body.append('        }')
        rof_body.append('        MapRange mr = m_mapRange.back();')
        rof_body.append('        if (!pSrcData || !mr.pData)')
        rof_body.append('        {')
        rof_body.append('            if (!pSrcData)')
        rof_body.append('                glv_LogError("gpuMemory::copyMappingData() null src pointer\\n");')
        rof_body.append('            else')
        rof_body.append('                glv_LogError("gpuMemory::copyMappingData() null dest pointer size=%u\\n", m_allocInfo.allocationSize);')
        rof_body.append('            m_mapRange.pop_back();')
        rof_body.append('            return;')
        rof_body.append('        }')
        rof_body.append('        memcpy(mr.pData, pSrcData, m_allocInfo.allocationSize);')
        rof_body.append('        if (!mr.pending)')
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
        for ty in vulkan.object_type_list:
            if ty in vulkan.object_parent_list:
                continue
            if (ty.startswith('Vk')):
                mem_var = ty.replace('Vk', '').lower()
            mem_var_list = mem_var.split('_')
            mem_var = 'm_%s%ss' % (mem_var_list[0], "".join([m.title() for m in mem_var_list[1:]]))
            obj_map_dict[mem_var] = ty
        rc_body = []
        rc_body.append('typedef struct _VKAllocInfo {')
        rc_body.append('    VkGpuSize size;')
        rc_body.append('    void *pData;')
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
        rc_body.append('            glv_LogError("objMemory::setReqs, internal mismatch on number of allocations");')
        rc_body.append('        if (m_pMemReqs == NULL && pReqs != NULL)')
        rc_body.append('        {')
        rc_body.append('            m_pMemReqs = (VkMemoryRequirements *) glv_malloc(num * sizeof(VkMemoryRequirements));')
        rc_body.append('            if (m_pMemReqs == NULL)')
        rc_body.append('            {')
        rc_body.append('                glv_LogError("objMemory::setReqs out of memory");')
        rc_body.append('                return;')
        rc_body.append('            }')
        rc_body.append('            memcpy(m_pMemReqs, pReqs, num);')
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
        rc_body.append('        void* pData;')
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
        rc_body.append('     VkGpuMemory replayGpuMem;')
        rc_body.append(' } gpuMemObj;')
        rc_body.append('')
        rc_body.append('class vkReplayObjMapper {')
        rc_body.append('public:')
        rc_body.append('    vkReplayObjMapper() {}')
        rc_body.append('    ~vkReplayObjMapper() {}')
        rc_body.append('')
        rc_body.append(' bool m_adjustForGPU; // true if replay adjusts behavior based on GPU')
        # Code for memory objects for handling replay GPU != trace GPU object memory requirements
        rc_body.append(' void init_objMemCount(const VkBaseObject& object, const uint32_t &num)\n {')
        rc_body.append('     VkImage img = static_cast <VkImage> (object);')
        rc_body.append('     std::map<VkImage, imageObj>::const_iterator it = m_images.find(img);')
        rc_body.append('     if (it != m_images.end())')
        rc_body.append('     {')
        rc_body.append('         objMemory obj = it->second.imageMem;')
        rc_body.append('         obj.setCount(num);')
        rc_body.append('         return;')
        rc_body.append('     }')
        rc_body.append('     VkBuffer buf = static_cast <VkBuffer> (object);')
        rc_body.append('     std::map<VkBuffer, bufferObj>::const_iterator itb = m_buffers.find(buf);')
        rc_body.append('     if (itb != m_buffers.end())')
        rc_body.append('     {')
        rc_body.append('         objMemory obj = itb->second.bufferMem;')
        rc_body.append('         obj.setCount(num);')
        rc_body.append('         return;')
        rc_body.append('     }')
        rc_body.append('     return;')
        rc_body.append(' }\n')
        rc_body.append('    void init_objMemReqs(const VkBaseObject& object, const VkMemoryRequirements *pMemReqs, const unsigned int num)\n    {')
        rc_body.append('        VkImage img = static_cast <VkImage> (object);')
        rc_body.append('        std::map<VkImage, imageObj>::const_iterator it = m_images.find(img);')
        rc_body.append('        if (it != m_images.end())')
        rc_body.append('        {')
        rc_body.append('            objMemory obj = it->second.imageMem;')
        rc_body.append('            obj.setReqs(pMemReqs, num);')
        rc_body.append('            return;')
        rc_body.append('        }')
        rc_body.append('        VkBuffer buf = static_cast <VkBuffer> (object);')
        rc_body.append('        std::map<VkBuffer, bufferObj>::const_iterator itb = m_buffers.find(buf);')
        rc_body.append('        if (itb != m_buffers.end())')
        rc_body.append('        {')
        rc_body.append('            objMemory obj = itb->second.bufferMem;')
        rc_body.append('            obj.setReqs(pMemReqs, num);')
        rc_body.append('            return;')
        rc_body.append('        }')
        rc_body.append('        return;')
        rc_body.append('    }')
        rc_body.append('')
        rc_body.append('    void clear_all_map_handles()\n    {')
        for var in sorted(obj_map_dict):
            rc_body.append('        %s.clear();' % var)
        rc_body.append('    }\n')
        for var in sorted(obj_map_dict):
            if obj_map_dict[var] == 'VkImage':
                rc_body.append(self._map_decl(obj_map_dict[var], 'imageObj', var))
                rc_body.append(self._add_to_map_decl(obj_map_dict[var], 'imageObj', var))
                rc_body.append(self._rm_from_map_decl(obj_map_dict[var], var))
                rc_body.append('    VkImage remap(const VkImage& value)')
                rc_body.append('    {')
                rc_body.append('        std::map<VkImage, imageObj>::const_iterator q = m_images.find(value);')
                rc_body.append('        return (q == m_images.end()) ? VK_NULL_HANDLE : q->second.replayImage;')
                rc_body.append('    }\n')
            elif obj_map_dict[var] == 'VkBuffer':
                rc_body.append(self._map_decl(obj_map_dict[var], 'bufferObj', var))
                rc_body.append(self._add_to_map_decl(obj_map_dict[var], 'bufferObj', var))
                rc_body.append(self._rm_from_map_decl(obj_map_dict[var], var))
                rc_body.append('    VkBuffer remap(const VkBuffer& value)')
                rc_body.append('    {')
                rc_body.append('        std::map<VkBuffer, bufferObj>::const_iterator q = m_buffers.find(value);')
                rc_body.append('        return (q == m_buffers.end()) ? VK_NULL_HANDLE : q->second.replayBuffer;')
                rc_body.append('    }\n')
            elif obj_map_dict[var] == 'VkGpuMemory':
                rc_body.append(self._map_decl(obj_map_dict[var], 'gpuMemObj', var))
                rc_body.append(self._add_to_map_decl(obj_map_dict[var], 'gpuMemObj', var))
                rc_body.append(self._rm_from_map_decl(obj_map_dict[var], var))
                rc_body.append('    VkGpuMemory remap(const VkGpuMemory& value)')
                rc_body.append('    {')
                rc_body.append('        std::map<VkGpuMemory, gpuMemObj>::const_iterator q = m_gpumemorys.find(value);')
                rc_body.append('        return (q == m_gpumemorys.end()) ? VK_NULL_HANDLE : q->second.replayGpuMem;')
                rc_body.append('    }\n')
            else:
                rc_body.append(self._map_decl(obj_map_dict[var], obj_map_dict[var], var))
                rc_body.append(self._add_to_map_decl(obj_map_dict[var], obj_map_dict[var], var))
                rc_body.append(self._rm_from_map_decl(obj_map_dict[var], var))
                rc_body.append(self._remap_decl(obj_map_dict[var], var))
        # VkDynamicStateObject code
        state_obj_remap_types = vulkan.object_dynamic_state_list
        rc_body.append('    VkDynamicStateObject remap(const VkDynamicStateObject& state)\n    {')
        rc_body.append('        VkDynamicStateObject obj;')
        for t in state_obj_remap_types:
            rc_body.append('        if ((obj = remap(static_cast <%s> (state))) != VK_NULL_HANDLE)' % t)
            rc_body.append('            return obj;')
        rc_body.append('        return VK_NULL_HANDLE;\n    }')
        rc_body.append('    void rm_from_map(const VkDynamicStateObject& state)\n    {')
        for t in state_obj_remap_types:
            rc_body.append('        rm_from_map(static_cast <%s> (state));' % t)
        rc_body.append('    }')
        rc_body.append('')
        # OBJECT code
        rc_body.append('    VkObject remap(const VkObject& object)\n    {')
        rc_body.append('        VkObject obj;')
        obj_remap_types = vulkan.object_list
        for var in obj_remap_types:
            rc_body.append('        if ((obj = remap(static_cast <%s> (object))) != VK_NULL_HANDLE)' % (var))
            rc_body.append('            return obj;')
        rc_body.append('        return VK_NULL_HANDLE;\n    }')
        rc_body.append('    void rm_from_map(const VkObject & objKey)\n    {')
        for var in obj_remap_types:
            rc_body.append('        rm_from_map(static_cast <%s> (objKey));' % (var))
        rc_body.append('    }')
        rc_body.append('    VkBaseObject remap(const VkBaseObject& object)\n    {')
        rc_body.append('        VkBaseObject obj;')
        base_obj_remap_types = ['VkDevice', 'VkQueue', 'VkGpuMemory', 'VkObject']
        for t in base_obj_remap_types:
            rc_body.append('        if ((obj = remap(static_cast <%s> (object))) != VK_NULL_HANDLE)' % t)
            rc_body.append('            return obj;')
        rc_body.append('        return VK_NULL_HANDLE;')
        rc_body.append('    }')
        rc_body.append('};')
        return "\n".join(rc_body)

    def _generate_replay_init_funcs(self):
        rif_body = []
        rif_body.append('void vkFuncs::init_funcs(void * handle)\n{\n    m_libHandle = handle;')
        for proto in self.protos:
            rif_body.append('    real_vk%s = (type_vk%s)(glv_platform_get_library_entrypoint(handle, "vk%s"));' % (proto.name, proto.name, proto.name))
        rif_body.append('}')
        return "\n".join(rif_body)

    def _get_packet_param(self, t, n):
        # list of types that require remapping
        remap_list = vulkan.object_type_list
        param_exclude_list = ['p1', 'p2', 'pGpus', 'pDescriptorSets']
        if t.strip('*').replace('const ', '') in remap_list and n not in param_exclude_list:
            if '*' in t:
                if 'const ' not in t:
                    return 'm_objMapper.remap(*pPacket->%s)' % (n)
                else: # TODO : Don't remap array ptrs?
                    return 'pPacket->%s' % (n)
            return 'm_objMapper.remap(pPacket->%s)' % (n)
        return 'pPacket->%s' % (n)

    def _gen_replay_enum_gpus(self):
        ieg_body = []
        ieg_body.append('            returnValue = manually_handle_vkEnumerateGpus(pPacket);')
        return "\n".join(ieg_body)

    def _gen_replay_get_gpu_info(self):
        ggi_body = []
        ggi_body.append('            returnValue = manually_handle_vkGetGpuInfo(pPacket);')
        return "\n".join(ggi_body)

    def _gen_replay_create_device(self):
        cd_body = []
        cd_body.append('            returnValue = manually_handle_vkCreateDevice(pPacket);')
        return "\n".join(cd_body)

    def _gen_replay_get_extension_support(self):
        ges_body = []
        ges_body.append('            returnValue = manually_handle_vkGetExtensionSupport(pPacket);')
        return "\n".join(ges_body)

    def _gen_replay_queue_submit(self):
        qs_body = []
        qs_body.append('            returnValue = manually_handle_vkQueueSubmit(pPacket);')
        return "\n".join(qs_body)

    def _gen_replay_get_object_info(self):
        goi_body = []
        goi_body.append('            returnValue = manually_handle_vkGetObjectInfo(pPacket);')
        return "\n".join(goi_body)

    def _gen_replay_get_format_info(self):
        gfi_body = []
        gfi_body.append('            returnValue = manually_handle_vkGetFormatInfo(pPacket);')
        return "\n".join(gfi_body)

    def _gen_replay_create_image(self):
        ci_body = []
        ci_body.append('            imageObj local_imageObj;')
        ci_body.append('            replayResult = m_vkFuncs.real_vkCreateImage(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_imageObj.replayImage);')
        ci_body.append('            if (replayResult == VK_SUCCESS)')
        ci_body.append('            {')
        ci_body.append('                m_objMapper.add_to_map(pPacket->pImage, &local_imageObj);')
        ci_body.append('            }')
        return "\n".join(ci_body)

    def _gen_replay_create_buffer(self):
        cb_body = []
        cb_body.append('            bufferObj local_bufferObj;')
        cb_body.append('            replayResult = m_vkFuncs.real_vkCreateBuffer(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_bufferObj.replayBuffer);')
        cb_body.append('            if (replayResult == VK_SUCCESS)')
        cb_body.append('            {')
        cb_body.append('                m_objMapper.add_to_map(pPacket->pBuffer, &local_bufferObj);')
        cb_body.append('            }')
        return "\n".join(cb_body)

    def _gen_replay_get_image_subresource_info(self):
        isi_body = []
        isi_body.append('            returnValue = manually_handle_vkGetImageSubresourceInfo(pPacket);')
        return "\n".join(isi_body)

    def _gen_replay_update_descriptors(self):
        ud_body = []
        ud_body.append('            returnValue = manually_handle_vkUpdateDescriptors(pPacket);')
        return "\n".join(ud_body)

    def _gen_replay_create_descriptor_set_layout(self):
        cdsl_body = []
        cdsl_body.append('            returnValue = manually_handle_vkCreateDescriptorSetLayout(pPacket);')
        return "\n".join(cdsl_body)

    def _gen_replay_create_graphics_pipeline(self):
        cgp_body = []
        cgp_body.append('            returnValue = manually_handle_vkCreateGraphicsPipeline(pPacket);')
        return "\n".join(cgp_body)

    def _gen_replay_cmd_wait_events(self):
        cwe_body = []
        cwe_body.append('            returnValue = manually_handle_vkCmdWaitEvents(pPacket);')
        return "\n".join(cwe_body)

    def _gen_replay_cmd_pipeline_barrier(self):
        cpb_body = []
        cpb_body.append('            returnValue = manually_handle_vkCmdPipelineBarrier(pPacket);')
        return "\n".join(cpb_body)

    def _gen_replay_create_framebuffer(self):
        cf_body = []
        cf_body.append('            returnValue = manually_handle_vkCreateFramebuffer(pPacket);')
        return "\n".join(cf_body)

    def _gen_replay_create_renderpass(self):
        cr_body = []
        cr_body.append('            returnValue = manually_handle_vkCreateRenderPass(pPacket);')
        return "\n".join(cr_body)

    def _gen_replay_begin_command_buffer(self):
        bcb_body = []
        bcb_body.append('            returnValue = manually_handle_vkBeginCommandBuffer(pPacket);')
        return "\n".join(bcb_body)

    def _gen_replay_store_pipeline(self):
        sp_body = []
        sp_body.append('            returnValue = manually_handle_vkStorePipeline(pPacket);')
        return "\n".join(sp_body)

    def _gen_replay_get_multi_gpu_compatibility(self):
        gmgc_body = []
        gmgc_body.append('            returnValue = manually_handle_vkGetMultiGpuCompatibility(pPacket);')
        return "\n".join(gmgc_body)

    def _gen_replay_destroy_object(self):
        do_body = []
        do_body.append('            returnValue = manually_handle_vkDestroyObject(pPacket);')
        return "\n".join(do_body)

    def _gen_replay_wait_for_fences(self):
        wf_body = []
        wf_body.append('            returnValue = manually_handle_vkWaitForFences(pPacket);')
        return "\n".join(wf_body)

    def _gen_replay_wsi_associate_connection(self):
        wac_body = []
        wac_body.append('            returnValue = manually_handle_vkWsiX11AssociateConnection(pPacket);')
        return "\n".join(wac_body)

    def _gen_replay_wsi_get_msc(self):
        wgm_body = []
        wgm_body.append('            returnValue = manually_handle_vkWsiX11GetMSC(pPacket);')
        return "\n".join(wgm_body)

    def _gen_replay_wsi_create_presentable_image(self):
        cpi_body = []
        cpi_body.append('            returnValue = manually_handle_vkWsiX11CreatePresentableImage(pPacket);')
        return "\n".join(cpi_body)

    def _gen_replay_wsi_queue_present(self):
        wqp_body = []
        wqp_body.append('            returnValue = manually_handle_vkWsiX11QueuePresent(pPacket);')
        return "\n".join(wqp_body)

    def _gen_replay_alloc_memory(self):
        am_body = []
        am_body.append('            gpuMemObj local_mem;')
        am_body.append('            if (!m_objMapper.m_adjustForGPU)')
        am_body.append('                replayResult = m_vkFuncs.real_vkAllocMemory(m_objMapper.remap(pPacket->device), pPacket->pAllocInfo, &local_mem.replayGpuMem);')
        am_body.append('            if (replayResult == VK_SUCCESS || m_objMapper.m_adjustForGPU)')
        am_body.append('            {')
        am_body.append('                local_mem.pGpuMem = new (gpuMemory);')
        am_body.append('                if (local_mem.pGpuMem)')
        am_body.append('                    local_mem.pGpuMem->setAllocInfo(pPacket->pAllocInfo, m_objMapper.m_adjustForGPU);')
        am_body.append('                m_objMapper.add_to_map(pPacket->pMem, &local_mem);')
        am_body.append('            }')
        return "\n".join(am_body)

    def _gen_replay_free_memory(self):
        fm_body = []
        fm_body.append('            returnValue = manually_handle_vkFreeMemory(pPacket);')
        return "\n".join(fm_body)

    def _gen_replay_map_memory(self):
        mm_body = []
        mm_body.append('            returnValue = manually_handle_vkMapMemory(pPacket);')
        return "\n".join(mm_body)

    def _gen_replay_unmap_memory(self):
        um_body = []
        um_body.append('            returnValue = manually_handle_vkUnmapMemory(pPacket);')
        return "\n".join(um_body)

    def _gen_replay_pin_system_memory(self):
        psm_body = []
        psm_body.append('            gpuMemObj local_mem;')
        psm_body.append('            /* TODO do we need to skip (make pending) this call for m_adjustForGPU */')
        psm_body.append('            replayResult = m_vkFuncs.real_vkPinSystemMemory(m_objMapper.remap(pPacket->device), pPacket->pSysMem, pPacket->memSize, &local_mem.replayGpuMem);')
        psm_body.append('            if (replayResult == VK_SUCCESS)')
        psm_body.append('                m_objMapper.add_to_map(pPacket->pMem, &local_mem);')
        return "\n".join(psm_body)

    # I don't think this function is being generated anymore (ie, it may have been removed from VK)
    def _gen_replay_bind_dynamic_memory_view(self):
        bdmv_body = []
        bdmv_body.append('            VK_MEMORY_VIEW_ATTACH_INFO memView;')
        bdmv_body.append('            memcpy(&memView, pPacket->pMemView, sizeof(VK_MEMORY_VIEW_ATTACH_INFO));')
        bdmv_body.append('            memView.mem = m_objMapper.remap(pPacket->pMemView->mem);')
        bdmv_body.append('            m_vkFuncs.real_vkCmdBindDynamicMemoryView(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, &memView);')
        return "\n".join(bdmv_body)

    # Generate main replay case statements where actual replay API call is dispatched based on input packet data
    def _generate_replay(self):
        # map protos to custom functions if body is fully custom
        custom_body_dict = {'EnumerateGpus': self._gen_replay_enum_gpus,
                            'GetGpuInfo': self._gen_replay_get_gpu_info,
                            'CreateDevice': self._gen_replay_create_device,
                            'GetExtensionSupport': self._gen_replay_get_extension_support,
                            'QueueSubmit': self._gen_replay_queue_submit,
                            'GetObjectInfo': self._gen_replay_get_object_info,
                            'GetFormatInfo': self._gen_replay_get_format_info,
                            'CreateImage': self._gen_replay_create_image,
                            'CreateBuffer': self._gen_replay_create_buffer,
                            'GetImageSubresourceInfo': self._gen_replay_get_image_subresource_info,
                            'CreateGraphicsPipeline': self._gen_replay_create_graphics_pipeline,
                            'CreateFramebuffer': self._gen_replay_create_framebuffer,
                            'CreateRenderPass': self._gen_replay_create_renderpass,
                            'BeginCommandBuffer': self._gen_replay_begin_command_buffer,
                            'StorePipeline': self._gen_replay_store_pipeline,
                            'GetMultiGpuCompatibility': self._gen_replay_get_multi_gpu_compatibility,
                            'DestroyObject': self._gen_replay_destroy_object,
                            'WaitForFences': self._gen_replay_wait_for_fences,
                            'WsiX11AssociateConnection': self._gen_replay_wsi_associate_connection,
                            'WsiX11GetMSC': self._gen_replay_wsi_get_msc,
                            'WsiX11CreatePresentableImage': self._gen_replay_wsi_create_presentable_image,
                            'WsiX11QueuePresent': self._gen_replay_wsi_queue_present,
                            'AllocMemory': self._gen_replay_alloc_memory,
                            'FreeMemory': self._gen_replay_free_memory,
                            'MapMemory': self._gen_replay_map_memory,
                            'UnmapMemory': self._gen_replay_unmap_memory,
                            'PinSystemMemory': self._gen_replay_pin_system_memory,
                            'CmdBindDynamicMemoryView': self._gen_replay_bind_dynamic_memory_view,
                            'UpdateDescriptors': self._gen_replay_update_descriptors,
                            'CreateDescriptorSetLayout': self._gen_replay_create_descriptor_set_layout,
                            'CmdWaitEvents': self._gen_replay_cmd_wait_events,
                            'CmdPipelineBarrier': self._gen_replay_cmd_pipeline_barrier}
        # TODO : Need to guard CreateInstance with "if (!m_display->m_initedVK)" check
        # Despite returning a value, don't check these funcs b/c custom code includes check already
        custom_check_ret_val = ['EnumerateGpus', 'GetGpuInfo', 'CreateDevice', 'GetExtensionSupport', 'QueueSubmit', 'GetObjectInfo',
                                'GetFormatInfo', 'GetImageSubresourceInfo', 'CreateDescriptorSetLayout', 'CreateGraphicsPipeline',
                                'CreateFramebuffer', 'CreateRenderPass', 'BeginCommandBuffer', 'StorePipeline', 'GetMultiGpuCompatibility',
                                'DestroyObject', 'WaitForFences', 'FreeMemory', 'MapMemory', 'UnmapMemory',
                                'WsiX11AssociateConnection', 'WsiX11GetMSC', 'WsiX11CreatePresentableImage', 'WsiX11QueuePresent']
        # multi-gpu Open funcs w/ list of local params to create
        custom_open_params = {'OpenSharedMemory': (-1,),
                              'OpenSharedSemaphore': (-1,),
                              'OpenPeerMemory': (-1,),
                              'OpenPeerImage': (-1, -2,)}
        # Functions that create views are unique from other create functions
        create_view_list = ['CreateBufferView', 'CreateImageView', 'CreateColorAttachmentView', 'CreateDepthStencilView', 'CreateComputePipeline']
        # Functions to treat as "Create' that don't have 'Create' in the name
        special_create_list = ['LoadPipeline', 'LoadPipelineDerivative', 'AllocMemory', 'GetDeviceQueue', 'PinSystemMemory', 'AllocDescriptorSets']
        # A couple funcs use do while loops
        do_while_dict = {'GetFenceStatus': 'replayResult != pPacket->result  && pPacket->result == VK_SUCCESS', 'GetEventStatus': '(pPacket->result == VK_EVENT_SET || pPacket->result == VK_EVENT_RESET) && replayResult != pPacket->result'}
        rbody = []
        rbody.append('glv_replay::GLV_REPLAY_RESULT vkReplay::replay(glv_trace_packet_header *packet)')
        rbody.append('{')
        rbody.append('    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;')
        rbody.append('    VkResult replayResult = VK_ERROR_UNKNOWN;')
        rbody.append('    switch (packet->packet_id)')
        rbody.append('    {')
        rbody.append('        case GLV_TPI_VK_vkApiVersion:')
        rbody.append('            break;  // nothing to replay on the version packet')
        for proto in self.protos:
            ret_value = False
            create_view = False
            create_func = False
            # TODO : How to handle void* return of GetProcAddr?
            if ('void' not in proto.ret) and (proto.name not in custom_check_ret_val):
                ret_value = True
            if proto.name in create_view_list:
                create_view = True
            elif 'Create' in proto.name or proto.name in special_create_list:
                create_func = True
            rbody.append('        case GLV_TPI_VK_vk%s:' % proto.name)
            rbody.append('        {')
            rbody.append('            struct_vk%s* pPacket = (struct_vk%s*)(packet->pBody);' % (proto.name, proto.name))
            if proto.name in custom_body_dict:
                rbody.append(custom_body_dict[proto.name]())
            else:
                if proto.name in custom_open_params:
                    rbody.append('            VkDevice handle;')
                    for pidx in custom_open_params[proto.name]:
                        rbody.append('            %s local_%s;' % (proto.params[pidx].ty.replace('const ', '').strip('*'), proto.params[pidx].name))
                    rbody.append('            handle = m_objMapper.remap(pPacket->device);')
                elif create_view:
                    rbody.append('            %s createInfo;' % (proto.params[1].ty.strip('*').replace('const ', '')))
                    rbody.append('            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(%s));' % (proto.params[1].ty.strip('*').replace('const ', '')))
                    if 'CreateComputePipeline' == proto.name:
                        rbody.append('            createInfo.cs.shader = m_objMapper.remap(pPacket->pCreateInfo->cs.shader);')
                    elif 'CreateBufferView' == proto.name:
                        rbody.append('            createInfo.buffer = m_objMapper.remap(pPacket->pCreateInfo->buffer);')
                    else:
                        rbody.append('            createInfo.image = m_objMapper.remap(pPacket->pCreateInfo->image);')
                    rbody.append('            %s local_%s;' % (proto.params[-1].ty.strip('*').replace('const ', ''), proto.params[-1].name))
                elif create_func: # Declare local var to store created handle into
                    rbody.append('            %s local_%s;' % (proto.params[-1].ty.strip('*').replace('const ', ''), proto.params[-1].name))
                    if 'AllocDescriptorSets' == proto.name:
                        rbody.append('            %s local_%s[100];' % (proto.params[-2].ty.strip('*').replace('const ', ''), proto.params[-2].name))
                        rbody.append('            VkDescriptorSetLayout localDescSets[100];')
                        rbody.append('            assert(pPacket->count <= 100);')
                        rbody.append('            for (uint32_t i = 0; i < pPacket->count; i++)')
                        rbody.append('            {')
                        rbody.append('                localDescSets[i] = m_objMapper.remap(pPacket->%s[i]);' % (proto.params[-3].name))
                        rbody.append('            }')
                elif proto.name == 'ClearDescriptorSets':
                    rbody.append('            VkDescriptorSet localDescSets[100];')
                    rbody.append('            assert(pPacket->count <= 100);')
                    rbody.append('            for (uint32_t i = 0; i < pPacket->count; i++)')
                    rbody.append('            {')
                    rbody.append('                localDescSets[i] = m_objMapper.remap(pPacket->%s[i]);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name == 'ResetFences':
                    rbody.append('            VkFence* fences = GLV_NEW_ARRAY(VkFence, pPacket->fenceCount);')
                    rbody.append('            for (uint32_t i = 0; i < pPacket->fenceCount; i++)')
                    rbody.append('            {')
                    rbody.append('                fences[i] = m_objMapper.remap(pPacket->%s[i]);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name in do_while_dict:
                    rbody.append('            do {')
                elif proto.name == 'EnumerateLayers':
                    rbody.append('            char **bufptr = GLV_NEW_ARRAY(char *, pPacket->maxLayerCount);')
                    rbody.append('            char **ptrLayers = (pPacket->pOutLayers == NULL) ? bufptr : (char **) pPacket->pOutLayers;')
                    rbody.append('            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)')
                    rbody.append('                bufptr[i] = GLV_NEW_ARRAY(char, pPacket->maxStringSize);')
                elif proto.name == 'DestroyInstance':
                    rbody.append('            vkDbgUnregisterMsgCallback(m_objMapper.remap(pPacket->instance), g_fpDbgMsgCallback);')
                rr_string = '            '
                if ret_value:
                    rr_string = '            replayResult = '
                rr_string += 'm_vkFuncs.real_vk%s(' % proto.name
                for p in proto.params:
                    # For last param of Create funcs, pass address of param
                    if create_func:
                        if p.name == proto.params[-1].name:
                            rr_string += '&local_%s, ' % p.name
                        elif proto.name == 'AllocDescriptorSets' and p.name == proto.params[-2].name:
                            rr_string += 'local_%s, ' % p.name
                        else:
                            rr_string += '%s, ' % self._get_packet_param(p.ty, p.name)
                    else:
                        rr_string += '%s, ' % self._get_packet_param(p.ty, p.name)
                rr_string = '%s);' % rr_string[:-2]
                if proto.name in custom_open_params:
                    rr_list = rr_string.split(', ')
                    rr_list[0] = rr_list[0].replace('m_objMapper.remap(pPacket->device)', 'handle')
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
                elif proto.name == 'EnumerateLayers':
                    rr_string = rr_string.replace('pPacket->pOutLayers', 'ptrLayers')
                elif proto.name == 'ClearDescriptorSets':
                    rr_string = rr_string.replace('pPacket->pDescriptorSets', 'localDescSets')
                elif proto.name == 'AllocDescriptorSets':
                    rr_string = rr_string.replace('pPacket->pSetLayouts', 'localDescSets')
                elif proto.name == 'ResetFences':
                    rr_string = rr_string.replace('m_objMapper.remap(*pPacket->pFences)', 'fences')

                # insert the real_*(..) call
                rbody.append(rr_string)

                # handle return values or anything that needs to happen after the real_*(..) call
                if 'DestroyDevice' in proto.name:
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                m_pCBDump = NULL;')
                    rbody.append('                m_pDSDump = NULL;')
                    rbody.append('                m_pGlvSnapshotPrint = NULL;')
                    rbody.append('                m_objMapper.rm_from_map(pPacket->device);')
                    rbody.append('                m_display->m_initedVK = false;')
                    rbody.append('            }')
                if 'DestroyInstance' in proto.name:
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                // TODO need to handle multiple instances and only clearing maps within an instance.')
                    rbody.append('                // TODO this only works with a single instance used at any given time.')
                    rbody.append('                m_objMapper.clear_all_map_handles();')
                    rbody.append('            }')
                elif 'AllocDescriptorSets' in proto.name:
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                for (uint32_t i = 0; i < local_pCount; i++) {')
                    rbody.append('                    m_objMapper.add_to_map(&pPacket->%s[i], &local_%s[i]);' % (proto.params[-2].name, proto.params[-2].name))
                    rbody.append('                }')
                    rbody.append('            }')
                elif proto.name == 'ResetFences':
                    rbody.append('            GLV_DELETE(fences);')
                elif create_func: # save handle mapping if create successful
                    rbody.append('            if (replayResult == VK_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                m_objMapper.add_to_map(pPacket->%s, &local_%s);' % (proto.params[-1].name, proto.params[-1].name))
                    if 'AllocMemory' == proto.name:
                        rbody.append('                m_objMapper.add_entry_to_mapData(local_%s, pPacket->pAllocInfo->allocationSize);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name in do_while_dict:
                    rbody[-1] = '    %s' % rbody[-1]
                    rbody.append('            } while (%s);' % do_while_dict[proto.name])
                    rbody.append('            if (pPacket->result != VK_NOT_READY || replayResult != VK_SUCCESS)')
                elif proto.name == 'EnumerateLayers':
                    rbody.append('            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)')
                    rbody.append('                GLV_DELETE(bufptr[i]);')
            if ret_value:
                rbody.append('            CHECK_RETURN_VALUE(vk%s);' % proto.name)
            if 'MsgCallback' in proto.name:
                rbody.pop()
                rbody.pop()
                rbody.pop()
                rbody.append('            // Just eating these calls as no way to restore dbg func ptr.')
            rbody.append('            break;')
            rbody.append('        }')
        rbody.append('        default:')
        rbody.append('            glv_LogWarn("Unrecognized packet_id %u, skipping\\n", packet->packet_id);')
        rbody.append('            returnValue = glv_replay::GLV_REPLAY_INVALID_ID;')
        rbody.append('            break;')
        rbody.append('    }')
        rbody.append('    return returnValue;')
        rbody.append('}')
        return "\n".join(rbody)

class GlaveTraceHeader(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glv_vk_vk_structs.h"')
        header_txt.append('#include "glv_vk_packet_id.h"\n')
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
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glv_platform.h"')
        header_txt.append('#include "glv_common.h"')
        header_txt.append('#include "glvtrace_vk_helpers.h"')
        header_txt.append('#include "glvtrace_vk_vk.h"')
        header_txt.append('#include "glvtrace_vk_vkdbg.h"')
        header_txt.append('#include "glvtrace_vk_vkwsix11ext.h"')
        header_txt.append('#include "glv_interconnect.h"')
        header_txt.append('#include "glv_filelike.h"')
        header_txt.append('#include "vk_struct_size_helper.h"')
        header_txt.append('#ifdef WIN32')
        header_txt.append('#include "mhook/mhook-lib/mhook.h"')
        header_txt.append('#endif')
        header_txt.append('#include "glv_trace_packet_utils.h"')
        header_txt.append('#include <stdio.h>\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_func_ptr_assignments(),
                self._generate_attach_hooks(),
                self._generate_detach_hooks(),
                self._generate_init_funcs(),
                self._generate_trace_funcs()]

        return "\n".join(body)

class GlavePacketID(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "glv_trace_packet_utils.h"')
        header_txt.append('#include "glv_trace_packet_identifiers.h"')
        header_txt.append('#include "glv_interconnect.h"')
        header_txt.append('#include "glv_vk_vk_structs.h"')
        header_txt.append('#include "glv_vk_vkdbg_structs.h"')
        header_txt.append('#include "glv_vk_vkwsix11ext_structs.h"')
        header_txt.append('#include "vk_enum_string_helper.h"')
        header_txt.append('#if defined(WIN32)')
        header_txt.append('#define snprintf _snprintf')
        header_txt.append('#endif')
        header_txt.append('#define SEND_ENTRYPOINT_ID(entrypoint) ;')
        header_txt.append('//#define SEND_ENTRYPOINT_ID(entrypoint) glv_TraceInfo(#entrypoint "\\n");\n')
        header_txt.append('#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) ;')
        header_txt.append('//#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) glv_TraceInfo(entrypoint, __VA_ARGS__);\n')
        header_txt.append('#define CREATE_TRACE_PACKET(entrypoint, buffer_bytes_needed) \\')
        header_txt.append('    pHeader = glv_create_trace_packet(GLV_TID_VULKAN, GLV_TPI_VK_##entrypoint, sizeof(struct_##entrypoint), buffer_bytes_needed);\n')
        header_txt.append('#define FINISH_TRACE_PACKET() \\')
        header_txt.append('    glv_finalize_trace_packet(pHeader); \\')
        header_txt.append('    glv_write_trace_packet(pHeader, glv_trace_get_trace_file()); \\')
        header_txt.append('    glv_delete_trace_packet(&pHeader);')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_packet_id_enum(),
                self._generate_stringify_func(),
                self._generate_interp_func()]

        return "\n".join(body)

class GlaveCoreStructs(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "glv_trace_packet_utils.h"\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_struct_util_funcs(),
                self._generate_interp_funcs()]

        return "\n".join(body)

class GlaveWsiHeader(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include "vkWsiX11Ext.h"\n')
        header_txt.append('#else')
        header_txt.append('#include "vkWsiWinExt.h"')
        header_txt.append('#endif')
        header_txt.append('void AttachHooks_vkwsix11ext();')
        header_txt.append('void DetachHooks_vkwsix11ext();')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_trace_func_ptrs_ext(),
                self._generate_trace_func_protos_ext()]

        return "\n".join(body)

class GlaveWsiC(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glv_platform.h"')
        header_txt.append('#include "glv_common.h"')
        header_txt.append('#include "glvtrace_vk_vkwsix11ext.h"')
        header_txt.append('#include "glv_vk_vkwsix11ext_structs.h"')
        header_txt.append('#include "glv_vk_packet_id.h"')
        header_txt.append('#ifdef WIN32')
        header_txt.append('#include "mhook/mhook-lib/mhook.h"')
        header_txt.append('#endif')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_func_ptr_assignments_ext(),
                self._generate_attach_hooks_ext(),
                self._generate_detach_hooks_ext(),
                self._generate_trace_funcs_ext()]

        return "\n".join(body)

class GlaveWsiStructs(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include "vkWsiX11Ext.h"')
        header_txt.append('#else')
        header_txt.append('#include "vkWsiWinExt.h"')
        header_txt.append('#endif')
        header_txt.append('#include "glv_trace_packet_utils.h"\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_interp_funcs_ext()]

        return "\n".join(body)

class GlaveDbgHeader(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "vkDbg.h"\n')
        header_txt.append('void AttachHooks_vkdbg();')
        header_txt.append('void DetachHooks_vkdbg();')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_trace_func_ptrs_ext('Dbg'),
                self._generate_trace_func_protos_ext('Dbg')]

        return "\n".join(body)

class GlaveDbgC(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glv_platform.h"')
        header_txt.append('#include "glv_common.h"')
        header_txt.append('#include "glvtrace_vk_vk.h"')
        header_txt.append('#include "glvtrace_vk_vkdbg.h"')
        header_txt.append('#include "glv_vk_vkdbg_structs.h"')
        header_txt.append('#include "glv_vk_packet_id.h"')
        header_txt.append('#ifdef WIN32')
        header_txt.append('#include "mhook/mhook-lib/mhook.h"')
        header_txt.append('#endif')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_func_ptr_assignments_ext('Dbg'),
                self._generate_attach_hooks_ext('Dbg'),
                self._generate_detach_hooks_ext('Dbg'),
                self._generate_trace_funcs_ext('Dbg')]

        return "\n".join(body)

class GlaveDbgStructs(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "vkDbg.h"')
        header_txt.append('#include "glv_trace_packet_utils.h"\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_interp_funcs_ext('Dbg')]

        return "\n".join(body)

class GlaveReplayVkFuncPtrs(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include <xcb/xcb.h>\n')
        header_txt.append('#endif')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "vkDbg.h"')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include "vkWsiX11Ext.h"')
        header_txt.append('#else')
        header_txt.append('#include "vkWsiWinExt.h"')
        header_txt.append('#endif')

    def generate_body(self):
        body = [self._generate_replay_func_ptrs()]
        return "\n".join(body)

class GlaveReplayObjMapperHeader(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include <set>')
        header_txt.append('#include <map>')
        header_txt.append('#include <vector>')
        header_txt.append('#include <string>')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "vkDbg.h"')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include "vkWsiX11Ext.h"')
        header_txt.append('#else')
        header_txt.append('#include "vkWsiWinExt.h"')
        header_txt.append('#endif')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_objmapper_class()]
        return "\n".join(body)

class GlaveReplayC(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glvreplay_vk_vkreplay.h"\n')
        header_txt.append('#include "glvreplay_vk.h"\n')
        header_txt.append('#include "glvreplay_main.h"\n')
        header_txt.append('#include <algorithm>')
        header_txt.append('#include <queue>')
        header_txt.append('\n')
        header_txt.append('extern "C" {')
        header_txt.append('#include "glv_vk_vk_structs.h"')
        header_txt.append('#include "glv_vk_vkdbg_structs.h"')
        header_txt.append('#include "glv_vk_vkwsix11ext_structs.h"')
        header_txt.append('#include "glv_vk_packet_id.h"')
        header_txt.append('#include "vk_enum_string_helper.h"\n}\n')
        header_txt.append('#define APP_NAME "glvreplay_vk"')
        header_txt.append('#define IDI_ICON 101\n')

        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_init_funcs(),
                self._generate_replay()]

        return "\n".join(body)

def main():
    subcommands = {
            "glave-trace-h" : GlaveTraceHeader,
            "glave-trace-c" : GlaveTraceC,
            "glave-packet-id" : GlavePacketID,
            "glave-core-structs" : GlaveCoreStructs,
            "glave-wsi-trace-h" : GlaveWsiHeader,
            "glave-wsi-trace-c" : GlaveWsiC,
            "glave-wsi-trace-structs" : GlaveWsiStructs,
            "glave-dbg-trace-h" : GlaveDbgHeader,
            "glave-dbg-trace-c" : GlaveDbgC,
            "glave-dbg-trace-structs" : GlaveDbgStructs,
            "glave-replay-vk-funcs" : GlaveReplayVkFuncPtrs,
            "glave-replay-obj-mapper-h" : GlaveReplayObjMapperHeader,
            "glave-replay-c" : GlaveReplayC,
    }

    if len(sys.argv) < 2 or sys.argv[1] not in subcommands:
        print("Usage: %s <subcommand> [options]" % sys.argv[0])
        print
        print("Available sucommands are: %s" % " ".join(subcommands))
        exit(1)

    subcmd = subcommands[sys.argv[1]](sys.argv[2:])
    subcmd.run()

if __name__ == "__main__":
    main()
