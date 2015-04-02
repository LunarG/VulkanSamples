#!/usr/bin/env python3
#
# XGL
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

# add main repo directory so xgl.py can be imported. This needs to be a complete path.
glv_scripts_path = os.path.dirname(os.path.abspath(__file__))
main_path = os.path.abspath(glv_scripts_path + "/../../../")
sys.path.append(main_path)

import xgl

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.headers = xgl.headers
        self.protos = xgl.protos

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
 * XGL
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
    def _get_printf_params(self, xgl_type, name, output_param):
        deref = ""
        # TODO : Need ENUM and STRUCT checks here
        if "_TYPE" in xgl_type: # TODO : This should be generic ENUM check
            return ("%s", "string_%s(%s)" % (xgl_type.replace('const ', '').strip('*'), name), deref)
        if "char*" == xgl_type:
            return ("%s", name, "*")
        if "uint64_t" in xgl_type:
            if '*' in xgl_type:
                return ("%lu",  "(%s == NULL) ? 0 : *(%s)" % (name, name), "*")
            return ("%lu", name, deref)
        if "size_t" in xgl_type:
            if '*' in xgl_type:
                return ("%zu", "(%s == NULL) ? 0 : *(%s)" % (name, name), "*")
            return ("%zu", name, deref)
        if "float" in xgl_type:
            if '[' in xgl_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                return ("[%f, %f, %f, %f]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name), deref)
            return ("%f", name, deref)
        if "bool" in xgl_type or 'xcb_randr_crtc_t' in xgl_type:
            return ("%u", name, deref)
        if True in [t in xgl_type for t in ["int", "FLAGS", "MASK", "xcb_window_t"]]:
            if '[' in xgl_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                return ("[%i, %i, %i, %i]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name), deref)
            if '*' in xgl_type:
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
                func_ptrs.append('#define __HOOKED_xgl%s hooked_xgl%s' % (proto.name, proto.name))

        func_ptrs.append('\n#elif defined(PLATFORM_LINUX)')
        func_ptrs.append('extern pthread_once_t gInitOnce;')
        for proto in self.protos:
            if True not in [skip_str in proto.name for skip_str in ['Dbg', 'Wsi']]:
                func_ptrs.append('#define __HOOKED_xgl%s xgl%s' % (proto.name, proto.name))

        func_ptrs.append('#endif\n')
        return "\n".join(func_ptrs)

    def _generate_trace_func_ptrs_ext(self, func_class='Wsi'):
        func_ptrs = []
        func_ptrs.append('#ifdef WIN32')
        for proto in self.protos:
            if func_class in proto.name:
                func_ptrs.append('#define __HOOKED_xgl%s hooked_xgl%s' % (proto.name, proto.name))

        func_ptrs.append('#elif defined(__linux__)')
        for proto in self.protos:
            if func_class in proto.name:
                func_ptrs.append('#define __HOOKED_xgl%s xgl%s' % (proto.name, proto.name))

        func_ptrs.append('#endif\n')
        return "\n".join(func_ptrs)

    def _generate_trace_func_protos(self):
        func_protos = []
        func_protos.append('// Hooked function prototypes\n')
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                func_protos.append('GLVTRACER_EXPORT %s;' % proto.c_func(prefix="__HOOKED_xgl", attr="XGLAPI"))

        return "\n".join(func_protos)

    def _generate_trace_func_protos_ext(self, func_class='Wsi'):
        func_protos = []
        func_protos.append('// Hooked function prototypes\n')
        for proto in self.protos:
            if func_class in proto.name:
                func_protos.append('GLVTRACER_EXPORT %s;' % proto.c_func(prefix="__HOOKED_xgl", attr="XGLAPI"))

        return "\n".join(func_protos)


    def _generate_func_ptr_assignments(self):
        func_ptr_assign = []
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                func_ptr_assign.append('static %s( XGLAPI * real_xgl%s)(' % (proto.ret, proto.name))
                for p in proto.params:
                    if 'color' == p.name:
                        func_ptr_assign.append('    %s %s[4],' % (p.ty.replace('[4]', ''), p.name))
                    else:
                        func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
                func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ') = xgl%s;\n' % (proto.name))
        func_ptr_assign.append('static BOOL isHooked = FALSE;\n')
        return "\n".join(func_ptr_assign)

    def _generate_func_ptr_assignments_ext(self, func_class='Wsi'):
        func_ptr_assign = []
        for proto in self.protos:
            if func_class in proto.name:
                func_ptr_assign.append('static %s( XGLAPI * real_xgl%s)(' % (proto.ret, proto.name))
                for p in proto.params:
                    func_ptr_assign.append('    %s %s,' % (p.ty, p.name))
                func_ptr_assign[-1] = func_ptr_assign[-1].replace(',', ') = xgl%s;\n' % (proto.name))
        return "\n".join(func_ptr_assign)

    def _generate_attach_hooks(self):
        hooks_txt = []
        hooks_txt.append('void AttachHooks()\n{\n   BOOL hookSuccess = TRUE;\n#if defined(WIN32)')
        hooks_txt.append('    Mhook_BeginMultiOperation(FALSE);')
        # TODO : Verify if CreateInstance is appropriate to key off of here
        hooks_txt.append('    if (real_xglCreateInstance != NULL)')
        hooks_txt.append('    {\n        isHooked = TRUE;')
        hook_operator = '='
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                hooks_txt.append('        hookSuccess %s Mhook_SetHook((PVOID*)&real_xgl%s, hooked_xgl%s);' % (hook_operator, proto.name, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }\n')
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook XGL.");\n    }\n')
        hooks_txt.append('    Mhook_EndMultiOperation();\n')
        hooks_txt.append('#elif defined(__linux__)')
        hooks_txt.append('    if (real_xglCreateInstance == xglCreateInstance)')
        hooks_txt.append('        hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateInstance,"xglCreateInstance");')
        hooks_txt.append('    isHooked = TRUE;')
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name and 'CreateInstance' not in proto.name:
                hooks_txt.append('    hookSuccess %s glv_platform_get_next_lib_sym((PVOID*)&real_xgl%s, "xgl%s");' % (hook_operator, proto.name, proto.name))
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook XGL.");\n    }\n')
        hooks_txt.append('#endif\n}\n')
        return "\n".join(hooks_txt)

    def _generate_attach_hooks_ext(self, func_class='Wsi'):
        func_ext_dict = {'Wsi': '_xglwsix11ext', 'Dbg': '_xgldbg'}
        first_proto_dict = {'Wsi': 'WsiX11AssociateConnection', 'Dbg': 'DbgSetValidationLevel'}
        hooks_txt = []
        hooks_txt.append('void AttachHooks%s()\n{\n    BOOL hookSuccess = TRUE;\n#if defined(WIN32)' % func_ext_dict[func_class])
        hooks_txt.append('    Mhook_BeginMultiOperation(FALSE);')
        hooks_txt.append('    if (real_xgl%s != NULL)' % first_proto_dict[func_class])
        hooks_txt.append('    {')
        hook_operator = '='
        for proto in self.protos:
            if func_class in proto.name:
                hooks_txt.append('        hookSuccess %s Mhook_SetHook((PVOID*)&real_xgl%s, hooked_xgl%s);' % (hook_operator, proto.name, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }\n')
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook XGL ext %s.");\n    }\n' % func_class)
        hooks_txt.append('    Mhook_EndMultiOperation();\n')
        hooks_txt.append('#elif defined(__linux__)')
        hooks_txt.append('    hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_xgl%s, "xgl%s");' % (first_proto_dict[func_class], first_proto_dict[func_class]))
        for proto in self.protos:
            if func_class in proto.name and first_proto_dict[func_class] not in proto.name:
                hooks_txt.append('    hookSuccess %s glv_platform_get_next_lib_sym((PVOID*)&real_xgl%s, "xgl%s");' % (hook_operator, proto.name, proto.name))
        hooks_txt.append('    if (!hookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to hook XGL ext %s.");\n    }\n' % func_class)
        hooks_txt.append('#endif\n}\n')
        return "\n".join(hooks_txt)

    def _generate_detach_hooks(self):
        hooks_txt = []
        hooks_txt.append('void DetachHooks()\n{\n#ifdef __linux__\n    return;\n#elif defined(WIN32)')
        hooks_txt.append('    BOOL unhookSuccess = TRUE;\n    if (real_xglGetGpuInfo != NULL)\n    {')
        hook_operator = '='
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                hooks_txt.append('        unhookSuccess %s Mhook_Unhook((PVOID*)&real_xgl%s);' % (hook_operator, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }\n    isHooked = FALSE;')
        hooks_txt.append('    if (!unhookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to unhook XGL.");\n    }')
        hooks_txt.append('#endif\n}')
        hooks_txt.append('#ifdef WIN32\nINIT_ONCE gInitOnce = INIT_ONCE_STATIC_INIT;\n#elif defined(PLATFORM_LINUX)\npthread_once_t gInitOnce = PTHREAD_ONCE_INIT;\n#endif\n')
        return "\n".join(hooks_txt)

    def _generate_detach_hooks_ext(self, func_class='Wsi'):
        func_ext_dict = {'Wsi': '_xglwsix11ext', 'Dbg': '_xgldbg'}
        first_proto_dict = {'Wsi': 'WsiX11AssociateConnection', 'Dbg': 'DbgSetValidationLevel'}
        hooks_txt = []
        hooks_txt.append('void DetachHooks%s()\n{\n#ifdef WIN32' % func_ext_dict[func_class])
        hooks_txt.append('    BOOL unhookSuccess = TRUE;\n    if (real_xgl%s != NULL)\n    {' % first_proto_dict[func_class])
        hook_operator = '='
        for proto in self.protos:
            if func_class in proto.name:
                hooks_txt.append('        unhookSuccess %s Mhook_Unhook((PVOID*)&real_xgl%s);' % (hook_operator, proto.name))
                hook_operator = '&='
        hooks_txt.append('    }')
        hooks_txt.append('    if (!unhookSuccess)\n    {')
        hooks_txt.append('        glv_LogError("Failed to unhook XGL ext %s.");\n    }' % func_class)
        hooks_txt.append('#elif defined(__linux__)\n    return;\n#endif\n}\n')
        return "\n".join(hooks_txt)

    def _generate_init_funcs(self):
        init_tracer = []
        init_tracer.append('void send_xgl_api_version_packet()\n{')
        init_tracer.append('    struct_xglApiVersion* pPacket;')
        init_tracer.append('    glv_trace_packet_header* pHeader;')
        init_tracer.append('    pHeader = glv_create_trace_packet(GLV_TID_XGL, GLV_TPI_XGL_xglApiVersion, sizeof(struct_xglApiVersion), 0);')
        init_tracer.append('    pPacket = interpret_body_as_xglApiVersion(pHeader, FALSE);')
        init_tracer.append('    pPacket->version = XGL_API_VERSION;')
        init_tracer.append('    FINISH_TRACE_PACKET();\n}\n')

        init_tracer.append('static GLV_CRITICAL_SECTION g_memInfoLock;')
        init_tracer.append('void InitTracer(void)\n{')
        init_tracer.append('    char *ipAddr = glv_get_global_var("GLVLIB_TRACE_IPADDR");')
        init_tracer.append('    if (ipAddr == NULL)')
        init_tracer.append('        ipAddr = "127.0.0.1";')
        init_tracer.append('    gMessageStream = glv_MessageStream_create(FALSE, ipAddr, GLV_BASE_PORT + GLV_TID_XGL);')
        init_tracer.append('    glv_trace_set_trace_file(glv_FileLike_create_msg(gMessageStream));')
        init_tracer.append('//    glv_tracelog_set_log_file(glv_FileLike_create_file(fopen("glv_log_traceside.txt","w")));')
        init_tracer.append('    glv_tracelog_set_tracer_id(GLV_TID_XGL);')
        init_tracer.append('    glv_create_critical_section(&g_memInfoLock);')
        init_tracer.append('    send_xgl_api_version_packet();\n}\n')
        return "\n".join(init_tracer)

    def _gen_unmap_memory(self):
        um_body = []
        um_body.append('GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglUnmapMemory(')
        um_body.append('    XGL_GPU_MEMORY mem)')
        um_body.append('{')
        um_body.append('    glv_trace_packet_header* pHeader;')
        um_body.append('    XGL_RESULT result;')
        um_body.append('    struct_xglUnmapMemory* pPacket;')
        um_body.append('    XGLAllocInfo *entry;')
        um_body.append('    SEND_ENTRYPOINT_PARAMS("xglUnmapMemory(mem %p)\\n", mem);')
        um_body.append('    // insert into packet the data that was written by CPU between the xglMapMemory call and here')
        um_body.append('    // Note must do this prior to the real xglUnMap() or else may get a FAULT')
        um_body.append('    glv_enter_critical_section(&g_memInfoLock);')
        um_body.append('    entry = find_mem_info_entry(mem);')
        um_body.append('    CREATE_TRACE_PACKET(xglUnmapMemory, (entry) ? entry->size : 0);')
        um_body.append('    pPacket = interpret_body_as_xglUnmapMemory(pHeader);')
        um_body.append('    if (entry)')
        um_body.append('    {')
        um_body.append('        assert(entry->handle == mem);')
        um_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), entry->size, entry->pData);')
        um_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));')
        um_body.append('        entry->pData = NULL;')
        um_body.append('    } else')
        um_body.append('    {')
        um_body.append('         glv_LogError("Failed to copy app memory into trace packet (idx = %u) on xglUnmapMemory\\n", pHeader->global_packet_index);')
        um_body.append('    }')
        um_body.append('    glv_leave_critical_section(&g_memInfoLock);')
        um_body.append('    result = real_xglUnmapMemory(mem);')
        um_body.append('    pPacket->mem = mem;')
        um_body.append('    pPacket->result = result;')
        um_body.append('    FINISH_TRACE_PACKET();')
        um_body.append('    return result;')
        um_body.append('}\n')
        return "\n".join(um_body)

    # Take a list of params and return a list of dicts w/ ptr param details
    def _get_packet_ptr_param_list(self, params):
        ptr_param_list = []
        # TODO : This is a slightly nicer way to handle custom cases than initial code, however
        #   this can still be further generalized to eliminate more custom code
        #   big case to handle is when ptrs to structs have embedded data that needs to be accounted for in packet
        custom_ptr_dict = {'XGL_DEVICE_CREATE_INFO': {'add_txt': 'add_XGL_DEVICE_CREATE_INFO_to_packet(pHeader, (XGL_DEVICE_CREATE_INFO**) &(pPacket->pCreateInfo), pCreateInfo)',
                                                  'finalize_txt': ''},
                           'XGL_APPLICATION_INFO': {'add_txt': 'add_XGL_APPLICATION_INFO_to_packet(pHeader, (XGL_APPLICATION_INFO**)&(pPacket->pAppInfo), pAppInfo)',
                                                'finalize_txt': ''},
                           'XGL_PHYSICAL_GPU': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpus), *pGpuCount*sizeof(XGL_PHYSICAL_GPU), pGpus)',
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
                           'XGL_SHADER_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_SHADER_CREATE_INFO), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), ((pCreateInfo != NULL) ? pCreateInfo->codeSize : 0), pCreateInfo->pCode)',
                                                      'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'XGL_FRAMEBUFFER_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_FRAMEBUFFER_CREATE_INFO), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments), colorCount * sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO), pCreateInfo->pColorAttachments);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment), dsSize, pCreateInfo->pDepthStencilAttachment)',
                                                           'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'XGL_RENDER_PASS_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_RENDER_PASS_CREATE_INFO), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps), colorCount * sizeof(XGL_ATTACHMENT_LOAD_OP), pCreateInfo->pColorLoadOps);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps), colorCount * sizeof(XGL_ATTACHMENT_STORE_OP), pCreateInfo->pColorStoreOps);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues), colorCount * sizeof(XGL_CLEAR_COLOR), pCreateInfo->pColorLoadClearValues)',
                                                          'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'XGL_CMD_BUFFER_BEGIN_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo), sizeof(XGL_CMD_BUFFER_BEGIN_INFO), pBeginInfo);\n    add_begin_cmdbuf_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo->pNext), pBeginInfo->pNext)',
                                                         'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBeginInfo))'},
                           'XGL_DYNAMIC_VP_STATE_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(XGL_VIEWPORT), pCreateInfo->pViewports);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(XGL_RECT), pCreateInfo->pScissors)',
                                                                'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'XGL_MEMORY_ALLOC_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(XGL_MEMORY_ALLOC_INFO), pAllocInfo);\n    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext)',
                                                     'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocInfo))'},
                           'XGL_GRAPHICS_PIPELINE_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO), pCreateInfo);\n    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext)',
                                                                 'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayoutInfoList), sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO), pSetLayoutInfoList);\n    if (pSetLayoutInfoList)\n        add_create_ds_layout_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayoutInfoList->pNext), pSetLayoutInfoList->pNext)',
                                                                     'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayoutInfoList))'},
                           'XGL_DESCRIPTOR_REGION_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DESCRIPTOR_REGION_CREATE_INFO), pCreateInfo);\n    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), rgCount * sizeof(XGL_DESCRIPTOR_TYPE_COUNT), pCreateInfo->pTypeCount)',
                                                                 'finalize_txt': 'glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));\n    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo))'},
                           'XGL_COMPUTE_PIPELINE_CREATE_INFO': {'add_txt': 'glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO), pCreateInfo);\n    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);\n    add_pipeline_shader_to_trace_packet(pHeader, (XGL_PIPELINE_SHADER*)&pPacket->pCreateInfo->cs, &pCreateInfo->cs)',
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
        custom_size_dict = {'pSetBindPoints': '(XGL_SHADER_STAGE_COMPUTE * sizeof(uint32_t))', # Accounting for largest possible array
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
                    ps.append('((pSubresource != NULL) ? sizeof(XGL_IMAGE_SUBRESOURCE) : 0)')
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
        for proto in self.protos:
            if 'UnmapMemory' == proto.name:
                func_body.append(self._gen_unmap_memory())
            elif 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                raw_packet_update_list = [] # non-ptr elements placed directly into packet
                ptr_packet_update_list = [] # ptr elements to be updated into packet
                return_txt = ''
                packet_size = []
                in_data_size = False # flag when we need to capture local input size variable for in/out size
                func_body.append('GLVTRACER_EXPORT %s XGLAPI __HOOKED_xgl%s(' % (proto.ret, proto.name))
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
                func_body.append('    struct_xgl%s* pPacket = NULL;' % proto.name)
                # functions that have non-standard sequence of  packet creation and calling real function
                # NOTE: Anytime we call the function before CREATE_TRACE_PACKET, need to add custom code for correctly tracking API call time
                if 'CreateInstance' == proto.name:
                    func_body.append('    uint64_t startTime;')
                    func_body.append('    glv_platform_thread_once(&gInitOnce, InitTracer);')
                    func_body.append('    SEND_ENTRYPOINT_ID(xgl%s);' % proto.name)
                    func_body.append('    if (real_xglCreateInstance == xglCreateInstance)')
                    func_body.append('    {')
                    func_body.append('        glv_platform_get_next_lib_sym((void **) &real_xglCreateInstance,"xglCreateInstance");')
                    func_body.append('    }')
                    func_body.append('    startTime = glv_get_time();')
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, sizeof(XGL_INSTANCE) + get_struct_chain_size((void*)pAppInfo) + ((pAllocCb == NULL) ? 0 :sizeof(XGL_ALLOC_CALLBACKS)));' % (proto.name))
                    func_body.append('    pHeader->entrypoint_begin_time = startTime;')
                    func_body.append('    if (isHooked == FALSE) {')
                    func_body.append('        AttachHooks();')
                    func_body.append('        AttachHooks_xgldbg();')
                    func_body.append('        AttachHooks_xglwsix11ext();')
                    func_body.append('    }')
                elif 'EnumerateGpus' == proto.name:
                    func_body.append('    uint64_t startTime;')
                    func_body.append('    SEND_ENTRYPOINT_ID(xgl%s);' % proto.name)
                    func_body.append('    startTime = glv_get_time();')
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                    func_body.append('    CREATE_TRACE_PACKET(xglEnumerateGpus, sizeof(uint32_t) + ((pGpus && pGpuCount) ? *pGpuCount * sizeof(XGL_PHYSICAL_GPU) : 0));')
                    func_body.append('    pHeader->entrypoint_begin_time = startTime;')
                elif 'EnumerateLayers' == proto.name:
                    func_body.append('    uint64_t startTime;')
                    func_body.append('    SEND_ENTRYPOINT_ID(xgl%s);' % proto.name)
                    func_body.append('    startTime = glv_get_time();')
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                    func_body.append('    size_t totStringSize = 0;')
                    func_body.append('    uint32_t i = 0;')
                    func_body.append('    for (i = 0; i < *pOutLayerCount; i++)')
                    func_body.append('        totStringSize += (pOutLayers[i] != NULL) ? strlen(pOutLayers[i]) + 1: 0;')
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, totStringSize + sizeof(size_t));' % (proto.name))
                    func_body.append('    pHeader->entrypoint_begin_time = startTime;')
                elif 'AllocDescriptorSets' == proto.name:
                    func_body.append('    uint64_t startTime;')
                    func_body.append('    SEND_ENTRYPOINT_ID(xgl%s);' % proto.name)
                    func_body.append('    startTime = glv_get_time();')
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                    func_body.append('    size_t customSize = (*pCount <= 0) ? (sizeof(XGL_DESCRIPTOR_SET)) : (*pCount * sizeof(XGL_DESCRIPTOR_SET));')
                    func_body.append('    CREATE_TRACE_PACKET(xglAllocDescriptorSets, sizeof(XGL_DESCRIPTOR_SET_LAYOUT) + customSize + sizeof(uint32_t));')
                    func_body.append('    pHeader->entrypoint_begin_time = startTime;')
                elif proto.name in ['CreateFramebuffer', 'CreateRenderPass', 'CreateDynamicViewportState', 
                                    'CreateDescriptorRegion', 'CmdWaitEvents', 'CmdPipelineBarrier']:
                    # these are regular case as far as sequence of tracing but have some custom size element
                    if 'CreateFramebuffer' == proto.name:
                        func_body.append('    int dsSize = (pCreateInfo != NULL && pCreateInfo->pDepthStencilAttachment != NULL) ? sizeof(XGL_DEPTH_STENCIL_BIND_INFO) : 0;')
                        func_body.append('    uint32_t colorCount = (pCreateInfo != NULL && pCreateInfo->pColorAttachments != NULL) ? pCreateInfo->colorAttachmentCount : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateFramebuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_FRAMEBUFFER));')
                    elif 'CreateRenderPass' == proto.name:
                        func_body.append('    uint32_t colorCount = (pCreateInfo != NULL && (pCreateInfo->pColorLoadOps != NULL || pCreateInfo->pColorStoreOps != NULL || pCreateInfo->pColorLoadClearValues != NULL)) ? pCreateInfo->colorAttachmentCount : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateRenderPass, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_RENDER_PASS));')
                    elif 'CreateDynamicViewportState' == proto.name:
                        func_body.append('    uint32_t vpsCount = (pCreateInfo != NULL && pCreateInfo->pViewports != NULL) ? pCreateInfo->viewportAndScissorCount : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateDynamicViewportState,  get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DYNAMIC_VP_STATE_OBJECT));')
                    elif 'CreateDescriptorRegion' == proto.name:
                        func_body.append('    uint32_t rgCount = (pCreateInfo != NULL && pCreateInfo->pTypeCount != NULL) ? pCreateInfo->count : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateDescriptorRegion,  get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DESCRIPTOR_REGION));')
                    else: # ['CmdWaitEvents', 'CmdPipelineBarrier']:
                        func_body.append('    size_t customSize;')
                        event_array_type = 'XGL_EVENT'
                        if 'CmdPipelineBarrier' == proto.name:
                            event_array_type = 'XGL_SET_EVENT'
                        func_body.append('    uint32_t eventCount = (%s != NULL && %s->pEvents != NULL) ? %s->eventCount : 0;' % (proto.params[-1].name, proto.params[-1].name, proto.params[-1].name))
                        func_body.append('    uint32_t mbCount = (%s != NULL && %s->ppMemBarriers != NULL) ? %s->memBarrierCount : 0;' % (proto.params[-1].name, proto.params[-1].name, proto.params[-1].name))
                        func_body.append('    customSize = (eventCount * sizeof(%s)) + mbCount * sizeof(void*) + calculate_memory_barrier_size(mbCount, %s->ppMemBarriers);' % (event_array_type, proto.params[-1].name))
                        func_body.append('    CREATE_TRACE_PACKET(xgl%s, sizeof(%s) + customSize);' % (proto.name, proto.params[-1].ty.strip('*').replace('const ', '')))
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                else:
                    if (0 == len(packet_size)):
                        func_body.append('    CREATE_TRACE_PACKET(xgl%s, 0);' % (proto.name))
                    else:
                        func_body.append('    CREATE_TRACE_PACKET(xgl%s, %s);' % (proto.name, ' + '.join(packet_size)))
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                if in_data_size:
                    func_body.append('    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;')
                func_body.append('    pPacket = interpret_body_as_xgl%s(pHeader);' % proto.name)
                func_body.append('\n'.join(raw_packet_update_list))
                if 'MapMemory' == proto.name: # Custom code for MapMem case
                    func_body.append('    if (ppData != NULL)')
                    func_body.append('    {')
                    func_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppData), sizeof(void*), *ppData);')
                    func_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));')
                    func_body.append('        add_data_to_mem_info(mem, *ppData);')
                    func_body.append('    }')
                    func_body.append('    pPacket->result = result;')
                    func_body.append('    FINISH_TRACE_PACKET();')
                elif 'EnumerateLayers' == proto.name: #custom code for EnumerateLayers case
                    func_body.append('    pPacket->gpu = gpu;')
                    func_body.append('    pPacket->maxLayerCount = maxLayerCount;')
                    func_body.append('    pPacket->maxStringSize = maxStringSize;')
                    func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOutLayerCount), sizeof(size_t), pOutLayerCount);')
                    func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOutLayerCount));')
                    func_body.append('    for (i = 0; i < *pOutLayerCount; i++) {')
                    func_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOutLayers[i]), ((pOutLayers[i] != NULL) ? strlen(pOutLayers[i]) + 1 : 0), pOutLayers[i]);')
                    func_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOutLayers[i]));')
                    func_body.append('    }')

                    func_body.append('    pPacket->pReserved = pReserved;')
                    func_body.append('    pPacket->result = result;')
                    func_body.append('    FINISH_TRACE_PACKET();')
                else:
                    for pp_dict in ptr_packet_update_list: #buff_ptr_indices:
                        func_body.append('    %s;' % (pp_dict['add_txt']))
                    # Some custom add_* and finalize_* function calls for Create* API calls
                    if proto.name in ['CmdWaitEvents', 'CmdPipelineBarrier']:
                        event_array_type = 'XGL_EVENT'
                        if 'CmdPipelineBarrier' == proto.name:
                            event_array_type = 'XGL_SET_EVENT'
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s->pEvents), eventCount * sizeof(%s), %s->pEvents);' % (proto.params[-1].name, event_array_type, proto.params[-1].name))
                        func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s->pEvents));' % (proto.params[-1].name))
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s->ppMemBarriers), mbCount * sizeof(void*), %s->ppMemBarriers);' % (proto.params[-1].name, proto.params[-1].name))
                        func_body.append('    uint32_t i, siz;')
                        func_body.append('    for (i = 0; i < mbCount; i++) {')
                        func_body.append('        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) %s->ppMemBarriers[i];' % proto.params[-1].name)
                        func_body.append('        switch (pNext->sType) {')
                        func_body.append('            case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:')
                        func_body.append('                siz = sizeof(XGL_MEMORY_BARRIER);')
                        func_body.append('                break;')
                        func_body.append('            case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:')
                        func_body.append('                siz = sizeof(XGL_BUFFER_MEMORY_BARRIER);')
                        func_body.append('                break;')
                        func_body.append('            case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:')
                        func_body.append('                siz = sizeof(XGL_IMAGE_MEMORY_BARRIER);')
                        func_body.append('                break;')
                        func_body.append('            default:')
                        func_body.append('                assert(0);')
                        func_body.append('                siz = 0;')
                        func_body.append('                break;')
                        func_body.append('        }')
                        func_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s->ppMemBarriers[i]), siz, %s->ppMemBarriers[i]);' % (proto.params[-1].name, proto.params[-1].name))
                        func_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s->ppMemBarriers[i]));' % (proto.params[-1].name))
                        func_body.append('    }')
                        func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s->ppMemBarriers));' % (proto.params[-1].name))
                    if 'void' not in proto.ret or '*' in proto.ret:
                        func_body.append('    pPacket->result = result;')
                    for pp_dict in ptr_packet_update_list:
                        if ('DEVICE_CREATE_INFO' not in proto.params[pp_dict['index']].ty) and ('APPLICATION_INFO' not in proto.params[pp_dict['index']].ty) and ('pUpdateChain' != proto.params[pp_dict['index']].name):
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
                func_body.append('GLVTRACER_EXPORT %s XGLAPI __HOOKED_xgl%s(' % (proto.ret, proto.name))
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
                func_body.append('    struct_xgl%s* pPacket = NULL;' % proto.name)
                if proto.name in thread_once_funcs:
                    func_body.append('    glv_platform_thread_once(&gInitOnce, InitTracer);')
                func_body.append('    SEND_ENTRYPOINT_ID(xgl%s);' % proto.name)
                if 'DbgRegisterMsgCallback' in proto.name:
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, sizeof(char));' % proto.name)
                else:
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, %s);' % (proto.name, packet_size))
                func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                func_body.append('    pPacket = interpret_body_as_xgl%s(pHeader);' % proto.name)
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

    def _generate_helper_funcs(self):
        hf_body = []
        hf_body.append('// Support for shadowing CPU mapped memory')
        hf_body.append('typedef struct _XGLAllocInfo {')
        hf_body.append('    XGL_GPU_SIZE   size;')
        hf_body.append('    XGL_GPU_MEMORY handle;')
        hf_body.append('    void           *pData;')
        hf_body.append('    BOOL           valid;')
        hf_body.append('} XGLAllocInfo;')
        hf_body.append('typedef struct _XGLMemInfo {')
        hf_body.append('    unsigned int numEntrys;')
        hf_body.append('    XGLAllocInfo *pEntrys;')
        hf_body.append('    XGLAllocInfo *pLastMapped;')
        hf_body.append('    unsigned int capacity;')
        hf_body.append('} XGLMemInfo;')
        hf_body.append('')
        hf_body.append('static XGLMemInfo g_memInfo = {0, NULL, NULL, 0};')
        hf_body.append('')
        hf_body.append('static void init_mem_info_entrys(XGLAllocInfo *ptr, const unsigned int num)')
        hf_body.append('{')
        hf_body.append('    unsigned int i;')
        hf_body.append('    for (i = 0; i < num; i++)')
        hf_body.append('    {')
        hf_body.append('        XGLAllocInfo *entry = ptr + i;')
        hf_body.append('        entry->pData = NULL;')
        hf_body.append('        entry->size  = 0;')
        hf_body.append('        entry->handle = NULL;')
        hf_body.append('        entry->valid = FALSE;')
        hf_body.append('    }')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('// caller must hold the g_memInfoLock')
        hf_body.append('static void init_mem_info()')
        hf_body.append('{')
        hf_body.append('    g_memInfo.numEntrys = 0;')
        hf_body.append('    g_memInfo.capacity = 4096;')
        hf_body.append('    g_memInfo.pLastMapped = NULL;')
        hf_body.append('')
        hf_body.append('    g_memInfo.pEntrys = GLV_NEW_ARRAY(XGLAllocInfo, g_memInfo.capacity);')
        hf_body.append('')
        hf_body.append('    if (g_memInfo.pEntrys == NULL)')
        hf_body.append('        glv_LogError("init_mem_info()  malloc failed\\n");')
        hf_body.append('    else')
        hf_body.append('        init_mem_info_entrys(g_memInfo.pEntrys, g_memInfo.capacity);')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('// caller must hold the g_memInfoLock')
        hf_body.append('static void delete_mem_info()')
        hf_body.append('{')
        hf_body.append('    GLV_DELETE(g_memInfo.pEntrys);')
        hf_body.append('    g_memInfo.pEntrys = NULL;')
        hf_body.append('    g_memInfo.numEntrys = 0;')
        hf_body.append('    g_memInfo.capacity = 0;')
        hf_body.append('    g_memInfo.pLastMapped = NULL;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('// caller must hold the g_memInfoLock')
        hf_body.append('static XGLAllocInfo * get_mem_info_entry()')
        hf_body.append('{')
        hf_body.append('    unsigned int i;')
        hf_body.append('    XGLAllocInfo *entry;')
        hf_body.append('    if (g_memInfo.numEntrys > g_memInfo.capacity)')
        hf_body.append('    {')
        hf_body.append('        glv_LogError("get_mem_info_entry() bad internal state numEntrys %u\\n", g_memInfo.numEntrys);')
        hf_body.append('        return NULL;')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    entry = g_memInfo.pEntrys;')
        hf_body.append('    for (i = 0; i < g_memInfo.numEntrys; i++)')
        hf_body.append('    {')
        hf_body.append('        if ((entry + i)->valid == FALSE)')
        hf_body.append('            return entry + i;')
        hf_body.append('    }')
        hf_body.append('    if (g_memInfo.numEntrys == g_memInfo.capacity)')
        hf_body.append('    {  // grow the array 2x')
        hf_body.append('        g_memInfo.capacity *= 2;')
        hf_body.append('        g_memInfo.pEntrys = (XGLAllocInfo *) GLV_REALLOC(g_memInfo.pEntrys, g_memInfo.capacity * sizeof(XGLAllocInfo));')
        hf_body.append('        if (g_memInfo.pEntrys == NULL)')
        hf_body.append('            glv_LogError("get_mem_info_entry() realloc failed\\n");')
        hf_body.append('        //glv_LogInfo("realloc memInfo from %u to %u\\n", g_memInfo.capacity /2, g_memInfo.capacity);')
        hf_body.append('        //init the newly added entrys')
        hf_body.append('        init_mem_info_entrys(g_memInfo.pEntrys + g_memInfo.capacity / 2, g_memInfo.capacity / 2);')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    assert(g_memInfo.numEntrys < g_memInfo.capacity);')
        hf_body.append('    entry = g_memInfo.pEntrys + g_memInfo.numEntrys;')
        hf_body.append('    g_memInfo.numEntrys++;')
        hf_body.append('    assert(entry->valid == FALSE);')
        hf_body.append('    return entry;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('// caller must hold the g_memInfoLock')
        hf_body.append('static XGLAllocInfo * find_mem_info_entry(const XGL_GPU_MEMORY handle)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry;')
        hf_body.append('    unsigned int i;')
        hf_body.append('    entry = g_memInfo.pEntrys;')
        hf_body.append('    if (g_memInfo.pLastMapped && g_memInfo.pLastMapped->handle == handle && g_memInfo.pLastMapped->valid)')
        hf_body.append('    {')
        hf_body.append('        return g_memInfo.pLastMapped;')
        hf_body.append('    }')
        hf_body.append('    for (i = 0; i < g_memInfo.numEntrys; i++)')
        hf_body.append('    {')
        hf_body.append('        if ((entry + i)->valid && (handle == (entry + i)->handle))')
        hf_body.append('        {')
        hf_body.append('            return entry + i;')
        hf_body.append('        }')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    return NULL;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static XGLAllocInfo * find_mem_info_entry_lock(const XGL_GPU_MEMORY handle)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *res;')
        hf_body.append('    glv_enter_critical_section(&g_memInfoLock);')
        hf_body.append('    res = find_mem_info_entry(handle);')
        hf_body.append('    glv_leave_critical_section(&g_memInfoLock);')
        hf_body.append('    return res;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_new_handle_to_mem_info(const XGL_GPU_MEMORY handle, XGL_GPU_SIZE size, void *pData)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry;')
        hf_body.append('')
        hf_body.append('    glv_enter_critical_section(&g_memInfoLock);')
        hf_body.append('    if (g_memInfo.capacity == 0)')
        hf_body.append('        init_mem_info();')
        hf_body.append('')
        hf_body.append('    entry = get_mem_info_entry();')
        hf_body.append('    if (entry)')
        hf_body.append('    {')
        hf_body.append('        entry->valid = TRUE;')
        hf_body.append('        entry->handle = handle;')
        hf_body.append('        entry->size = size;')
        hf_body.append('        entry->pData = pData;   // NOTE: xglFreeMemory will free this mem, so no malloc()')
        hf_body.append('    }')
        hf_body.append('    glv_leave_critical_section(&g_memInfoLock);')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_data_to_mem_info(const XGL_GPU_MEMORY handle, void *pData)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry;')
        hf_body.append('')
        hf_body.append('    glv_enter_critical_section(&g_memInfoLock);')
        hf_body.append('    entry = find_mem_info_entry(handle);')
        hf_body.append('    if (entry)')
        hf_body.append('    {')
        hf_body.append('        entry->pData = pData;')
        hf_body.append('    }')
        hf_body.append('    g_memInfo.pLastMapped = entry;')
        hf_body.append('    glv_leave_critical_section(&g_memInfoLock);')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void rm_handle_from_mem_info(const XGL_GPU_MEMORY handle)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry;')
        hf_body.append('')
        hf_body.append('    glv_enter_critical_section(&g_memInfoLock);')
        hf_body.append('    entry = find_mem_info_entry(handle);')
        hf_body.append('    if (entry)')
        hf_body.append('    {')
        hf_body.append('        entry->valid = FALSE;')
        hf_body.append('        entry->pData = NULL;')
        hf_body.append('        entry->size = 0;')
        hf_body.append('        entry->handle = NULL;')
        hf_body.append('')
        hf_body.append('        if (entry == g_memInfo.pLastMapped)')
        hf_body.append('            g_memInfo.pLastMapped = NULL;')
        hf_body.append('        // adjust numEntrys to be last valid entry in list')
        hf_body.append('        do {')
        hf_body.append('            entry =  g_memInfo.pEntrys + g_memInfo.numEntrys - 1;')
        hf_body.append('            if (entry->valid == FALSE)')
        hf_body.append('                g_memInfo.numEntrys--;')
        hf_body.append('        } while ((entry->valid == FALSE) && (g_memInfo.numEntrys > 0));')
        hf_body.append('        if (g_memInfo.numEntrys == 0)')
        hf_body.append('            delete_mem_info();')
        hf_body.append('    }')
        hf_body.append('    glv_leave_critical_section(&g_memInfoLock);')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_begin_cmdbuf_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)')
        hf_body.append('{')
        hf_body.append('    const XGL_CMD_BUFFER_BEGIN_INFO* pInNow = pIn;')
        hf_body.append('    XGL_CMD_BUFFER_BEGIN_INFO** ppOutNext = (XGL_CMD_BUFFER_BEGIN_INFO**)ppOut;')
        hf_body.append('    while (pInNow != NULL)')
        hf_body.append('    {')
        hf_body.append('        XGL_CMD_BUFFER_BEGIN_INFO** ppOutNow = ppOutNext;')
        hf_body.append('        ppOutNext = NULL;')
        hf_body.append('')
        hf_body.append('        switch (pInNow->sType)')
        hf_body.append('        {')
        hf_body.append('            case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:')
        hf_body.append('            {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO), pInNow);')
        hf_body.append('                ppOutNext = (XGL_CMD_BUFFER_BEGIN_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            default:')
        hf_body.append('                assert(!"Encountered an unexpected type in cmdbuffer_begin_info list");')
        hf_body.append('        }')
        hf_body.append('        pInNow = (XGL_CMD_BUFFER_BEGIN_INFO*)pInNow->pNext;')
        hf_body.append('    }')
        hf_body.append('    return;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_alloc_memory_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)')
        hf_body.append('{')
        hf_body.append('    const XGL_MEMORY_ALLOC_INFO* pInNow = pIn;')
        hf_body.append('    XGL_MEMORY_ALLOC_INFO** ppOutNext = (XGL_MEMORY_ALLOC_INFO**)ppOut;')
        hf_body.append('    while (pInNow != NULL)')
        hf_body.append('    {')
        hf_body.append('        XGL_MEMORY_ALLOC_INFO** ppOutNow = ppOutNext;')
        hf_body.append('        ppOutNext = NULL;')
        hf_body.append('')
        hf_body.append('        switch (pInNow->sType)')
        hf_body.append('        {')
        hf_body.append('        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:')
        hf_body.append('        {')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO), pInNow);')
        hf_body.append('            ppOutNext = (XGL_MEMORY_ALLOC_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:')
        hf_body.append('        {')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO), pInNow);')
        hf_body.append('            ppOutNext = (XGL_MEMORY_ALLOC_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        default:')
        hf_body.append('            assert(!"Encountered an unexpected type in memory_alloc_info list");')
        hf_body.append('        }')
        hf_body.append('        pInNow = (XGL_MEMORY_ALLOC_INFO*)pInNow->pNext;')
        hf_body.append('    }')
        hf_body.append('    return;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static size_t calculate_memory_barrier_size(uint32_t mbCount, const void** ppMemBarriers)')
        hf_body.append('{')
        hf_body.append('    uint32_t i, siz=0;')
        hf_body.append('    for (i = 0; i < mbCount; i++) {')
        hf_body.append('        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) ppMemBarriers[i];')
        hf_body.append('        switch (pNext->sType) {')
        hf_body.append('            case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:')
        hf_body.append('                siz += sizeof(XGL_MEMORY_BARRIER);')
        hf_body.append('                break;')
        hf_body.append('            case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:')
        hf_body.append('                siz += sizeof(XGL_BUFFER_MEMORY_BARRIER);')
        hf_body.append('                break;')
        hf_body.append('            case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:')
        hf_body.append('                siz += sizeof(XGL_IMAGE_MEMORY_BARRIER);')
        hf_body.append('                break;')
        hf_body.append('            default:')
        hf_body.append('                assert(0);')
        hf_body.append('                break;')
        hf_body.append('        }')
        hf_body.append('    }')
        hf_body.append('    return siz;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_pipeline_shader_to_trace_packet(glv_trace_packet_header* pHeader, XGL_PIPELINE_SHADER* packetShader, const XGL_PIPELINE_SHADER* paramShader)')
        hf_body.append('{')
        hf_body.append('    uint32_t i;')
        hf_body.append('    // constant buffers')
        hf_body.append('    if (paramShader->linkConstBufferCount > 0 && paramShader->pLinkConstBufferInfo != NULL)')
        hf_body.append('    {')
        hf_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo), sizeof(XGL_LINK_CONST_BUFFER) * paramShader->linkConstBufferCount, paramShader->pLinkConstBufferInfo);')
        hf_body.append('        for (i = 0; i < paramShader->linkConstBufferCount; i++)')
        hf_body.append('        {')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo[i].pBufferData), packetShader->pLinkConstBufferInfo[i].bufferSize, paramShader->pLinkConstBufferInfo[i].pBufferData);')
        hf_body.append('        }')
        hf_body.append('    }')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void finalize_pipeline_shader_address(glv_trace_packet_header* pHeader, const XGL_PIPELINE_SHADER* packetShader)')
        hf_body.append('{')
        hf_body.append('    uint32_t i;')
        hf_body.append('    // constant buffers')
        hf_body.append('    if (packetShader->linkConstBufferCount > 0 && packetShader->pLinkConstBufferInfo != NULL)')
        hf_body.append('    {')
        hf_body.append('        for (i = 0; i < packetShader->linkConstBufferCount; i++)')
        hf_body.append('        {')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)&(packetShader->pLinkConstBufferInfo[i].pBufferData));')
        hf_body.append('        }')
        hf_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(packetShader->pLinkConstBufferInfo));')
        hf_body.append('    }')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_create_ds_layout_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)')
        hf_body.append('{')
        hf_body.append('    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pInNow = pIn;')
        hf_body.append('    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO** ppOutNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO**)ppOut;')
        hf_body.append('    while (pInNow != NULL)')
        hf_body.append('    {')
        hf_body.append('        XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO** ppOutNow = ppOutNext;')
        hf_body.append('        ppOutNext = NULL;')
        hf_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO), pInNow);')
        hf_body.append('        ppOutNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('        glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('        pInNow = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pInNow->pNext;')
        hf_body.append('    }')
        hf_body.append('    return;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_update_descriptors_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)')
        hf_body.append('{')
        hf_body.append('    const XGL_UPDATE_SAMPLERS* pInNow = pIn;')
        hf_body.append('    XGL_UPDATE_SAMPLERS** ppOutNext = (XGL_UPDATE_SAMPLERS**)ppOut;')
        hf_body.append('    while (pInNow != NULL)')
        hf_body.append('    {')
        hf_body.append('        XGL_UPDATE_SAMPLERS** ppOutNow = ppOutNext;')
        hf_body.append('        ppOutNext = NULL;')
        hf_body.append('        switch (pInNow->sType)')
        hf_body.append('        {')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:')
        hf_body.append('        {')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_SAMPLERS), pInNow);')
        hf_body.append('            XGL_UPDATE_SAMPLERS* pPacket = (XGL_UPDATE_SAMPLERS*)*ppOutNow;')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplers, ((XGL_UPDATE_SAMPLERS*)pInNow)->count * sizeof(XGL_SAMPLER), ((XGL_UPDATE_SAMPLERS*)pInNow)->pSamplers);')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplers));')
        hf_body.append('            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:')
        hf_body.append('        {')
        #hf_body.append('            totalUpdateSize += sizeof(XGL_UPDATE_SAMPLER_TEXTURES) + ((XGL_UPDATE_SAMPLER_TEXTURES*)pNext)->count * (sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO) + sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_SAMPLER_TEXTURES), pInNow);')
        hf_body.append('            XGL_UPDATE_SAMPLER_TEXTURES* pPacket = (XGL_UPDATE_SAMPLER_TEXTURES*)*ppOutNow;')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplerImageViews, ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->count * sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO), ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->pSamplerImageViews);')
        # TODO : This is still broken. How to update the original XGL_SAMPLER_IMAGE_VIEW_INFO struct ptrs to have correct address for newly added XGL_IMAGE_VIEW_ATTACH_INFO blocks below?
        hf_body.append('            uint32_t i;')
        hf_body.append('            for (i = 0; i < ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->count; i++) {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplerImageViews[i].pImageView, sizeof(XGL_IMAGE_VIEW_ATTACH_INFO), ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->pSamplerImageViews[i].pImageView);')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplerImageViews[i].pImageView));')
        hf_body.append('            }')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplerImageViews));')
        hf_body.append('            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:')
        hf_body.append('        {')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_IMAGES), pInNow);')
        hf_body.append('            XGL_UPDATE_IMAGES* pPacket = (XGL_UPDATE_IMAGES*)*ppOutNow;')
        hf_body.append('            uint32_t i;')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pImageViews, ((XGL_UPDATE_IMAGES*)pInNow)->count * sizeof(XGL_IMAGE_VIEW_ATTACH_INFO *), ((XGL_UPDATE_IMAGES*)pInNow)->pImageViews);')
        hf_body.append('            for (i = 0; i < ((XGL_UPDATE_IMAGES*)pInNow)->count; i++) {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pImageViews[i], sizeof(XGL_IMAGE_VIEW_ATTACH_INFO), ((XGL_UPDATE_IMAGES*)pInNow)->pImageViews[i]);')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageViews[i]));')
        hf_body.append('            }')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageViews));')
        hf_body.append('            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:')
        hf_body.append('        {')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_BUFFERS), pInNow);')
        hf_body.append('            XGL_UPDATE_BUFFERS* pPacket = (XGL_UPDATE_BUFFERS*)*ppOutNow;')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pBufferViews, ((XGL_UPDATE_BUFFERS*)pInNow)->count * sizeof(XGL_BUFFER_VIEW_ATTACH_INFO *), ((XGL_UPDATE_BUFFERS*)pInNow)->pBufferViews);')
        hf_body.append('            uint32_t i;')
        hf_body.append('            for (i = 0; i < ((XGL_UPDATE_BUFFERS*)pInNow)->count; i++) {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pBufferViews[i], sizeof(XGL_BUFFER_VIEW_ATTACH_INFO), ((XGL_UPDATE_BUFFERS*)pInNow)->pBufferViews[i]);')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBufferViews[i]));')
        hf_body.append('            }')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBufferViews));')
        hf_body.append('            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:')
        hf_body.append('        {')
        hf_body.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_AS_COPY), pInNow);')
        hf_body.append('            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;')
        hf_body.append('            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('            default:')
        hf_body.append('                assert(0);')
        hf_body.append('        }')
        hf_body.append('        pInNow = (XGL_UPDATE_SAMPLERS*)pInNow->pNext;')
        hf_body.append('    }')
        hf_body.append('    return;')
        hf_body.append('}')
        hf_body.append('')
        pipe_state_list = ['IA_STATE_CREATE_INFO',
                           'TESS_STATE_CREATE_INFO',
                           'RS_STATE_CREATE_INFO',
                           'DS_STATE_CREATE_INFO',
                           'VP_STATE_CREATE_INFO',
                           'MS_STATE_CREATE_INFO'
                           ]
        hf_body.append('static void add_pipeline_state_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)')
        hf_body.append('{')
        hf_body.append('    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pInNow = pIn;')
        hf_body.append('    XGL_GRAPHICS_PIPELINE_CREATE_INFO** ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)ppOut;')
        hf_body.append('    while (pInNow != NULL)')
        hf_body.append('    {')
        hf_body.append('        XGL_GRAPHICS_PIPELINE_CREATE_INFO** ppOutNow = ppOutNext;')
        hf_body.append('        ppOutNext = NULL;')
        hf_body.append('')
        hf_body.append('        switch (pInNow->sType)')
        hf_body.append('        {')
        for pipe_state in pipe_state_list:
            hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_%s:' % pipe_state)
            hf_body.append('            {')
            hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_%s), pInNow);' % pipe_state)
            hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
            hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
            hf_body.append('                break;')
            hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                XGL_PIPELINE_CB_STATE_CREATE_INFO *pPacket = NULL;')
        hf_body.append('                XGL_PIPELINE_CB_STATE_CREATE_INFO *pIn = NULL;')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_CB_STATE_CREATE_INFO), pInNow);')
        hf_body.append('                pPacket = (XGL_PIPELINE_CB_STATE_CREATE_INFO*) *ppOutNow;')
        hf_body.append('                pIn = (XGL_PIPELINE_CB_STATE_CREATE_INFO*) pInNow;')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pAttachments, pIn->attachmentCount * sizeof(XGL_PIPELINE_CB_ATTACHMENT_STATE), pIn->pAttachments);')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAttachments));')
        hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pPacket = NULL;')
        hf_body.append('                XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pInPacket = NULL;')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO), pInNow);')
        hf_body.append('                pPacket = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) *ppOutNow;')
        hf_body.append('                pInPacket = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) pInNow;')
        hf_body.append('                add_pipeline_shader_to_trace_packet(pHeader, &pPacket->shader, &pInPacket->shader);')
        hf_body.append('                finalize_pipeline_shader_address(pHeader, &pPacket->shader);')
        hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pPacket = NULL;')
        hf_body.append('                XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pIn = NULL;')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO), pInNow);')
        hf_body.append('                pPacket = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*) *ppOutNow;')
        hf_body.append('                pIn = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*) pInNow;')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pVertexBindingDescriptions, pIn->bindingCount * sizeof(XGL_VERTEX_INPUT_BINDING_DESCRIPTION), pIn->pVertexBindingDescriptions);')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexBindingDescriptions));')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pVertexAttributeDescriptions, pIn->attributeCount * sizeof(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION), pIn->pVertexAttributeDescriptions);')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexAttributeDescriptions));')
        hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            default:')
        hf_body.append('                assert(!"Encountered an unexpected type in pipeline state list");')
        hf_body.append('        }')
        hf_body.append('        pInNow = (XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pInNow->pNext;')
        hf_body.append('    }')
        hf_body.append('    return;')
        hf_body.append('}')
        return "\n".join(hf_body)

    def _generate_packet_id_enum(self):
        pid_enum = []
        pid_enum.append('enum GLV_TRACE_PACKET_ID_XGL')
        pid_enum.append('{')
        first_func = True
        for proto in self.protos:
            if first_func:
                first_func = False
                pid_enum.append('    GLV_TPI_XGL_xglApiVersion = GLV_TPI_BEGIN_API_HERE,')
                pid_enum.append('    GLV_TPI_XGL_xgl%s,' % proto.name)
            else:
                pid_enum.append('    GLV_TPI_XGL_xgl%s,' % proto.name)
        pid_enum.append('};\n')
        return "\n".join(pid_enum)

    def _generate_stringify_func(self):
        func_body = []
        func_body.append('static const char *stringify_xgl_packet_id(const enum GLV_TRACE_PACKET_ID_XGL id, const glv_trace_packet_header* pHeader)')
        func_body.append('{')
        func_body.append('    static char str[1024];')
        func_body.append('    switch(id) {')
        func_body.append('    case GLV_TPI_XGL_xglApiVersion:')
        func_body.append('    {')
        func_body.append('        struct_xglApiVersion* pPacket = (struct_xglApiVersion*)(pHeader->pBody);')
        func_body.append('        snprintf(str, 1024, "xglApiVersion = 0x%x", pPacket->version);')
        func_body.append('        return str;')
        func_body.append('    }')
        for proto in self.protos:
            func_body.append('    case GLV_TPI_XGL_xgl%s:' % proto.name)
            func_body.append('    {')
            func_str = 'xgl%s(' % proto.name
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
            func_body.append('        struct_xgl%s* pPacket = (struct_xgl%s*)(pHeader->pBody);' % (proto.name, proto.name))
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
        interp_func_body.append('static glv_trace_packet_header* interpret_trace_packet_xgl(glv_trace_packet_header* pHeader)')
        interp_func_body.append('{')
        interp_func_body.append('    if (pHeader == NULL)')
        interp_func_body.append('    {')
        interp_func_body.append('        return NULL;')
        interp_func_body.append('    }')
        interp_func_body.append('    switch (pHeader->packet_id)')
        interp_func_body.append('    {')
        interp_func_body.append('        case GLV_TPI_XGL_xglApiVersion:\n        {')
        interp_func_body.append('            return interpret_body_as_xglApiVersion(pHeader, TRUE)->header;\n        }')
        for proto in self.protos:
            interp_func_body.append('        case GLV_TPI_XGL_xgl%s:\n        {' % proto.name)
            header_prefix = 'h'
            if 'Wsi' in proto.name or 'Dbg' in proto.name:
                header_prefix = 'pH'
            interp_func_body.append('            return interpret_body_as_xgl%s(pHeader)->%seader;\n        }' % (proto.name, header_prefix))
        interp_func_body.append('        default:')
        interp_func_body.append('            return NULL;')
        interp_func_body.append('    }')
        interp_func_body.append('    return NULL;')
        interp_func_body.append('}')
        return "\n".join(interp_func_body)

    def _generate_struct_util_funcs(self):
        pid_enum = []
        pid_enum.append('//=============================================================================')
        pid_enum.append('static void add_XGL_APPLICATION_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_APPLICATION_INFO** ppStruct, const XGL_APPLICATION_INFO *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_APPLICATION_INFO), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pAppName), strlen(pInStruct->pAppName) + 1, pInStruct->pAppName);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pEngineName), strlen(pInStruct->pEngineName) + 1, pInStruct->pEngineName);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pAppName));')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pEngineName));')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&*ppStruct);')
        pid_enum.append('};\n')
        pid_enum.append('//=============================================================================\n')
        pid_enum.append('static void add_XGL_DEVICE_CREATE_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_DEVICE_CREATE_INFO** ppStruct, const XGL_DEVICE_CREATE_INFO *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_DEVICE_CREATE_INFO), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pRequestedQueues, pInStruct->queueRecordCount*sizeof(XGL_DEVICE_QUEUE_CREATE_INFO), pInStruct->pRequestedQueues);')
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
        pid_enum.append('    XGL_LAYER_CREATE_INFO *pNext = ( XGL_LAYER_CREATE_INFO *) pInStruct->pNext;')
        pid_enum.append('    while (pNext != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        if ((pNext->sType == XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pNext->layerCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppStruct)->pNext)), sizeof(XGL_LAYER_CREATE_INFO), pNext);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void**)(&((*ppStruct)->pNext)));')
        pid_enum.append('            XGL_LAYER_CREATE_INFO **ppOutStruct = (XGL_LAYER_CREATE_INFO **) &((*ppStruct)->pNext);')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppOutStruct)->ppActiveLayerNames), pNext->layerCount * sizeof(char *), pNext->ppActiveLayerNames);')
        pid_enum.append('            for (i = 0; i < pNext->layerCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppOutStruct)->ppActiveLayerNames[i])), strlen(pNext->ppActiveLayerNames[i]) + 1, pNext->ppActiveLayerNames[i]);')
        pid_enum.append('                glv_finalize_buffer_address(pHeader, (void**)(&((*ppOutStruct)->ppActiveLayerNames[i])));')
        pid_enum.append('            }')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void **)&(*ppOutStruct)->ppActiveLayerNames);')
        pid_enum.append('        }')
        pid_enum.append('        pNext = ( XGL_LAYER_CREATE_INFO *) pNext->pNext;')
        pid_enum.append('    }')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)ppStruct);')
        pid_enum.append('}\n')
        pid_enum.append('static XGL_DEVICE_CREATE_INFO* interpret_XGL_DEVICE_CREATE_INFO(glv_trace_packet_header*  pHeader, intptr_t ptr_variable)')
        pid_enum.append('{')
        pid_enum.append('    XGL_DEVICE_CREATE_INFO* pXGL_DEVICE_CREATE_INFO = (XGL_DEVICE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)ptr_variable);\n')
        pid_enum.append('    if (pXGL_DEVICE_CREATE_INFO != NULL)')
        pid_enum.append('    {')
        pid_enum.append('            uint32_t i;')
        pid_enum.append('            const char** pNames;')
        pid_enum.append('        pXGL_DEVICE_CREATE_INFO->pRequestedQueues = (const XGL_DEVICE_QUEUE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->pRequestedQueues);\n')
        pid_enum.append('        if (pXGL_DEVICE_CREATE_INFO->extensionCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames = (const char *const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames);')
        pid_enum.append('            pNames = (const char**)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames;')
        pid_enum.append('            for (i = 0; i < pXGL_DEVICE_CREATE_INFO->extensionCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                pNames[i] = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames[i]));')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('        XGL_LAYER_CREATE_INFO *pNext = ( XGL_LAYER_CREATE_INFO *) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->pNext);')
        pid_enum.append('        while (pNext != NULL)')
        pid_enum.append('        {')
        pid_enum.append('            if ((pNext->sType == XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pNext->layerCount > 0)')
        pid_enum.append('            {')
        pid_enum.append('                pNext->ppActiveLayerNames = (const char**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pNext->ppActiveLayerNames));')
        pid_enum.append('                pNames = (const char**)pNext->ppActiveLayerNames;')
        pid_enum.append('                for (i = 0; i < pNext->layerCount; i++)')
        pid_enum.append('                {')
        pid_enum.append('                    pNames[i] = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pNext->ppActiveLayerNames[i]));')
        pid_enum.append('                }')
        pid_enum.append('            }')
        pid_enum.append('            pNext = ( XGL_LAYER_CREATE_INFO *) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);')
        pid_enum.append('        }')
        pid_enum.append('    }\n')
        pid_enum.append('    return pXGL_DEVICE_CREATE_INFO;')
        pid_enum.append('}\n')
        pid_enum.append('static void interpret_pipeline_shader(glv_trace_packet_header*  pHeader, XGL_PIPELINE_SHADER* pShader)')
        pid_enum.append('{')
        pid_enum.append('    if (pShader != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        // constant buffers')
        pid_enum.append('        if (pShader->linkConstBufferCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            uint32_t i;')
        pid_enum.append('            pShader->pLinkConstBufferInfo = (const XGL_LINK_CONST_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo);')
        pid_enum.append('            for (i = 0; i < pShader->linkConstBufferCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                XGL_LINK_CONST_BUFFER* pBuffer = (XGL_LINK_CONST_BUFFER*)pShader->pLinkConstBufferInfo;')
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
        custom_case_dict = { 'CreateInstance' : {'param': 'pAppInfo', 'txt': ['XGL_APPLICATION_INFO* pInfo = (XGL_APPLICATION_INFO*)pPacket->pAppInfo;\n',
                                                       'pInfo->pAppName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pAppName);\n',
                                                       'pInfo->pEngineName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pEngineName);']},
                             'CreateShader' : {'param': 'pCreateInfo', 'txt': ['XGL_SHADER_CREATE_INFO* pInfo = (XGL_SHADER_CREATE_INFO*)pPacket->pCreateInfo;\n',
                                               'pInfo->pCode = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pCode);']},
                             'CreateDynamicViewportState' : {'param': 'pCreateInfo', 'txt': ['XGL_DYNAMIC_VP_STATE_CREATE_INFO* pInfo = (XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pPacket->pCreateInfo;\n',
                                                                                             'pInfo->pViewports = (XGL_VIEWPORT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pViewports);\n',
                                                                                             'pInfo->pScissors = (XGL_RECT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pScissors);']},
                             'CreateFramebuffer' : {'param': 'pCreateInfo', 'txt': ['XGL_FRAMEBUFFER_CREATE_INFO* pInfo = (XGL_FRAMEBUFFER_CREATE_INFO*)pPacket->pCreateInfo;\n',
                                                    'pInfo->pColorAttachments = (XGL_COLOR_ATTACHMENT_BIND_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorAttachments);\n',
                                                    'pInfo->pDepthStencilAttachment = (XGL_DEPTH_STENCIL_BIND_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pDepthStencilAttachment);\n']},
                             'CreateRenderPass' : {'param': 'pCreateInfo', 'txt': ['XGL_RENDER_PASS_CREATE_INFO* pInfo = (XGL_RENDER_PASS_CREATE_INFO*)pPacket->pCreateInfo;\n',
                                                   'pInfo->pColorLoadOps = (XGL_ATTACHMENT_LOAD_OP*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorLoadOps);\n',
                                                   'pInfo->pColorStoreOps = (XGL_ATTACHMENT_STORE_OP*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorStoreOps);\n',
                                                   'pInfo->pColorLoadClearValues = (XGL_CLEAR_COLOR*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorLoadClearValues);\n']},
                             'CreateDescriptorRegion' : {'param': 'pCreateInfo', 'txt': ['XGL_DESCRIPTOR_REGION_CREATE_INFO* pInfo = (XGL_DESCRIPTOR_REGION_CREATE_INFO*)pPacket->pCreateInfo;\n',
                                                                                             'pInfo->pTypeCount = (XGL_DESCRIPTOR_TYPE_COUNT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pTypeCount);\n']},
                             'CmdWaitEvents' : {'param': 'pWaitInfo', 'txt': ['XGL_EVENT_WAIT_INFO* pInfo = (XGL_EVENT_WAIT_INFO*)pPacket->pWaitInfo;\n',
                                                                          'pInfo->pEvents = (XGL_EVENT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pWaitInfo->pEvents);\n',
                                                                          'pInfo->ppMemBarriers = (const void**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pWaitInfo->ppMemBarriers);\n',
                                                                          'uint32_t i;\n',
                                                                          'for (i = 0; i < pInfo->memBarrierCount; i++) {\n',
                                                                          '    void** ppLocalMemBarriers = (void**)&pInfo->ppMemBarriers[i];\n',
                                                                          '    *ppLocalMemBarriers = (void*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->ppMemBarriers[i]);\n',
                                                                          '}']},
                             'CmdPipelineBarrier' : {'param': 'pBarrier', 'txt': ['XGL_PIPELINE_BARRIER* pBarrier = (XGL_PIPELINE_BARRIER*)pPacket->pBarrier;\n',
                                                                          'pBarrier->pEvents = (XGL_SET_EVENT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBarrier->pEvents);\n',
                                                                          'pBarrier->ppMemBarriers = (const void**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBarrier->ppMemBarriers);\n',
                                                                          'uint32_t i;\n',
                                                                          'for (i = 0; i < pBarrier->memBarrierCount; i++) {\n',
                                                                          '    void** ppLocalMemBarriers = (void**)&pBarrier->ppMemBarriers[i];\n',
                                                                          '    *ppLocalMemBarriers = (void*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pBarrier->ppMemBarriers[i]);\n',
                                                                          '}']},
                             'CreateDescriptorSetLayout' : {'param': 'pSetLayoutInfoList', 'txt': ['if (pPacket->pSetLayoutInfoList->sType == XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {\n',
                                                                                         '    // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         '    void** ppNextVoidPtr = (void**)&(pPacket->pSetLayoutInfoList->pNext);\n',
                                                                                         '    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetLayoutInfoList->pNext);\n',
                                                                                         '    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pPacket->pSetLayoutInfoList->pNext;\n',
                                                                                         '    while (NULL != pNext)\n', '    {\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:\n',
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
                                                                                         '        pNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pNext->pNext;\n',
                                                                                         '     }\n',
                                                                                         '} else {\n',
                                                                                         '     // This is unexpected.\n',
                                                                                         '     glv_LogError("CreateDescriptorSetLayout must have LayoutInfoList stype of XGL_STRCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO\\n");\n',
                                                                                         '     pPacket->header = NULL;\n',
                                                                                         '}']},
                             'BeginCommandBuffer' : {'param': 'pBeginInfo', 'txt': ['if (pPacket->pBeginInfo->sType == XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO) {\n',
                                                                                         '    // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         '    XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO** ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO**)&(pPacket->pBeginInfo->pNext);\n',
                                                                                         '    *ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBeginInfo->pNext);\n',
                                                                                         '    XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pNext = *ppNext;\n',
                                                                                         '    while (NULL != pNext)\n', '    {\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO**) &pNext->pNext;\n',
                                                                                         '                *ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '                glv_LogError("Encountered an unexpected type in begin command buffer list.\\n");\n',
                                                                                         '                pPacket->header = NULL;\n',
                                                                                         '                pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pNext->pNext;\n',
                                                                                         '    }\n',
                                                                                         '} else {\n',
                                                                                         '    // This is unexpected.\n',
                                                                                         '    glv_LogError("BeginCommandBuffer must have BeginInfo stype of XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO.\\n");\n',
                                                                                         '    pPacket->header = NULL;\n',
                                                                                         '}']},
                             'AllocMemory' : {'param': 'pAllocInfo', 'txt': ['if (pPacket->pAllocInfo->sType == XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO) {\n',
                                                                                         '    XGL_MEMORY_ALLOC_INFO** ppNext = (XGL_MEMORY_ALLOC_INFO**) &(pPacket->pAllocInfo->pNext);\n',
                                                                                         '    *ppNext = (XGL_MEMORY_ALLOC_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocInfo->pNext);\n',
                                                                                         '    XGL_MEMORY_ALLOC_INFO* pNext = (XGL_MEMORY_ALLOC_INFO*) *ppNext;\n',
                                                                                         '    while (NULL != pNext)\n', '    {\n',
                                                                                         '        switch(pNext->sType)\n', '        {\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                ppNext = (XGL_MEMORY_ALLOC_INFO **) &(pNext->pNext);\n',
                                                                                         '                *ppNext = (XGL_MEMORY_ALLOC_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '               glv_LogError("Encountered an unexpected type alloc memory list.\\n");\n',
                                                                                         '               pPacket->header = NULL;\n',
                                                                                         '               pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (XGL_MEMORY_ALLOC_INFO*)pNext->pNext;\n',
                                                                                         '    }\n',
                                                                                         '} else {\n',
                                                                                         '    // This is unexpected.\n',
                                                                                         '    glv_LogError("AllocMemory must have AllocInfo stype of XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO.\\n");\n',
                                                                                         '    pPacket->header = NULL;\n',
                                                                                         '}']},
                             'UpdateDescriptors' : {'param': 'pUpdateChain', 'txt': ['XGL_UPDATE_SAMPLERS* pNext = (XGL_UPDATE_SAMPLERS*)pPacket->pUpdateChain;\n',
                                                                                         'while ((NULL != pNext) && (XGL_NULL_HANDLE != pNext))\n', '{\n',
                                                                                         '    switch(pNext->sType)\n', '    {\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            XGL_UPDATE_SAMPLERS* pUS = (XGL_UPDATE_SAMPLERS*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            pUS->pSamplers = (XGL_SAMPLER*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUS->pSamplers);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            XGL_UPDATE_SAMPLER_TEXTURES* pUST = (XGL_UPDATE_SAMPLER_TEXTURES*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            pUST->pSamplerImageViews = (XGL_SAMPLER_IMAGE_VIEW_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUST->pSamplerImageViews);\n',
                                                                                         '            uint32_t i;\n',
                                                                                         '            for (i = 0; i < pUST->count; i++) {\n',
                                                                                         '                XGL_IMAGE_VIEW_ATTACH_INFO** ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO**)&pUST->pSamplerImageViews[i].pImageView;\n',
                                                                                         '                *ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUST->pSamplerImageViews[i].pImageView);\n',
                                                                                         '            }\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            XGL_UPDATE_IMAGES* pUI = (XGL_UPDATE_IMAGES*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            XGL_IMAGE_VIEW_ATTACH_INFO** ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO**)&pUI->pImageViews;\n',
                                                                                         '            *ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUI->pImageViews);\n',
                                                                                         '            uint32_t i;\n',
                                                                                         '            for (i = 0; i < pUI->count; i++) {\n',
                                                                                         '                XGL_IMAGE_VIEW_ATTACH_INFO** ppLocalImageViews = (XGL_IMAGE_VIEW_ATTACH_INFO**)&pUI->pImageViews[i];\n',
                                                                                         '                *ppLocalImageViews = (XGL_IMAGE_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUI->pImageViews[i]);\n',
                                                                                         '            }\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            XGL_UPDATE_BUFFERS* pUB = (XGL_UPDATE_BUFFERS*)pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            XGL_BUFFER_VIEW_ATTACH_INFO** ppLocalBufferView = (XGL_BUFFER_VIEW_ATTACH_INFO**)&pUB->pBufferViews;\n',
                                                                                         '            *ppLocalBufferView = (XGL_BUFFER_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUB->pBufferViews);\n',
                                                                                         '            uint32_t i;\n',
                                                                                         '            for (i = 0; i < pUB->count; i++) {\n',
                                                                                         '                XGL_BUFFER_VIEW_ATTACH_INFO** ppLocalBufferViews = (XGL_BUFFER_VIEW_ATTACH_INFO**)&pUB->pBufferViews[i];\n',
                                                                                         '                *ppLocalBufferViews = (XGL_BUFFER_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUB->pBufferViews[i]);\n',
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
                                                                                         '    pNext = (XGL_UPDATE_SAMPLERS*)pNext->pNext;\n',
                                                                                         '}']},
                             'CreateGraphicsPipeline' : {'param': 'pCreateInfo', 'txt': ['if (pPacket->pCreateInfo->sType == XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {\n',
                                                                                         '    // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         '    void** ppNextVoidPtr = (void**)&pPacket->pCreateInfo->pNext;\n',
                                                                                         '    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pNext);\n',
                                                                                         '    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;\n',
                                                                                         '    while ((NULL != pNext) && (XGL_NULL_HANDLE != pNext))\n', '{\n',
                                                                                         '        switch(pNext->sType)\n', '    {\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                XGL_PIPELINE_CB_STATE_CREATE_INFO *pCb = (XGL_PIPELINE_CB_STATE_CREATE_INFO *) pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                pCb->pAttachments = (XGL_PIPELINE_CB_ATTACHMENT_STATE*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pCb->pAttachments);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                interpret_pipeline_shader(pHeader, &pNext->shader);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:\n',
                                                                                         '            {\n',
                                                                                         '                void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '                XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pVi = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *) pNext;\n',
                                                                                         '                *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '                pVi->pVertexBindingDescriptions = (XGL_VERTEX_INPUT_BINDING_DESCRIPTION*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexBindingDescriptions);\n',
                                                                                         '                pVi->pVertexAttributeDescriptions = (XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexAttributeDescriptions);\n',
                                                                                         '                break;\n',
                                                                                         '            }\n',
                                                                                         '            default:\n',
                                                                                         '            {\n',
                                                                                         '               glv_LogError("Encountered an unexpected type in pipeline state list.\\n");\n',
                                                                                         '               pPacket->header = NULL;\n',
                                                                                         '               pNext->pNext = NULL;\n',
                                                                                         '            }\n',
                                                                                         '        }\n',
                                                                                         '        pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;\n',
                                                                                         '    }\n',
                                                                                         '} else {\n',
                                                                                         '    // This is unexpected.\n',
                                                                                         '    glv_LogError("CreateGraphicsPipeline must have CreateInfo stype of XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO.\\n");\n',
                                                                                         '    pPacket->header = NULL;\n',
                                                                                         '}']},
                             'CreateComputePipeline' : {'param': 'pCreateInfo', 'txt': ['interpret_pipeline_shader(pHeader, (XGL_PIPELINE_SHADER*)(&pPacket->pCreateInfo->cs));']}}
        if_body = []
        if_body.append('typedef struct struct_xglApiVersion {')
        if_body.append('    glv_trace_packet_header* header;')
        if_body.append('    uint32_t version;')
        if_body.append('} struct_xglApiVersion;\n')
        if_body.append('static struct_xglApiVersion* interpret_body_as_xglApiVersion(glv_trace_packet_header* pHeader, BOOL check_version)')
        if_body.append('{')
        if_body.append('    struct_xglApiVersion* pPacket = (struct_xglApiVersion*)pHeader->pBody;')
        if_body.append('    pPacket->header = pHeader;')
        if_body.append('    if (check_version && pPacket->version != XGL_API_VERSION)')
        if_body.append('        glv_LogError("Trace file from older XGL version 0x%x, xgl replayer built from version 0x%x, replayer may fail\\n", pPacket->version, XGL_API_VERSION);')
        if_body.append('    return pPacket;')
        if_body.append('}\n')
        for proto in self.protos:
            if 'Wsi' not in proto.name and 'Dbg' not in proto.name:
                if 'UnmapMemory' == proto.name:
                    proto.params.append(xgl.Param("void*", "pData"))
                if_body.append('typedef struct struct_xgl%s {' % proto.name)
                if_body.append('    glv_trace_packet_header* header;')
                for p in proto.params:
                    if '[4]' in p.ty:
                        if_body.append('    %s %s[4];' % (p.ty.strip('[4]'), p.name))
                    else:
                        if_body.append('    %s %s;' % (p.ty, p.name))
                if 'void' != proto.ret:
                    if_body.append('    %s result;' % proto.ret)
                if_body.append('} struct_xgl%s;\n' % proto.name)
                if_body.append('static struct_xgl%s* interpret_body_as_xgl%s(glv_trace_packet_header* pHeader)' % (proto.name, proto.name))
                if_body.append('{')
                if_body.append('    struct_xgl%s* pPacket = (struct_xgl%s*)pHeader->pBody;' % (proto.name, proto.name))
                if_body.append('    pPacket->header = pHeader;')
                for p in proto.params:
                    if '*' in p.ty:
                        if 'DEVICE_CREATE_INFO' in p.ty:
                            if_body.append('    pPacket->%s = interpret_XGL_DEVICE_CREATE_INFO(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.name))
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
                if_body.append('typedef struct struct_xgl%s {' % proto.name)
                if_body.append('    glv_trace_packet_header* pHeader;')
                for p in proto.params:
                    if_body.append('    %s %s;' % (p.ty, p.name))
                if 'void' != proto.ret:
                    if_body.append('    %s result;' % proto.ret)
                if_body.append('} struct_xgl%s;\n' % proto.name)
                if_body.append('static struct_xgl%s* interpret_body_as_xgl%s(glv_trace_packet_header* pHeader)' % (proto.name, proto.name))
                if_body.append('{')
                if_body.append('    struct_xgl%s* pPacket = (struct_xgl%s*)pHeader->pBody;' % (proto.name, proto.name))
                if_body.append('    pPacket->pHeader = pHeader;')
                for p in proto.params:
                    if '*' in p.ty:
                        if_body.append('    pPacket->%s = (%s)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->%s);' % (p.name, p.ty, p.name))
                if_body.append('    return pPacket;')
                if_body.append('}\n')
        return "\n".join(if_body)

    def _generate_replay_func_ptrs(self):
        xf_body = []
        xf_body.append('struct xglFuncs {')
        xf_body.append('    void init_funcs(void * libHandle);')
        xf_body.append('    void *m_libHandle;\n')
        for proto in self.protos:
            xf_body.append('    typedef %s( XGLAPI * type_xgl%s)(' % (proto.ret, proto.name))
            for p in proto.params:
                if '[4]' in p.ty:
                    xf_body.append('        %s %s[4],' % (p.ty.strip('[4]'), p.name))
                else:
                    xf_body.append('        %s %s,' % (p.ty, p.name))
            xf_body[-1] = xf_body[-1].replace(',', ');')
            xf_body.append('    type_xgl%s real_xgl%s;' % (proto.name, proto.name))
        xf_body.append('};')
        return "\n".join(xf_body)

    def _map_decl(self, type1, type2, name):
        return '    std::map<%s, %s> %s;' % (type1, type2, name)

    def _add_to_map_decl(self, type1, type2, name):
        txt = '    void add_to_map(%s* pTraceVal, %s* pReplayVal)\n    {\n' % (type1, type2)
        txt += '        assert(pTraceVal != NULL);\n'
        txt += '        assert(pReplayVal != NULL);\n'
        txt += '        %s[*pTraceVal] = *pReplayVal;\n    }' % name
        return txt

    def _rm_from_map_decl(self, ty, name):
        txt = '    void rm_from_map(const %s& key)\n    {\n' % (ty)
        txt += '        %s.erase(key);\n    }' % name
        return txt

    def _remap_decl(self, ty, name):
        txt = '    %s remap(const %s& value)\n    {\n' % (ty, ty)
        txt += '        std::map<%s, %s>::const_iterator q = %s.find(value);\n' % (ty, ty, name)
        txt += '        return (q == %s.end()) ? XGL_NULL_HANDLE : q->second;\n    }' % name
        return txt

    def _generate_replay_objmapper_class(self):
        # Create dict mapping member var names to XGL type (i.e. 'm_imageViews' : 'XGL_IMAGE_VIEW')
        obj_map_dict = {}
        for ty in xgl.object_type_list:
            if ty in xgl.object_parent_list:
                continue
            mem_var = ty.replace('XGL_', '').lower()
            mem_var_list = mem_var.split('_')
            mem_var = 'm_%s%ss' % (mem_var_list[0], "".join([m.title() for m in mem_var_list[1:]]))
            obj_map_dict[mem_var] = ty
        rc_body = []
        rc_body.append('typedef struct _XGLAllocInfo {')
        rc_body.append('    XGL_GPU_SIZE size;')
        rc_body.append('    void *pData;')
        rc_body.append('} XGLAllocInfo;')
        rc_body.append('')
        rc_body.append('class xglReplayObjMapper {')
        rc_body.append('public:')
        rc_body.append('    xglReplayObjMapper() {}')
        rc_body.append('    ~xglReplayObjMapper() {}')
        rc_body.append('')
        rc_body.append(self._map_decl('XGL_GPU_MEMORY', 'XGLAllocInfo', 'm_mapData'))
        # Custom code for 1-off memory mapping functions
        rc_body.append('    void add_entry_to_mapData(XGL_GPU_MEMORY handle, XGL_GPU_SIZE size)')
        rc_body.append('    {')
        rc_body.append('        XGLAllocInfo info;')
        rc_body.append('        info.pData = NULL;')
        rc_body.append('        info.size = size;')
        rc_body.append('        m_mapData.insert(std::pair<XGL_GPU_MEMORY, XGLAllocInfo>(handle, info));')
        rc_body.append('    }')
        rc_body.append('')
        rc_body.append('    void add_mapping_to_mapData(XGL_GPU_MEMORY handle, void *pData)')
        rc_body.append('    {')
        rc_body.append('        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);')
        rc_body.append('        if (it == m_mapData.end())')
        rc_body.append('        {')
        rc_body.append('            glv_LogWarn("add_mapping_to_mapData() could not find entry\\n");')
        rc_body.append('            return;')
        rc_body.append('        }')
        rc_body.append('        XGLAllocInfo &info = it->second;')
        rc_body.append('        if (info.pData != NULL)')
        rc_body.append('            glv_LogWarn("add_mapping_to_mapData() data already mapped overwrite old mapping\\n");')
        rc_body.append('        else if (pData == NULL)')
        rc_body.append('            glv_LogWarn("add_mapping_to_mapData() adding NULL pointer\\n");')
        rc_body.append('        info.pData = pData;')
        rc_body.append('    }')
        rc_body.append('')
        rc_body.append('    void rm_entry_from_mapData(XGL_GPU_MEMORY handle)')
        rc_body.append('    {')
        rc_body.append('        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);')
        rc_body.append('        if (it == m_mapData.end())')
        rc_body.append('            return;')
        rc_body.append('        m_mapData.erase(it);')
        rc_body.append('    }')
        rc_body.append('')
        rc_body.append('    void rm_mapping_from_mapData(XGL_GPU_MEMORY handle, void* pData)')
        rc_body.append('    {')
        rc_body.append('        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);')
        rc_body.append('        if (it == m_mapData.end())')
        rc_body.append('            return;\n')
        rc_body.append('        XGLAllocInfo &info = it->second;')
        rc_body.append('        if (!pData || !info.pData)')
        rc_body.append('        {')
        rc_body.append('            if (!pData)')
        rc_body.append('                glv_LogWarn("rm_mapping_from_mapData() null src pointer\\n");')
        rc_body.append('            else')
        rc_body.append('                glv_LogWarn("rm_mapping_from_mapData() null dest pointer size=%u\\n", info.size);')
        rc_body.append('            info.pData = NULL;')
        rc_body.append('            return;')
        rc_body.append('        }')
        rc_body.append('        memcpy(info.pData, pData, info.size);')
        rc_body.append('        info.pData = NULL;')
        rc_body.append('    }')
        rc_body.append('')
        rc_body.append('    /*std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU> m_gpus;')
        rc_body.append('    void add_to_map(XGL_PHYSICAL_GPU* pTraceGpu, XGL_PHYSICAL_GPU* pReplayGpu)')
        rc_body.append('    {')
        rc_body.append('        assert(pTraceGpu != NULL);')
        rc_body.append('        assert(pReplayGpu != NULL);')
        rc_body.append('        m_gpus[*pTraceGpu] = *pReplayGpu;')
        rc_body.append('    }')
        rc_body.append('')
        rc_body.append('    XGL_PHYSICAL_GPU remap(const XGL_PHYSICAL_GPU& gpu)')
        rc_body.append('    {')
        rc_body.append('        std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU>::const_iterator q = m_gpus.find(gpu);')
        rc_body.append('        return (q == m_gpus.end()) ? XGL_NULL_HANDLE : q->second;')
        rc_body.append('    }*/')
        rc_body.append('')
        rc_body.append('    void clear_all_map_handles()\n    {')
        for var in sorted(obj_map_dict):
            rc_body.append('        %s.clear();' % var)
        rc_body.append('    }')
        for var in sorted(obj_map_dict):
            rc_body.append('')
            rc_body.append(self._map_decl(obj_map_dict[var], obj_map_dict[var], var))
            rc_body.append(self._add_to_map_decl(obj_map_dict[var], obj_map_dict[var], var))
            rc_body.append(self._rm_from_map_decl(obj_map_dict[var], var))
            rc_body.append(self._remap_decl(obj_map_dict[var], var))
        # XGL_DYNAMIC_STATE_OBJECT code
        state_obj_remap_types = xgl.object_dynamic_state_list
        rc_body.append('    XGL_DYNAMIC_STATE_OBJECT remap(const XGL_DYNAMIC_STATE_OBJECT& state)\n    {')
        rc_body.append('        XGL_DYNAMIC_STATE_OBJECT obj;')
        for t in state_obj_remap_types:
            rc_body.append('        if ((obj = remap(static_cast <%s> (state))) != XGL_NULL_HANDLE)' % t)
            rc_body.append('            return obj;')
        rc_body.append('        return XGL_NULL_HANDLE;\n    }')
        rc_body.append('    void rm_from_map(const XGL_DYNAMIC_STATE_OBJECT& state)\n    {')
        for t in state_obj_remap_types:
            rc_body.append('        rm_from_map(static_cast <%s> (state));' % t)
        rc_body.append('    }')
        rc_body.append('')
        # OBJECT code
        rc_body.append('    XGL_OBJECT remap(const XGL_OBJECT& object)\n    {')
        rc_body.append('        XGL_OBJECT obj;')
        obj_remap_types = xgl.object_list
        for var in obj_remap_types:
            rc_body.append('        if ((obj = remap(static_cast <%s> (object))) != XGL_NULL_HANDLE)' % (var))
            rc_body.append('            return obj;')
        rc_body.append('        return XGL_NULL_HANDLE;\n    }')
        rc_body.append('    void rm_from_map(const XGL_OBJECT & objKey)\n    {')
        for var in obj_remap_types:
            rc_body.append('        rm_from_map(static_cast <%s> (objKey));' % (var))
        rc_body.append('    }')
        rc_body.append('    XGL_BASE_OBJECT remap(const XGL_BASE_OBJECT& object)\n    {')
        rc_body.append('        XGL_BASE_OBJECT obj;')
        base_obj_remap_types = ['XGL_DEVICE', 'XGL_QUEUE', 'XGL_GPU_MEMORY', 'XGL_OBJECT']
        for t in base_obj_remap_types:
            rc_body.append('        if ((obj = remap(static_cast <%s> (object))) != XGL_NULL_HANDLE)' % t)
            rc_body.append('            return obj;')
        rc_body.append('        return XGL_NULL_HANDLE;')
        rc_body.append('    }\n')
        rc_body.append('};')
        return "\n".join(rc_body)


    def _generate_replay_init_funcs(self):
        rif_body = []
        rif_body.append('void xglFuncs::init_funcs(void * handle)\n{\n    m_libHandle = handle;')
        for proto in self.protos:
            rif_body.append('    real_xgl%s = (type_xgl%s)(glv_platform_get_library_entrypoint(handle, "xgl%s"));' % (proto.name, proto.name, proto.name))
        rif_body.append('}')
        return "\n".join(rif_body)

    def _get_packet_param(self, t, n):
        # list of types that require remapping
        remap_list = xgl.object_type_list
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
        ieg_body.append('            returnValue = manually_handle_xglEnumerateGpus(pPacket);')
        return "\n".join(ieg_body)

    def _gen_replay_get_gpu_info(self):
        ggi_body = []
        ggi_body.append('            returnValue = manually_handle_xglGetGpuInfo(pPacket);')
        return "\n".join(ggi_body)

    def _gen_replay_create_device(self):
        cd_body = []
        cd_body.append('            returnValue = manually_handle_xglCreateDevice(pPacket);')
        return "\n".join(cd_body)

    def _gen_replay_get_extension_support(self):
        ges_body = []
        ges_body.append('            returnValue = manually_handle_xglGetExtensionSupport(pPacket);')
        return "\n".join(ges_body)

    def _gen_replay_queue_submit(self):
        qs_body = []
        qs_body.append('            returnValue = manually_handle_xglQueueSubmit(pPacket);')
        return "\n".join(qs_body)

    def _gen_replay_get_object_info(self):
        goi_body = []
        goi_body.append('            returnValue = manually_handle_xglGetObjectInfo(pPacket);')
        return "\n".join(goi_body)

    def _gen_replay_get_format_info(self):
        gfi_body = []
        gfi_body.append('            returnValue = manually_handle_xglGetFormatInfo(pPacket);')
        return "\n".join(gfi_body)

    def _gen_replay_get_image_subresource_info(self):
        isi_body = []
        isi_body.append('            returnValue = manually_handle_xglGetImageSubresourceInfo(pPacket);')
        return "\n".join(isi_body)

    def _gen_replay_update_descriptors(self):
        ud_body = []
        ud_body.append('            returnValue = manually_handle_xglUpdateDescriptors(pPacket);')
        return "\n".join(ud_body)

    def _gen_replay_create_descriptor_set_layout(self):
        cdsl_body = []
        cdsl_body.append('            returnValue = manually_handle_xglCreateDescriptorSetLayout(pPacket);')
        return "\n".join(cdsl_body)

    def _gen_replay_create_graphics_pipeline(self):
        cgp_body = []
        cgp_body.append('            returnValue = manually_handle_xglCreateGraphicsPipeline(pPacket);')
        return "\n".join(cgp_body)

    def _gen_replay_cmd_wait_events(self):
        cwe_body = []
        cwe_body.append('            returnValue = manually_handle_xglCmdWaitEvents(pPacket);')
        return "\n".join(cwe_body)

    def _gen_replay_cmd_pipeline_barrier(self):
        cpb_body = []
        cpb_body.append('            returnValue = manually_handle_xglCmdPipelineBarrier(pPacket);')
        return "\n".join(cpb_body)

    def _gen_replay_create_framebuffer(self):
        cf_body = []
        cf_body.append('            returnValue = manually_handle_xglCreateFramebuffer(pPacket);')
        return "\n".join(cf_body)

    def _gen_replay_create_renderpass(self):
        cr_body = []
        cr_body.append('            returnValue = manually_handle_xglCreateRenderPass(pPacket);')
        return "\n".join(cr_body)

    def _gen_replay_begin_command_buffer(self):
        bcb_body = []
        bcb_body.append('            returnValue = manually_handle_xglBeginCommandBuffer(pPacket);')
        return "\n".join(bcb_body)

    def _gen_replay_store_pipeline(self):
        sp_body = []
        sp_body.append('            returnValue = manually_handle_xglStorePipeline(pPacket);')
        return "\n".join(sp_body)

    def _gen_replay_get_multi_gpu_compatibility(self):
        gmgc_body = []
        gmgc_body.append('            returnValue = manually_handle_xglGetMultiGpuCompatibility(pPacket);')
        return "\n".join(gmgc_body)

    def _gen_replay_destroy_object(self):
        do_body = []
        do_body.append('            returnValue = manually_handle_xglDestroyObject(pPacket);')
        return "\n".join(do_body)

    def _gen_replay_wait_for_fences(self):
        wf_body = []
        wf_body.append('            returnValue = manually_handle_xglWaitForFences(pPacket);')
        return "\n".join(wf_body)

    def _gen_replay_wsi_associate_connection(self):
        wac_body = []
        wac_body.append('            returnValue = manually_handle_xglWsiX11AssociateConnection(pPacket);')
        return "\n".join(wac_body)

    def _gen_replay_wsi_get_msc(self):
        wgm_body = []
        wgm_body.append('            returnValue = manually_handle_xglWsiX11GetMSC(pPacket);')
        return "\n".join(wgm_body)

    def _gen_replay_wsi_create_presentable_image(self):
        cpi_body = []
        cpi_body.append('            returnValue = manually_handle_xglWsiX11CreatePresentableImage(pPacket);')
        return "\n".join(cpi_body)

    def _gen_replay_wsi_queue_present(self):
        wqp_body = []
        wqp_body.append('            returnValue = manually_handle_xglWsiX11QueuePresent(pPacket);')
        return "\n".join(wqp_body)

    def _gen_replay_free_memory(self):
        fm_body = []
        fm_body.append('            returnValue = manually_handle_xglFreeMemory(pPacket);')
        return "\n".join(fm_body)

    def _gen_replay_map_memory(self):
        mm_body = []
        mm_body.append('            returnValue = manually_handle_xglMapMemory(pPacket);')
        return "\n".join(mm_body)
        
    def _gen_replay_unmap_memory(self):
        um_body = []
        um_body.append('            returnValue = manually_handle_xglUnmapMemory(pPacket);')
        return "\n".join(um_body)

    # I don't think this function is being generated anymore (ie, it may have been removed from XGL)
    def _gen_replay_bind_dynamic_memory_view(self):
        bdmv_body = []
        bdmv_body.append('            XGL_MEMORY_VIEW_ATTACH_INFO memView;')
        bdmv_body.append('            memcpy(&memView, pPacket->pMemView, sizeof(XGL_MEMORY_VIEW_ATTACH_INFO));')
        bdmv_body.append('            memView.mem = m_objMapper.remap(pPacket->pMemView->mem);')
        bdmv_body.append('            m_xglFuncs.real_xglCmdBindDynamicMemoryView(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, &memView);')
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
                            'FreeMemory': self._gen_replay_free_memory,
                            'MapMemory': self._gen_replay_map_memory,
                            'UnmapMemory': self._gen_replay_unmap_memory,
                            'CmdBindDynamicMemoryView': self._gen_replay_bind_dynamic_memory_view,
                            'UpdateDescriptors': self._gen_replay_update_descriptors,
                            'CreateDescriptorSetLayout': self._gen_replay_create_descriptor_set_layout,
                            'CmdWaitEvents': self._gen_replay_cmd_wait_events,
                            'CmdPipelineBarrier': self._gen_replay_cmd_pipeline_barrier}
        # TODO : Need to guard CreateInstance with "if (!m_display->m_initedXGL)" check
        # Despite returning a value, don't check these funcs b/c custom code includes check already
        custom_check_ret_val = ['EnumerateGpus', 'GetGpuInfo', 'CreateDevice', 'GetExtensionSupport', 'QueueSubmit', 'GetObjectInfo',
                                'GetFormatInfo', 'GetImageSubresourceInfo', 'CreateDescriptorSetLayout', 'CreateGraphicsPipeline',
                                'CreateFramebuffer', 'CreateRenderPass', 'BeginCommandBuffer', 'StorePipeline', 'GetMultiGpuCompatibility',
                                'DestroyObject', 'WaitForFences', 'FreeMemory', 'MapMemory', 'UnmapMemory',
                                'WsiX11AssociateConnection', 'WsiX11GetMSC', 'WsiX11CreatePresentableImage', 'WsiX11QueuePresent']
        # multi-gpu Open funcs w/ list of local params to create
        custom_open_params = {'OpenSharedMemory': (-1,),
                              'OpenSharedQueueSemaphore': (-1,),
                              'OpenPeerMemory': (-1,),
                              'OpenPeerImage': (-1, -2,)}
        # Functions that create views are unique from other create functions
        create_view_list = ['CreateBufferView', 'CreateImageView', 'CreateColorAttachmentView', 'CreateDepthStencilView', 'CreateComputePipeline']
        # Functions to treat as "Create' that don't have 'Create' in the name
        special_create_list = ['LoadPipeline', 'AllocMemory', 'GetDeviceQueue', 'PinSystemMemory', 'AllocDescriptorSets']
        # A couple funcs use do while loops
        do_while_dict = {'GetFenceStatus': 'replayResult != pPacket->result  && pPacket->result == XGL_SUCCESS', 'GetEventStatus': '(pPacket->result == XGL_EVENT_SET || pPacket->result == XGL_EVENT_RESET) && replayResult != pPacket->result'}
        rbody = []
        rbody.append('glv_replay::GLV_REPLAY_RESULT xglReplay::replay(glv_trace_packet_header *packet)')
        rbody.append('{')
        rbody.append('    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;')
        rbody.append('    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;')
        rbody.append('    switch (packet->packet_id)')
        rbody.append('    {')
        rbody.append('        case GLV_TPI_XGL_xglApiVersion:')
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
            rbody.append('        case GLV_TPI_XGL_xgl%s:' % proto.name)
            rbody.append('        {')
            rbody.append('            struct_xgl%s* pPacket = (struct_xgl%s*)(packet->pBody);' % (proto.name, proto.name))
            if proto.name in custom_body_dict:
                rbody.append(custom_body_dict[proto.name]())
            else:
                if proto.name in custom_open_params:
                    rbody.append('            XGL_DEVICE handle;')
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
                        rbody.append('            XGL_DESCRIPTOR_SET_LAYOUT localDescSets[100];')
                        rbody.append('            assert(pPacket->count <= 100);')
                        rbody.append('            for (uint32_t i = 0; i < pPacket->count; i++)')
                        rbody.append('            {')
                        rbody.append('                localDescSets[i] = m_objMapper.remap(pPacket->%s[i]);' % (proto.params[-3].name))
                        rbody.append('            }')
                elif proto.name == 'ClearDescriptorSets':
                    rbody.append('            XGL_DESCRIPTOR_SET localDescSets[100];')
                    rbody.append('            assert(pPacket->count <= 100);')
                    rbody.append('            for (uint32_t i = 0; i < pPacket->count; i++)')
                    rbody.append('            {')
                    rbody.append('                localDescSets[i] = m_objMapper.remap(pPacket->%s[i]);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name in do_while_dict:
                    rbody.append('            do {')
                elif proto.name == 'EnumerateLayers':
                    rbody.append('            char **bufptr = GLV_NEW_ARRAY(char *, pPacket->maxLayerCount);')
                    rbody.append('            char **ptrLayers = (pPacket->pOutLayers == NULL) ? bufptr : (char **) pPacket->pOutLayers;')
                    rbody.append('            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)')
                    rbody.append('                bufptr[i] = GLV_NEW_ARRAY(char, pPacket->maxStringSize);')
                elif proto.name == 'DestroyInstance':
                    rbody.append('            xglDbgUnregisterMsgCallback(g_fpDbgMsgCallback);')
                rr_string = '            '
                if ret_value:
                    rr_string = '            replayResult = '
                rr_string += 'm_xglFuncs.real_xgl%s(' % proto.name
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
                rbody.append(rr_string)
                if 'DestroyDevice' in proto.name:
                    rbody.append('            if (replayResult == XGL_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                m_pCBDump = NULL;')
                    rbody.append('                m_pDSDump = NULL;')
                    rbody.append('                m_pGlvSnapshotPrint = NULL;')
                    rbody.append('                m_objMapper.rm_from_map(pPacket->device);')
                    rbody.append('                m_display->m_initedXGL = false;')
                    rbody.append('            }')
                if 'DestroyInstance' in proto.name:
                    rbody.append('            if (replayResult == XGL_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                // TODO need to handle multiple instances and only clearing maps within an instance.')
                    rbody.append('                // TODO this only works with a single instance used at any given time.')
                    rbody.append('                m_objMapper.clear_all_map_handles();')
                    rbody.append('            }')
                elif 'AllocDescriptorSets' in proto.name:
                    rbody.append('            if (replayResult == XGL_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                for (uint32_t i = 0; i < local_pCount; i++) {')
                    rbody.append('                    m_objMapper.add_to_map(&pPacket->%s[i], &local_%s[i]);' % (proto.params[-2].name, proto.params[-2].name))
                    rbody.append('                }')
                    rbody.append('            }')
                elif create_func: # save handle mapping if create successful
                    rbody.append('            if (replayResult == XGL_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                m_objMapper.add_to_map(pPacket->%s, &local_%s);' % (proto.params[-1].name, proto.params[-1].name))
                    if 'AllocMemory' == proto.name:
                        rbody.append('                m_objMapper.add_entry_to_mapData(local_%s, pPacket->pAllocInfo->allocationSize);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name in do_while_dict:
                    rbody[-1] = '    %s' % rbody[-1]
                    rbody.append('            } while (%s);' % do_while_dict[proto.name])
                    rbody.append('            if (pPacket->result != XGL_NOT_READY || replayResult != XGL_SUCCESS)')
                elif proto.name == 'EnumerateLayers':
                    rbody.append('            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)')
                    rbody.append('                GLV_DELETE(bufptr[i]);')
            if ret_value:
                rbody.append('            CHECK_RETURN_VALUE(xgl%s);' % proto.name)
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
                self._generate_trace_func_protos()]

        return "\n".join(body)

class GlaveTraceC(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glv_platform.h"')
        header_txt.append('#include "glv_common.h"')
        header_txt.append('#include "glvtrace_xgl_xgl.h"')
        header_txt.append('#include "glvtrace_xgl_xgldbg.h"')
        header_txt.append('#include "glvtrace_xgl_xglwsix11ext.h"')
        header_txt.append('#include "glv_interconnect.h"')
        header_txt.append('#include "glv_filelike.h"')
        header_txt.append('#include "xgl_struct_size_helper.h"')
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
                self._generate_helper_funcs(),
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
        header_txt.append('#include "xgl_enum_string_helper.h"')
        header_txt.append('#if defined(WIN32)')
        header_txt.append('#define snprintf _snprintf')
        header_txt.append('#endif')
        header_txt.append('#define SEND_ENTRYPOINT_ID(entrypoint) ;')
        header_txt.append('//#define SEND_ENTRYPOINT_ID(entrypoint) glv_TraceInfo(#entrypoint "\\n");\n')
        header_txt.append('#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) ;')
        header_txt.append('//#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) glv_TraceInfo(entrypoint, __VA_ARGS__);\n')
        header_txt.append('#define CREATE_TRACE_PACKET(entrypoint, buffer_bytes_needed) \\')
        header_txt.append('    pHeader = glv_create_trace_packet(GLV_TID_XGL, GLV_TPI_XGL_##entrypoint, sizeof(struct_##entrypoint), buffer_bytes_needed);\n')
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
        header_txt.append('#include "xgl.h"')
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
        header_txt.append('#include "xgl.h"')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include "xglWsiX11Ext.h"\n')
        header_txt.append('#else')
        header_txt.append('#include "xglWsiWinExt.h"')
        header_txt.append('#endif')
        header_txt.append('void AttachHooks_xglwsix11ext();')
        header_txt.append('void DetachHooks_xglwsix11ext();')
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
        header_txt.append('#include "glvtrace_xgl_xglwsix11ext.h"')
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
        header_txt.append('#include "xglWsiX11Ext.h"')
        header_txt.append('#else')
        header_txt.append('#include "xglWsiWinExt.h"')
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
        header_txt.append('#include "xgl.h"')
        header_txt.append('#include "xglDbg.h"\n')
        header_txt.append('void AttachHooks_xgldbg();')
        header_txt.append('void DetachHooks_xgldbg();')
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
        header_txt.append('#include "glvtrace_xgl_xgl.h"')
        header_txt.append('#include "glvtrace_xgl_xgldbg.h"')
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
        header_txt.append('#include "xglDbg.h"')
        header_txt.append('#include "glv_trace_packet_utils.h"\n')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_interp_funcs_ext('Dbg')]

        return "\n".join(body)

class GlaveReplayXglFuncPtrs(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include <xcb/xcb.h>\n')
        header_txt.append('#endif')
        header_txt.append('#include "xgl.h"')
        header_txt.append('#include "xglDbg.h"')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include "xglWsiX11Ext.h"')
        header_txt.append('#else')
        header_txt.append('#include "xglWsiWinExt.h"')
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
        header_txt.append('#include "xgl.h"')
        header_txt.append('#include "xglDbg.h"')
        header_txt.append('#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)')
        header_txt.append('#include "xglWsiX11Ext.h"')
        header_txt.append('#else')
        header_txt.append('#include "xglWsiWinExt.h"')
        header_txt.append('#endif')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_objmapper_class()]

        return "\n".join(body)

class GlaveReplayC(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glvreplay_xgl_xglreplay.h"\n')
        header_txt.append('#include "glvreplay_xgl.h"\n')
        header_txt.append('#include "glvreplay_main.h"\n')
        header_txt.append('#include <algorithm>')
        header_txt.append('#include <queue>')
        header_txt.append('\n')
        header_txt.append('extern "C" {')
        header_txt.append('#include "glv_vk_vk_structs.h"')
        header_txt.append('#include "glv_vk_vkdbg_structs.h"')
        header_txt.append('#include "glv_vk_vkwsix11ext_structs.h"')
        header_txt.append('#include "glv_vk_packet_id.h"')
        header_txt.append('#include "xgl_enum_string_helper.h"\n}\n')
        header_txt.append('#define APP_NAME "glvreplay_xgl"')
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
            "glave-replay-xgl-funcs" : GlaveReplayXglFuncPtrs,
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
