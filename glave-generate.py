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

import sys

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
                func_protos.append('%s;' % proto.c_func(prefix="__HOOKED_xgl", attr="XGLAPI"))

        return "\n".join(func_protos)

    def _generate_trace_func_protos_ext(self, func_class='Wsi'):
        func_protos = []
        func_protos.append('// Hooked function prototypes\n')
        for proto in self.protos:
            if func_class in proto.name:
                func_protos.append('%s;' % proto.c_func(prefix="__HOOKED_xgl", attr="XGLAPI"))

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

        init_tracer.append('void InitTracer()\n{')
        init_tracer.append('char *ipAddr = glv_get_global_var("GLVLIB_TRACE_IPADDR");')
        init_tracer.append('if (ipAddr == NULL)')
        init_tracer.append('    ipAddr = "127.0.0.1";')
        init_tracer.append('    gMessageStream = glv_MessageStream_create(FALSE, ipAddr, GLV_BASE_PORT + GLV_TID_XGL);')
        init_tracer.append('    glv_trace_set_trace_file(glv_FileLike_create_msg(gMessageStream));')
        init_tracer.append('//    glv_tracelog_set_log_file(glv_FileLike_create_file(fopen("glv_log_traceside.txt","w")));')
        init_tracer.append('    glv_tracelog_set_tracer_id(GLV_TID_XGL);')
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
        um_body.append('    entry = find_mem_info_entry(mem);')
        um_body.append('    CREATE_TRACE_PACKET(xglUnmapMemory, (entry) ? entry->size : 0);')
        um_body.append('    pPacket = interpret_body_as_xglUnmapMemory(pHeader);')
        um_body.append('    if (entry)')
        um_body.append('    {')
        um_body.append('        assert(entry->handle == mem);')
        um_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), entry->size, entry->pData);')
        um_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));')
        um_body.append('        entry->pData = NULL;')
        um_body.append('    }')
        um_body.append('    result = real_xglUnmapMemory(mem);')
        um_body.append('    pPacket->mem = mem;')
        um_body.append('    pPacket->result = result;')
        um_body.append('    FINISH_TRACE_PACKET();')
        um_body.append('    return result;')
        um_body.append('}\n')
        return "\n".join(um_body)

    # Generate functions used to trace API calls and store the input and result data into a packet
    def _generate_trace_funcs(self):
        func_body = []
        for proto in self.protos:
            if 'UnmapMemory' == proto.name:
                func_body.append(self._gen_unmap_memory())
            elif 'Dbg' not in proto.name and 'Wsi' not in proto.name:
                packet_update_txt = ''
                return_txt = ''
                packet_size = ''
                in_data_size = False # flag when we need to capture local input size variable for in/out size
                buff_ptr_indices = []
                func_body.append('GLVTRACER_EXPORT %s XGLAPI __HOOKED_xgl%s(' % (proto.ret, proto.name))
                for p in proto.params: # TODO : For all of the ptr types, check them for NULL and return 0 is NULL
                    if 'color' == p.name:
                        func_body.append('    %s %s[4],' % (p.ty.replace('[4]', ''), p.name))
                    else:
                        func_body.append('    %s %s,' % (p.ty, p.name))
                    if '*' in p.ty and 'pSysMem' != p.name and 'pReserved' != p.name:
                        if 'pData' == p.name:
                            if 'dataSize' == proto.params[proto.params.index(p)-1].name:
                                packet_size += 'dataSize + '
                            elif 'counterCount' == proto.params[proto.params.index(p)-1].name:
                                packet_size += 'sizeof(%s) + ' % p.ty.strip('*').replace('const ', '')
                            else:
                                packet_size += '((pDataSize != NULL && pData != NULL) ? *pDataSize : 0) + '
                        elif '**' in p.ty and 'void' in p.ty:
                            packet_size += 'sizeof(void*) + '
                        elif 'void' in p.ty:
                            packet_size += 'sizeof(%s) + ' % p.name
                        elif 'char' in p.ty:
                            packet_size += '((%s != NULL) ? strlen(%s) + 1 : 0) + ' % (p.name, p.name)
                        elif 'DEVICE_CREATE_INFO' in p.ty:
                            packet_size += 'calc_size_XGL_DEVICE_CREATE_INFO(pCreateInfo) + '
                        elif 'pDataSize' in p.name:
                            packet_size += '((pDataSize != NULL) ? sizeof(size_t) : 0) + '
                            in_data_size = True;
                        elif 'IMAGE_SUBRESOURCE' in p.ty and 'pSubresource' == p.name:
                            packet_size += '((pSubresource != NULL) ? sizeof(XGL_IMAGE_SUBRESOURCE) : 0) + '
                        else:
                            packet_size += 'sizeof(%s) + ' % p.ty.strip('*').replace('const ', '')
                        buff_ptr_indices.append(proto.params.index(p))
                    else:
                        if 'color' == p.name:
                            packet_update_txt += '    memcpy((void*)pPacket->color, color, 4 * sizeof(uint32_t));\n'
                        else:
                            packet_update_txt += '    pPacket->%s = %s;\n' % (p.name, p.name)
                    if 'Count' in p.name and proto.params[-1].name != p.name and p.name not in ['queryCount', 'vertexCount', 'indexCount', 'startCounter'] and proto.name not in ['CmdLoadAtomicCounters', 'CmdSaveAtomicCounters']:
                        if '*' in p.ty:
                            packet_size += '*%s*' % p.name
                        else:
                            packet_size += '%s*' % p.name
                if '' == packet_size:
                    packet_size = '0'
                else:
                    packet_size = packet_size.strip(' + ')
                func_body[-1] = func_body[-1].replace(',', ')')
                func_body.append('{\n    glv_trace_packet_header* pHeader;')
                if 'void' not in proto.ret or '*' in proto.ret:
                    func_body.append('    %s result;' % proto.ret)
                    return_txt = 'result = '
                if in_data_size:
                    func_body.append('    size_t _dataSize;')
                func_body.append('    struct_xgl%s* pPacket = NULL;' % proto.name)
                # functions that have non-standard sequence of  packet creation and calling real function
                # NOTE: Anytime we call the function first, need to add custom code for correctly tracking API call time
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
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, sizeof(XGL_INSTANCE) + calc_size_XGL_APPLICATION_INFO(pAppInfo) + ((pAllocCb == NULL) ? 0 :sizeof(XGL_ALLOC_CALLBACKS)));' % (proto.name))
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
                elif proto.name in ['CreateShader', 'CreateFramebuffer', 'CreateRenderPass', 'BeginCommandBuffer', 'CreateDynamicViewportState', 
                                    'AllocMemory', 'CreateGraphicsPipeline', 'CreateComputePipeline', 'UpdateDescriptors', 'CreateDescriptorSetLayout',
                                    'CreateDescriptorRegion', 'CmdWaitEvents', 'CmdPipelineBarrier']:
                    # these are regular case as far as sequence of tracing but custom sizes
                    func_body.append('    size_t customSize;')
                    if 'CreateShader' == proto.name:
                        func_body.append('    customSize = (pCreateInfo != NULL) ? pCreateInfo->codeSize : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateShader, sizeof(XGL_SHADER_CREATE_INFO) + sizeof(XGL_SHADER) + customSize);')
                    elif 'CreateGraphicsPipeline' == proto.name:
                        func_body.append('    customSize = calculate_pipeline_state_size(pCreateInfo->pNext);')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateGraphicsPipeline, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO) + sizeof(XGL_PIPELINE) + customSize);')
                    elif 'UpdateDescriptors' == proto.name:
                        func_body.append('    customSize = calculate_update_descriptors_size(pUpdateChain);')
                        func_body.append('    CREATE_TRACE_PACKET(xglUpdateDescriptors, sizeof(XGL_DESCRIPTOR_SET) + customSize);')
                    elif 'CreateDescriptorSetLayout' == proto.name:
                        func_body.append('    customSize = calculate_create_ds_layout_size(pSetLayoutInfoList);')
                        # NOTE : Just allocating enough packet size to account for largest array of uints
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateDescriptorSetLayout, XGL_SHADER_STAGE_COMPUTE * sizeof(uint32_t) + sizeof(XGL_DESCRIPTOR_SET_LAYOUT) + customSize);')
                    elif 'CreateFramebuffer' == proto.name:
                        func_body.append('    int dsSize = (pCreateInfo != NULL && pCreateInfo->pDepthStencilAttachment != NULL) ? sizeof(XGL_DEPTH_STENCIL_BIND_INFO) : 0;')
                        func_body.append('    uint32_t colorCount = (pCreateInfo != NULL && pCreateInfo->pColorAttachments != NULL) ? pCreateInfo->colorAttachmentCount : 0;')
                        func_body.append('    customSize = colorCount * sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO) + dsSize;')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateFramebuffer, sizeof(XGL_FRAMEBUFFER_CREATE_INFO) + sizeof(XGL_FRAMEBUFFER) + customSize);')
                    elif 'CreateRenderPass' == proto.name:
                        func_body.append('    uint32_t colorCount = (pCreateInfo != NULL && pCreateInfo->pColorLoadOps != NULL && pCreateInfo->pColorStoreOps != NULL && pCreateInfo->pColorLoadClearValues != NULL) ? pCreateInfo->colorAttachmentCount : 0;')
                        func_body.append('    customSize = colorCount * (sizeof(XGL_ATTACHMENT_LOAD_OP) + sizeof(XGL_ATTACHMENT_STORE_OP) + sizeof(XGL_CLEAR_COLOR));')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateRenderPass, sizeof(XGL_RENDER_PASS_CREATE_INFO) + sizeof(XGL_RENDER_PASS) + customSize);')
                    elif 'BeginCommandBuffer' == proto.name:
                        func_body.append('    customSize = calculate_begin_cmdbuf_size(pBeginInfo->pNext);')
                        func_body.append('    CREATE_TRACE_PACKET(xglBeginCommandBuffer, sizeof(XGL_CMD_BUFFER_BEGIN_INFO) + customSize);')
                    elif 'CreateDynamicViewportState' == proto.name:
                        func_body.append('    uint32_t vpsCount = (pCreateInfo != NULL && pCreateInfo->pViewports != NULL) ? pCreateInfo->viewportAndScissorCount : 0;')
                        func_body.append('    customSize = vpsCount * sizeof(XGL_VIEWPORT) + vpsCount * sizeof(XGL_RECT);')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateDynamicViewportState,  sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO) + sizeof(XGL_DYNAMIC_VP_STATE_OBJECT) + customSize);')
                    elif 'AllocMemory' == proto.name:
                        func_body.append('    customSize = calculate_alloc_memory_size(pAllocInfo->pNext);')
                        func_body.append('    CREATE_TRACE_PACKET(xglAllocMemory,  sizeof(XGL_MEMORY_ALLOC_INFO) + sizeof(XGL_GPU_MEMORY) + customSize);')
                    elif 'CreateDescriptorRegion' == proto.name:
                        func_body.append('    uint32_t rgCount = (pCreateInfo != NULL && pCreateInfo->pTypeCount != NULL) ? pCreateInfo->count : 0;')
                        func_body.append('    customSize = rgCount * sizeof(XGL_DESCRIPTOR_TYPE_COUNT);')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateDescriptorRegion,  sizeof(XGL_DESCRIPTOR_REGION_CREATE_INFO) + sizeof(XGL_DESCRIPTOR_REGION) + customSize);')
                    elif proto.name in ['CmdWaitEvents', 'CmdPipelineBarrier']:
                        event_array_type = 'XGL_EVENT'
                        if 'CmdPipelineBarrier' == proto.name:
                            event_array_type = 'XGL_SET_EVENT'
                        func_body.append('    uint32_t eventCount = (%s != NULL && %s->pEvents != NULL) ? %s->eventCount : 0;' % (proto.params[-1].name, proto.params[-1].name, proto.params[-1].name))
                        func_body.append('    uint32_t mbCount = (%s != NULL && %s->ppMemBarriers != NULL) ? %s->memBarrierCount : 0;' % (proto.params[-1].name, proto.params[-1].name, proto.params[-1].name))
                        func_body.append('    customSize = (eventCount * sizeof(%s)) + mbCount * sizeof(void*) + calculate_memory_barrier_size(mbCount, %s->ppMemBarriers);' % (event_array_type, proto.params[-1].name))
                        func_body.append('    CREATE_TRACE_PACKET(xgl%s, sizeof(%s) + customSize);' % (proto.name, proto.params[-1].ty.strip('*').replace('const ', '')))
                    else: #'CreateComputePipeline'
                        func_body.append('    customSize = calculate_pipeline_state_size(pCreateInfo->pNext);')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateComputePipeline, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO) + sizeof(XGL_PIPELINE) + customSize + calculate_pipeline_shader_size(&pCreateInfo->cs));')
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                else:
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, %s);' % (proto.name, packet_size))
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                if in_data_size:
                    func_body.append('    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;')
                func_body.append('    pPacket = interpret_body_as_xgl%s(pHeader);' % proto.name)
                func_body.append(packet_update_txt.strip('\n'))
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
                    # TODO : Clean this up.  Too much custom code and branching.
                    for idx in buff_ptr_indices:
                        if 'DEVICE_CREATE_INFO' in proto.params[idx].ty:
                            func_body.append('    add_XGL_DEVICE_CREATE_INFO_to_packet(pHeader, (XGL_DEVICE_CREATE_INFO**) &(pPacket->pCreateInfo), pCreateInfo);')
                        elif 'APPLICATION_INFO' in proto.params[idx].ty:
                            func_body.append('    add_XGL_APPLICATION_INFO_to_packet(pHeader, (XGL_APPLICATION_INFO**)&(pPacket->pAppInfo), pAppInfo);')
                        elif 'char' in proto.params[idx].ty:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), ((%s != NULL) ? strlen(%s) + 1 : 0), %s);' % (proto.params[idx].name, proto.params[idx].name, proto.params[idx].name, proto.params[idx].name))
                        elif 'Count' in proto.params[idx-1].name and 'queryCount' != proto.params[idx-1].name:
                            deref = ''
                            if '*' in proto.params[idx-1].ty:
                                deref = '*'
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s%s*sizeof(%s), %s);' % (proto.params[idx].name, deref, proto.params[idx-1].name, proto.params[idx].ty.strip('*').replace('const ', ''), proto.params[idx].name))
                        elif 'dataSize' == proto.params[idx].name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), dataSize, %s);' % (proto.params[idx].name, proto.params[idx].name))
                        elif 'pDataSize' == proto.params[idx].name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(size_t), &_dataSize);' % (proto.params[idx].name))
                        elif 'pDescriptorSets' == proto.params[idx].name and 'AllocDescriptorSets' == proto.name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), customSize, pDescriptorSets);')
                        elif 'pData' == proto.params[idx].name:
                            if 'dataSize' == proto.params[idx-1].name:
                                func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), dataSize, %s);' % (proto.params[idx].name, proto.params[idx].name))
                            elif in_data_size:
                                func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), _dataSize, %s);' % (proto.params[idx].name, proto.params[idx].name))
                            else:
                                func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), (pDataSize != NULL && pData != NULL) ? *pDataSize : 0, %s);' % (proto.params[idx].name, proto.params[idx].name))
                        elif 'pUpdateChain' == proto.params[idx].name:
                            pass
                        else:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(%s), %s);' % (proto.params[idx].name, proto.params[idx].ty.strip('*').replace('const ', ''), proto.params[idx].name))
                    # Some custom add_* and finalize_* function calls for Create* API calls
                    if proto.name in ['CreateShader', 'CreateFramebuffer', 'CreateRenderPass', 'BeginCommandBuffer', 'CreateDynamicViewportState',
                                      'AllocMemory', 'UpdateDescriptors', 'CreateDescriptorSetLayout', 'CreateGraphicsPipeline', 'CreateComputePipeline',
                                      'CreateDescriptorRegion', 'CmdWaitEvents', 'CmdPipelineBarrier']:
                        if 'CreateShader' == proto.name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), customSize, pCreateInfo->pCode);')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));')
                        elif 'CreateFramebuffer' == proto.name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments), colorCount * sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO), pCreateInfo->pColorAttachments);')
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment), dsSize, pCreateInfo->pDepthStencilAttachment);')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments));')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment));')
                        elif 'CreateRenderPass' == proto.name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps), colorCount * sizeof(XGL_ATTACHMENT_LOAD_OP), pCreateInfo->pColorLoadOps);')
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps), colorCount * sizeof(XGL_ATTACHMENT_STORE_OP), pCreateInfo->pColorStoreOps);')
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues), colorCount * sizeof(XGL_CLEAR_COLOR), pCreateInfo->pColorLoadClearValues);')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps));')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps));')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues));')
                        elif 'BeginCommandBuffer' == proto.name:
                            func_body.append('    add_begin_cmdbuf_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo->pNext), pBeginInfo->pNext);')
                        elif 'CreateDynamicViewportState' == proto.name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(XGL_VIEWPORT), pCreateInfo->pViewports);')
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(XGL_RECT), pCreateInfo->pScissors);')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));')
                        elif 'AllocMemory' == proto.name:
                            func_body.append('    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext);')
                        elif 'CreateGraphicsPipeline' == proto.name:
                            func_body.append('    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);')
                        elif 'UpdateDescriptors' == proto.name:
                            func_body.append('    add_update_descriptors_to_trace_packet(pHeader, (void**)&(pPacket->pUpdateChain), pUpdateChain);')
                        elif 'CreateDescriptorSetLayout' == proto.name:
                            func_body.append('    if (pSetLayoutInfoList)')
                            func_body.append('        add_create_ds_layout_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayoutInfoList->pNext), pSetLayoutInfoList->pNext);')
                        elif 'CreateDescriptorRegion' == proto.name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), rgCount * sizeof(XGL_DESCRIPTOR_TYPE_COUNT), pCreateInfo->pTypeCount);')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));')
                        elif proto.name in ['CmdWaitEvents', 'CmdPipelineBarrier']:
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
                        else:  #'CreateComputePipeline'
                            func_body.append('    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);')
                            func_body.append('    add_pipeline_shader_to_trace_packet(pHeader, (XGL_PIPELINE_SHADER*)&pPacket->pCreateInfo->cs, &pCreateInfo->cs);')
                            func_body.append('    finalize_pipeline_shader_address(pHeader, &pPacket->pCreateInfo->cs);')
                    if 'void' not in proto.ret or '*' in proto.ret:
                        func_body.append('    pPacket->result = result;')
                    for idx in buff_ptr_indices:
                        if ('DEVICE_CREATE_INFO' not in proto.params[idx].ty) and ('APPLICATION_INFO' not in proto.params[idx].ty) and ('pUpdateChain' != proto.params[idx].name):
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s));' % (proto.params[idx].name))
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
                func_body.append('    CREATE_TRACE_PACKET(xgl%s, %s);' % (proto.name, packet_size))
                func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                func_body.append('    pPacket = interpret_body_as_xgl%s(pHeader);' % proto.name)
                func_body.append(packet_update_txt.strip('\n'))
                for idx in buff_ptr_indices:
                    if 'char' in proto.params[idx].ty:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), ((%s != NULL) ? strlen(%s) + 1 : 0), %s);' % (proto.params[idx].name, proto.params[idx].name, proto.params[idx].name, proto.params[idx].name))
                    elif 'Size' in proto.params[idx-1].name:
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s, %s);' % (proto.params[idx].name, proto.params[idx-1].name, proto.params[idx].name))
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
        hf_body.append('static XGLMemInfo mInfo = {0, NULL, NULL, 0};')
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
        hf_body.append('static void init_mem_info()')
        hf_body.append('{')
        hf_body.append('    mInfo.numEntrys = 0;')
        hf_body.append('    mInfo.capacity = 1024;')
        hf_body.append('    mInfo.pLastMapped = NULL;')
        hf_body.append('')
        hf_body.append('    mInfo.pEntrys = GLV_NEW_ARRAY(XGLAllocInfo, mInfo.capacity);')
        hf_body.append('')
        hf_body.append('    if (mInfo.pEntrys == NULL)')
        hf_body.append('        glv_LogError("init_mem_info()  malloc failed\\n");')
        hf_body.append('    else')
        hf_body.append('        init_mem_info_entrys(mInfo.pEntrys, mInfo.capacity);')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void delete_mem_info()')
        hf_body.append('{')
        hf_body.append('    GLV_DELETE(mInfo.pEntrys);')
        hf_body.append('    mInfo.pEntrys = NULL;')
        hf_body.append('    mInfo.numEntrys = 0;')
        hf_body.append('    mInfo.capacity = 0;')
        hf_body.append('    mInfo.pLastMapped = NULL;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static XGLAllocInfo * get_mem_info_entry()')
        hf_body.append('{')
        hf_body.append('    unsigned int i;')
        hf_body.append('    XGLAllocInfo *entry;')
        hf_body.append('    if (mInfo.numEntrys > mInfo.capacity)')
        hf_body.append('    {')
        hf_body.append('        glv_LogError("get_mem_info_entry() bad internal state numEntrys\\n");')
        hf_body.append('        return NULL;')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    if (mInfo.numEntrys == mInfo.capacity)')
        hf_body.append('    {  // grow the array 2x')
        hf_body.append('        mInfo.capacity *= 2;')
        hf_body.append('        mInfo.pEntrys = (XGLAllocInfo *) GLV_REALLOC(mInfo.pEntrys, mInfo.capacity * sizeof(XGLAllocInfo));')
        hf_body.append('        //init the newly added entrys')
        hf_body.append('        init_mem_info_entrys(mInfo.pEntrys + mInfo.capacity / 2, mInfo.capacity / 2);')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    assert(mInfo.numEntrys < mInfo.capacity);')
        hf_body.append('    entry = mInfo.pEntrys;')
        hf_body.append('    for (i = 0; i < mInfo.capacity; i++)')
        hf_body.append('    {')
        hf_body.append('        if ((entry + i)->valid == FALSE)')
        hf_body.append('            return entry + i;')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    glv_LogError("get_mem_info_entry() did not find an entry\\n");')
        hf_body.append('    return NULL;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static XGLAllocInfo * find_mem_info_entry(const XGL_GPU_MEMORY handle)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry = mInfo.pEntrys;')
        hf_body.append('    unsigned int i;')
        hf_body.append('    if (mInfo.pLastMapped && mInfo.pLastMapped->handle == handle && mInfo.pLastMapped->valid)')
        hf_body.append('        return mInfo.pLastMapped;')
        hf_body.append('    for (i = 0; i < mInfo.numEntrys; i++)')
        hf_body.append('    {')
        hf_body.append('        if ((entry + i)->valid && (handle == (entry + i)->handle))')
        hf_body.append('            return entry + i;')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    return NULL;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_new_handle_to_mem_info(const XGL_GPU_MEMORY handle, XGL_GPU_SIZE size, void *pData)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry;')
        hf_body.append('')
        hf_body.append('    if (mInfo.capacity == 0)')
        hf_body.append('        init_mem_info();')
        hf_body.append('')
        hf_body.append('    entry = get_mem_info_entry();')
        hf_body.append('    if (entry)')
        hf_body.append('    {')
        hf_body.append('        entry->valid = TRUE;')
        hf_body.append('        entry->handle = handle;')
        hf_body.append('        entry->size = size;')
        hf_body.append('        entry->pData = pData;   // NOTE: xglFreeMemory will free this mem, so no malloc()')
        hf_body.append('        mInfo.numEntrys++;')
        hf_body.append('    }')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void add_data_to_mem_info(const XGL_GPU_MEMORY handle, void *pData)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry = find_mem_info_entry(handle);')
        hf_body.append('')
        hf_body.append('    if (entry)')
        hf_body.append('    {')
        hf_body.append('        entry->pData = pData;')
        hf_body.append('    }')
        hf_body.append('    mInfo.pLastMapped = entry;')
        hf_body.append('}')
        hf_body.append('')
        hf_body.append('static void rm_handle_from_mem_info(const XGL_GPU_MEMORY handle)')
        hf_body.append('{')
        hf_body.append('    XGLAllocInfo *entry = find_mem_info_entry(handle);')
        hf_body.append('')
        hf_body.append('    if (entry)')
        hf_body.append('    {')
        hf_body.append('        entry->valid = FALSE;')
        hf_body.append('        entry->pData = NULL;')
        hf_body.append('        entry->size = 0;')
        hf_body.append('        entry->handle = NULL;')
        hf_body.append('')
        hf_body.append('        mInfo.numEntrys--;')
        hf_body.append('        if (entry == mInfo.pLastMapped)')
        hf_body.append('            mInfo.pLastMapped = NULL;')
        hf_body.append('        if (mInfo.numEntrys == 0)')
        hf_body.append('            delete_mem_info();')
        hf_body.append('    }')
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
        hf_body.append('static size_t calculate_begin_cmdbuf_size(const XGL_CMD_BUFFER_BEGIN_INFO* pNext)')
        hf_body.append('{')
        hf_body.append('    size_t siz = 0;')
        hf_body.append('    while (pNext != NULL)')
        hf_body.append('    {')
        hf_body.append('        switch (pNext->sType)')
        hf_body.append('        {')
        hf_body.append('        case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:')
        hf_body.append('        {')
        hf_body.append('            siz += sizeof(XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO);')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        default:')
        hf_body.append('            glv_LogError("calculate_begin_cmdbuf_size() bad internal state sType\\n");')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        pNext = (XGL_CMD_BUFFER_BEGIN_INFO*)pNext->pNext;')
        hf_body.append('    }')
        hf_body.append('    return siz;')
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
        hf_body.append('static size_t calculate_alloc_memory_size(const XGL_MEMORY_ALLOC_INFO* pNext)')
        hf_body.append('{')
        hf_body.append('    size_t siz = 0;')
        hf_body.append('    while (pNext != NULL)')
        hf_body.append('    {')
        hf_body.append('        switch (pNext->sType)')
        hf_body.append('        {')
        hf_body.append('        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:')
        hf_body.append('        {')
        hf_body.append('            siz += sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO);')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:')
        hf_body.append('        {')
        hf_body.append('            siz += sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO);')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        default:')
        hf_body.append('            glv_LogError("calculate_alloc_memory_size() bad internal state sType\\n");')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        pNext = (XGL_MEMORY_ALLOC_INFO*)pNext->pNext;')
        hf_body.append('    }')
        hf_body.append('    return siz;')
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
        hf_body.append('static size_t calculate_pipeline_shader_size(const XGL_PIPELINE_SHADER* shader)')
        hf_body.append('{')
        hf_body.append('    size_t size = 0;')
        hf_body.append('    size += sizeof(XGL_PIPELINE_SHADER);')
        hf_body.append('    // constant buffers')
        hf_body.append('    if (shader->linkConstBufferCount > 0 && shader->pLinkConstBufferInfo != NULL)')
        hf_body.append('    {')
        hf_body.append('        uint32_t i;')
        hf_body.append('        for (i = 0; i < shader->linkConstBufferCount; i++)')
        hf_body.append('        {')
        hf_body.append('            size += sizeof(XGL_LINK_CONST_BUFFER);')
        hf_body.append('            size += shader->pLinkConstBufferInfo[i].bufferSize;')
        hf_body.append('        }')
        hf_body.append('    }')
        hf_body.append('    return size;')
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
        hf_body.append('static size_t calculate_create_ds_layout_size(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo)')
        hf_body.append('{')
        hf_body.append('    size_t update_size = 0;')
        hf_body.append('    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pTrav = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pCreateInfo;')
        hf_body.append('    while (pTrav)')
        hf_body.append('    {')
        hf_body.append('        update_size += sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);')
        hf_body.append('        pTrav = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pTrav->pNext;')
        hf_body.append('    }')
        hf_body.append('    return update_size;')
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
        hf_body.append('static size_t calculate_update_descriptors_size(const void* pUpdateChain)')
        hf_body.append('{')
        hf_body.append('    const XGL_UPDATE_SAMPLERS* pNext = pUpdateChain;')
        hf_body.append('    size_t totalUpdateSize = 0;')
        hf_body.append('    while (pNext)')
        hf_body.append('    {')
        hf_body.append('        switch (pNext->sType)')
        hf_body.append('        {')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:')
        hf_body.append('        {')
        hf_body.append('            totalUpdateSize += sizeof(XGL_UPDATE_SAMPLERS) + ((XGL_UPDATE_SAMPLERS*)pNext)->count * sizeof(XGL_SAMPLER);')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:')
        hf_body.append('        {')
        hf_body.append('            totalUpdateSize += sizeof(XGL_UPDATE_SAMPLER_TEXTURES) + ((XGL_UPDATE_SAMPLER_TEXTURES*)pNext)->count * (sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO) + sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:')
        hf_body.append('        {')
        hf_body.append('            totalUpdateSize += sizeof(XGL_UPDATE_IMAGES) + (((XGL_UPDATE_IMAGES*)pNext)->count * (sizeof(XGL_IMAGE_VIEW_ATTACH_INFO) + sizeof(XGL_IMAGE_VIEW_ATTACH_INFO*)));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:')
        hf_body.append('        {')
        hf_body.append('            totalUpdateSize += sizeof(XGL_UPDATE_BUFFERS) + (((XGL_UPDATE_BUFFERS*)pNext)->count * (sizeof(XGL_BUFFER_VIEW_ATTACH_INFO) + sizeof(XGL_BUFFER_VIEW_ATTACH_INFO*)));')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:')
        hf_body.append('        {')
        hf_body.append('            totalUpdateSize += sizeof(XGL_UPDATE_AS_COPY);')
        hf_body.append('            break;')
        hf_body.append('        }')
        hf_body.append('            default:')
        hf_body.append('                assert(0);')
        hf_body.append('        }')
        hf_body.append('        pNext = (XGL_UPDATE_SAMPLERS*)pNext->pNext;')
        hf_body.append('    }')
        hf_body.append('    return totalUpdateSize;')
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
        hf_body.append('static size_t calculate_pipeline_state_size(const void* pState)')
        hf_body.append('{')
        hf_body.append('    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pNext = pState;')
        hf_body.append('    size_t totalStateSize = 0;')
        hf_body.append('    while (pNext)')
        hf_body.append('    {')
        hf_body.append('        switch (pNext->sType)')
        hf_body.append('        {')
        pipe_state_list = ['IA_STATE_CREATE_INFO',
                           'TESS_STATE_CREATE_INFO',
                           'RS_STATE_CREATE_INFO',
                           'DS_STATE_CREATE_INFO',
                           'VP_STATE_CREATE_INFO',
                           'MS_STATE_CREATE_INFO'
                           ]
        for pipe_state in pipe_state_list:
            hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_%s:' % pipe_state)
            hf_body.append('                totalStateSize += sizeof(XGL_PIPELINE_%s);' % pipe_state)
            hf_body.append('                break;')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                totalStateSize += (sizeof(XGL_PIPELINE_CB_STATE_CREATE_INFO) + ((XGL_PIPELINE_CB_STATE_CREATE_INFO *) pNext)->attachmentCount * sizeof(XGL_PIPELINE_CB_ATTACHMENT_STATE));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pShaderStage = (const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext;')
        hf_body.append('                totalStateSize += (sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO) + calculate_pipeline_shader_size(&pShaderStage->shader));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pVi = (const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pNext;')
        hf_body.append('                totalStateSize += sizeof(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO) + pVi->bindingCount * sizeof(XGL_VERTEX_INPUT_BINDING_DESCRIPTION)')
        hf_body.append('                                    + pVi->attributeCount * sizeof(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION);')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            default:')
        hf_body.append('                assert(0);')
        hf_body.append('        }')
        hf_body.append('        pNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pNext->pNext;')
        hf_body.append('    }')
        hf_body.append('    return totalStateSize;')
        hf_body.append('}')
        hf_body.append('')
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
        pid_enum.append('static uint64_t calc_size_XGL_APPLICATION_INFO(const XGL_APPLICATION_INFO* pStruct)')
        pid_enum.append('{')
        pid_enum.append('    return ((pStruct == NULL) ? 0 : sizeof(XGL_APPLICATION_INFO)) + strlen(pStruct->pAppName) + 1 + strlen(pStruct->pEngineName) + 1;')
        pid_enum.append('}\n')
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
        pid_enum.append('static uint64_t calc_size_XGL_DEVICE_CREATE_INFO(const XGL_DEVICE_CREATE_INFO* pStruct)')
        pid_enum.append('{')
        pid_enum.append('    uint64_t total_size_ppEnabledExtensionNames = pStruct->extensionCount * sizeof(char *);')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    for (i = 0; i < pStruct->extensionCount; i++)')
        pid_enum.append('    {')
        pid_enum.append('        total_size_ppEnabledExtensionNames += strlen(pStruct->ppEnabledExtensionNames[i]) + 1;')
        pid_enum.append('    }')
        pid_enum.append('    uint64_t total_size_layers = 0;')
        pid_enum.append('    XGL_LAYER_CREATE_INFO *pNext = ( XGL_LAYER_CREATE_INFO *) pStruct->pNext;')
        pid_enum.append('    while (pNext != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        if ((pNext->sType == XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pNext->layerCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            total_size_layers += sizeof(XGL_LAYER_CREATE_INFO);')
        pid_enum.append('            for (i = 0; i < pNext->layerCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                total_size_layers += strlen(pNext->ppActiveLayerNames[i]) + 1;')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('        pNext = ( XGL_LAYER_CREATE_INFO *) pNext->pNext;')
        pid_enum.append('    }')
        pid_enum.append('    return sizeof(XGL_DEVICE_CREATE_INFO) + (pStruct->queueRecordCount*sizeof(XGL_DEVICE_CREATE_INFO)) + total_size_ppEnabledExtensionNames + total_size_layers;')
        pid_enum.append('}\n')
        pid_enum.append('static void add_XGL_DEVICE_CREATE_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_DEVICE_CREATE_INFO** ppStruct, const XGL_DEVICE_CREATE_INFO *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_DEVICE_CREATE_INFO), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pRequestedQueues, pInStruct->queueRecordCount*sizeof(XGL_DEVICE_CREATE_INFO), pInStruct->pRequestedQueues);')
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
                             'CreateDescriptorSetLayout' : {'param': 'pSetLayoutInfoList', 'txt': ['assert(pPacket->pSetLayoutInfoList->sType == XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);\n',
                                                                                         '// need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         'void** ppNextVoidPtr = (void**)&(pPacket->pSetLayoutInfoList->pNext);\n',
                                                                                         '*ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetLayoutInfoList->pNext);\n',
                                                                                         'XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pPacket->pSetLayoutInfoList->pNext;\n',
                                                                                         'while (NULL != pNext)\n', '{\n',
                                                                                         '    switch(pNext->sType)\n', '    {\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        default:\n',
                                                                                         '            assert(!"Encountered an unexpected type in descriptor set layout create list");\n',
                                                                                         '    }\n',
                                                                                         '    pNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pNext->pNext;\n',
                                                                                         '}']},
                             'BeginCommandBuffer' : {'param': 'pBeginInfo', 'txt': ['assert(pPacket->pBeginInfo->sType == XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO);\n',
                                                                                         '// need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         'XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO** ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO**)&(pPacket->pBeginInfo->pNext);\n',
                                                                                         '*ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBeginInfo->pNext);\n',
                                                                                         'XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pNext = *ppNext;\n',
                                                                                         'while (NULL != pNext)\n', '{\n',
                                                                                         '    switch(pNext->sType)\n', '    {\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO**) &pNext->pNext;\n',
                                                                                         '            *ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        default:\n',
                                                                                         '            assert(!"Encountered an unexpected type in begin command buffer list");\n',
                                                                                         '    }\n',
                                                                                         '    pNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pNext->pNext;\n',
                                                                                         '}']},
                             'AllocMemory' : {'param': 'pAllocInfo', 'txt': ['assert(pPacket->pAllocInfo->sType == XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO);\n',
                                                                                         'XGL_MEMORY_ALLOC_INFO** ppNext = (XGL_MEMORY_ALLOC_INFO**) &(pPacket->pAllocInfo->pNext);\n',
                                                                                         '*ppNext = (XGL_MEMORY_ALLOC_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocInfo->pNext);\n',
                                                                                         'XGL_MEMORY_ALLOC_INFO* pNext = (XGL_MEMORY_ALLOC_INFO*) *ppNext;\n',
                                                                                         'while (NULL != pNext)\n', '{\n',
                                                                                         '    switch(pNext->sType)\n', '    {\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            ppNext = (XGL_MEMORY_ALLOC_INFO **) &(pNext->pNext);\n',
                                                                                         '            *ppNext = (XGL_MEMORY_ALLOC_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        default:\n',
                                                                                         '            assert(!"Encountered an unexpected type in alloc memory list");\n',
                                                                                         '    }\n',
                                                                                         '    pNext = (XGL_MEMORY_ALLOC_INFO*)pNext->pNext;\n',
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
                                                                                         '            assert(!"Encountered an unexpected type in update descriptors pUpdateChain");\n',
                                                                                         '    }\n',
                                                                                         '    pNext = (XGL_UPDATE_SAMPLERS*)pNext->pNext;\n',
                                                                                         '}']},
                             'CreateGraphicsPipeline' : {'param': 'pCreateInfo', 'txt': ['assert(pPacket->pCreateInfo->sType == XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);\n',
                                                                                         '// need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n',
                                                                                         'void** ppNextVoidPtr = (void**)&pPacket->pCreateInfo->pNext;\n',
                                                                                         '*ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pNext);\n',
                                                                                         'XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;\n',
                                                                                         'while ((NULL != pNext) && (XGL_NULL_HANDLE != pNext))\n', '{\n',
                                                                                         '    switch(pNext->sType)\n', '    {\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            XGL_PIPELINE_CB_STATE_CREATE_INFO *pCb = (XGL_PIPELINE_CB_STATE_CREATE_INFO *) pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            pCb->pAttachments = (XGL_PIPELINE_CB_ATTACHMENT_STATE*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pCb->pAttachments);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            interpret_pipeline_shader(pHeader, &pNext->shader);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            void** ppNextVoidPtr = (void**)&pNext->pNext;\n',
                                                                                         '            XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pVi = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *) pNext;\n',
                                                                                         '            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            pVi->pVertexBindingDescriptions = (XGL_VERTEX_INPUT_BINDING_DESCRIPTION*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexBindingDescriptions);\n',
                                                                                         '            pVi->pVertexAttributeDescriptions = (XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexAttributeDescriptions);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        default:\n',
                                                                                         '            assert(!"Encountered an unexpected type in pipeline state list");\n',
                                                                                         '    }\n',
                                                                                         '    pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;\n',
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

    def _generate_replay_class_decls(self):
        cd_body = []
        cd_body.append('class ApiReplay {')
        cd_body.append('public:')
        cd_body.append('    virtual ~ApiReplay() { }')
        cd_body.append('    virtual enum glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header * packet) = 0;')
        cd_body.append('    virtual int init(glv_replay::Display & disp) = 0;')
        cd_body.append('    virtual void push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char* pMsg) = 0;')
        cd_body.append('    virtual glv_replay::GLV_REPLAY_RESULT pop_validation_msgs() = 0;')
        cd_body.append('};\n')
        cd_body.append('class xglDisplay: public glv_replay::DisplayImp {')
        cd_body.append('friend class xglReplay;')
        cd_body.append('public:')
        cd_body.append('    xglDisplay();')
        cd_body.append('    ~xglDisplay();')
        cd_body.append('    int init(const unsigned int gpu_idx);')
        cd_body.append('    int set_window(glv_window_handle hWindow, unsigned int width, unsigned int height);')
        cd_body.append('    int create_window(const unsigned int width, const unsigned int height);')
        cd_body.append('    void resize_window(const unsigned int width, const unsigned int height);')
        cd_body.append('    void process_event();')
        cd_body.append('    // XGL_DEVICE get_device() { return m_dev[m_gpuIdx];}')
        cd_body.append('#if defined(WIN32)')
        cd_body.append('    HWND get_window_handle() { return m_windowHandle; }')
        cd_body.append('#elif defined(PLATFORM_LINUX)')
        cd_body.append('    xcb_window_t get_window_handle() { return m_XcbWindow; }')
        cd_body.append('#endif')
        cd_body.append('private:')
        cd_body.append('    XGL_RESULT init_xgl(const unsigned int gpu_idx);')
        cd_body.append('    bool m_initedXGL;')
        cd_body.append('#if defined(WIN32)')
        cd_body.append('    HWND m_windowHandle;')
        cd_body.append('#elif defined(PLATFORM_LINUX)')
        cd_body.append('    XGL_WSI_X11_CONNECTION_INFO m_WsiConnection;')
        cd_body.append('    xcb_screen_t *m_pXcbScreen;')
        cd_body.append('    xcb_window_t m_XcbWindow;')
        cd_body.append('#endif')
        cd_body.append('    unsigned int m_windowWidth;')
        cd_body.append('    unsigned int m_windowHeight;')
        cd_body.append('    unsigned int m_frameNumber;')
        cd_body.append('    std::vector<uint32_t> imageWidth;')
        cd_body.append('    std::vector<uint32_t> imageHeight;')
        cd_body.append('    std::vector<XGL_IMAGE> imageHandles;')
        cd_body.append('    std::vector<XGL_GPU_MEMORY> imageMemory;')
        cd_body.append('#if 0')
        cd_body.append('    XGL_DEVICE m_dev[XGL_MAX_PHYSICAL_GPUS];')
        cd_body.append('    uint32_t m_gpuCount;')
        cd_body.append('    unsigned int m_gpuIdx;')
        cd_body.append('    XGL_PHYSICAL_GPU m_gpus[XGL_MAX_PHYSICAL_GPUS];')
        cd_body.append('    XGL_PHYSICAL_GPU_PROPERTIES m_gpuProps[XGL_MAX_PHYSICAL_GPUS];')
        cd_body.append('#endif')
        cd_body.append('    std::vector<char *>m_extensions;')
        cd_body.append('};\n')
        cd_body.append('typedef struct _XGLAllocInfo {')
        cd_body.append('    XGL_GPU_SIZE size;')
        cd_body.append('    void *pData;')
        cd_body.append('} XGLAllocInfo;')
        return "\n".join(cd_body)

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

    def _generate_replay_class(self):
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
        rc_body.append('class xglReplay : public ApiReplay {')
        rc_body.append('public:')
        rc_body.append('    ~xglReplay();')
        rc_body.append('    xglReplay(glvreplay_settings *pReplaySettings);\n')
        rc_body.append('    int init(glv_replay::Display & disp);')
        rc_body.append('    xglDisplay * get_display() {return m_display;}')
        rc_body.append('    glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header *packet);')
        rc_body.append('    glv_replay::GLV_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn);\n')
        rc_body.append('    void push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char* pMsg);')
        rc_body.append('    glv_replay::GLV_REPLAY_RESULT pop_validation_msgs();')
        rc_body.append('private:')
        rc_body.append('    struct xglFuncs m_xglFuncs;')
        rc_body.append('    xglDisplay *m_display;')
        rc_body.append('    struct shaderPair {')
        rc_body.append('        XGL_SHADER *addr;')
        rc_body.append('        XGL_SHADER val;')
        rc_body.append('    };')
        rc_body.append('    struct validationMsg {')
        rc_body.append('        XGL_VALIDATION_LEVEL validationLevel;')
        rc_body.append('        XGL_BASE_OBJECT srcObject;')
        rc_body.append('        size_t location;')
        rc_body.append('        int32_t msgCode;')
        rc_body.append('        char msg[256];')
        rc_body.append('    };')
        rc_body.append('    std::vector<struct validationMsg> m_validationMsgs;')
        rc_body.append('    std::vector<int> m_screenshotFrames;')
        rc_body.append(self._map_decl('XGL_GPU_MEMORY', 'XGLAllocInfo', 'm_mapData'))
        # Custom code for 1-off memory mapping functions
        rc_body.append('    void add_entry_to_mapData(XGL_GPU_MEMORY handle, XGL_GPU_SIZE size)')
        rc_body.append('    {')
        rc_body.append('        XGLAllocInfo info;')
        rc_body.append('        info.pData = NULL;')
        rc_body.append('        info.size = size;')
        rc_body.append('        m_mapData.insert(std::pair<XGL_GPU_MEMORY, XGLAllocInfo>(handle, info));')
        rc_body.append('    }')
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
        rc_body.append('        {')
        rc_body.append('            glv_LogWarn("add_mapping_to_mapData() data already mapped overwrite old mapping\\n");')
        rc_body.append('        }')
        rc_body.append('        info.pData = pData;')
        rc_body.append('    }')
        rc_body.append('    void rm_entry_from_mapData(XGL_GPU_MEMORY handle)')
        rc_body.append('    {')
        rc_body.append('        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);')
        rc_body.append('        if (it == m_mapData.end())')
        rc_body.append('            return;')
        rc_body.append('        m_mapData.erase(it);')
        rc_body.append('    }')
        rc_body.append('    void rm_mapping_from_mapData(XGL_GPU_MEMORY handle, void* pData)')
        rc_body.append('    {')
        rc_body.append('        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);')
        rc_body.append('        if (it == m_mapData.end())')
        rc_body.append('            return;\n')
        rc_body.append('        XGLAllocInfo &info = it->second;')
        rc_body.append('        if (!pData || !info.pData)')
        rc_body.append('        {')
        rc_body.append('            glv_LogWarn("rm_mapping_from_mapData() null src or dest pointers\\n");')
        rc_body.append('            info.pData = NULL;')
        rc_body.append('            return;')
        rc_body.append('        }')
        rc_body.append('        memcpy(info.pData, pData, info.size);')
        rc_body.append('        info.pData = NULL;')
        rc_body.append('    }\n')
        rc_body.append('    /*std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU> m_gpus;')
        rc_body.append('    void add_to_map(XGL_PHYSICAL_GPU* pTraceGpu, XGL_PHYSICAL_GPU* pReplayGpu)')
        rc_body.append('    {')
        rc_body.append('        assert(pTraceGpu != NULL);')
        rc_body.append('        assert(pReplayGpu != NULL);')
        rc_body.append('        m_gpus[*pTraceGpu] = *pReplayGpu;')
        rc_body.append('    }\n')
        rc_body.append('    XGL_PHYSICAL_GPU remap(const XGL_PHYSICAL_GPU& gpu)')
        rc_body.append('    {')
        rc_body.append('        std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU>::const_iterator q = m_gpus.find(gpu);')
        rc_body.append('        return (q == m_gpus.end()) ? XGL_NULL_HANDLE : q->second;')
        rc_body.append('    }*/\n')
        rc_body.append('    void clear_all_map_handles()\n    {')
        for var in sorted(obj_map_dict):
            rc_body.append('        %s.clear();' % var)
        rc_body.append('    }')
        for var in sorted(obj_map_dict):
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
        rc_body.append('        return XGL_NULL_HANDLE;\n    }')
        rc_body.append('void process_screenshot_list(const char *list)')
        rc_body.append('{')
        rc_body.append('    std::string spec(list), word;')
        rc_body.append('    size_t start = 0, comma = 0;\n')
        rc_body.append('    while (start < spec.size()) {')
        rc_body.append('        comma = spec.find(\',\', start);\n')
        rc_body.append('        if (comma == std::string::npos)')
        rc_body.append('            word = std::string(spec, start);')
        rc_body.append('        else')
        rc_body.append('            word = std::string(spec, start, comma - start);\n')
        rc_body.append('        m_screenshotFrames.push_back(atoi(word.c_str()));')
        rc_body.append('        if (comma == std::string::npos)')
        rc_body.append('            break;\n')
        rc_body.append('        start = comma + 1;\n')
        rc_body.append('    }')
        rc_body.append('}')
        rc_body.append('};')
        return "\n".join(rc_body)

    def _generate_replay_display_init_xgl(self):
        dix_body = []
        dix_body.append('XGL_RESULT xglDisplay::init_xgl(unsigned int gpu_idx)')
        dix_body.append('{')
        dix_body.append('#if 0')
        dix_body.append('    XGL_APPLICATION_INFO appInfo = {};')
        dix_body.append('    appInfo.pAppName = APP_NAME;')
        dix_body.append('    appInfo.pEngineName = "";')
        dix_body.append('    appInfo.apiVersion = XGL_API_VERSION;')
        dix_body.append('    XGL_RESULT res = xglInitAndEnumerateGpus(&appInfo, NULL, XGL_MAX_PHYSICAL_GPUS, &m_gpuCount, m_gpus);')
        dix_body.append('    if ( res == XGL_SUCCESS ) {')
        dix_body.append('        // retrieve the GPU information for all GPUs')
        dix_body.append('        for( uint32_t gpu = 0; gpu < m_gpuCount; gpu++)')
        dix_body.append('        {')
        dix_body.append('            size_t gpuInfoSize = sizeof(m_gpuProps[0]);\n')
        dix_body.append('            // get the GPU physical properties:')
        dix_body.append('            res = xglGetGpuInfo( m_gpus[gpu], XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, &gpuInfoSize, &m_gpuProps[gpu]);')
        dix_body.append('            if (res != XGL_SUCCESS)')
        dix_body.append('                glv_LogWarn("Failed to retrieve properties for gpu[%d] result %d\\n", gpu, res);')
        dix_body.append('        }')
        dix_body.append('        res = XGL_SUCCESS;')
        dix_body.append('    } else if ((gpu_idx + 1) > m_gpuCount) {')
        dix_body.append('        glv_LogError("xglInitAndEnumerate number of gpus does not include requested index: num %d, requested %d\\n", m_gpuCount, gpu_idx);')
        dix_body.append('        return -1;')
        dix_body.append('    } else {')
        dix_body.append('        glv_LogError("xglInitAndEnumerate failed\\n");')
        dix_body.append('        return res;')
        dix_body.append('    }')
        dix_body.append('    // TODO add multi-gpu support always use gpu[gpu_idx] for now')
        dix_body.append('    // get all extensions supported by this device gpu[gpu_idx]')
        dix_body.append('    // first check if extensions are available and save a list of them')
        dix_body.append('    bool foundWSIExt = false;')
        dix_body.append('    for( int ext = 0; ext < sizeof( extensions ) / sizeof( extensions[0] ); ext++)')
        dix_body.append('    {')
        dix_body.append('        res = xglGetExtensionSupport( m_gpus[gpu_idx], extensions[ext] );')
        dix_body.append('        if (res == XGL_SUCCESS) {')
        dix_body.append('            m_extensions.push_back((char *) extensions[ext]);')
        dix_body.append('            if (!strcmp(extensions[ext], "XGL_WSI_WINDOWS"))')
        dix_body.append('                foundWSIExt = true;')
        dix_body.append('        }')
        dix_body.append('    }')
        dix_body.append('    if (!foundWSIExt) {')
        dix_body.append('        glv_LogError("XGL_WSI_WINDOWS extension not supported by gpu[%d]\\n", gpu_idx);')
        dix_body.append('        return XGL_ERROR_INCOMPATIBLE_DEVICE;')
        dix_body.append('    }')
        dix_body.append('    // TODO generalize this: use one universal queue for now')
        dix_body.append('    XGL_DEVICE_QUEUE_CREATE_INFO dqci = {};')
        dix_body.append('    dqci.queueCount = 1;')
        dix_body.append('    dqci.queueType = XGL_QUEUE_UNIVERSAL;')
        dix_body.append('    // create the device enabling validation level 4')
        dix_body.append('    const char * const * extNames = &m_extensions[0];')
        dix_body.append('    XGL_DEVICE_CREATE_INFO info = {};')
        dix_body.append('    info.queueRecordCount = 1;')
        dix_body.append('    info.pRequestedQueues = &dqci;')
        dix_body.append('    info.extensionCount = static_cast <uint32_t> (m_extensions.size());')
        dix_body.append('    info.ppEnabledExtensionNames = extNames;')
        dix_body.append('    info.flags = XGL_DEVICE_CREATE_VALIDATION;')
        dix_body.append('    info.maxValidationLevel = XGL_VALIDATION_LEVEL_4;')
        dix_body.append('    bool32_t xglTrue = XGL_TRUE;')
        dix_body.append('    res = xglDbgSetGlobalOption( XGL_DBG_OPTION_BREAK_ON_ERROR, sizeof( xglTrue ), &xglTrue );')
        dix_body.append('    if (res != XGL_SUCCESS)')
        dix_body.append('        glv_LogWarn("Could not set debug option break on error\\n");')
        dix_body.append('    res = xglCreateDevice( m_gpus[0], &info, &m_dev[gpu_idx]);')
        dix_body.append('    return res;')
        dix_body.append('#else')
        dix_body.append('    return XGL_ERROR_INITIALIZATION_FAILED;')
        dix_body.append('#endif')
        dix_body.append('}')
        return "\n".join(dix_body)

    def _generate_replay_display_init(self):
        di_body = []
        di_body.append('int xglDisplay::init(const unsigned int gpu_idx)')
        di_body.append('{')
        di_body.append('    //m_gpuIdx = gpu_idx;')
        di_body.append('#if 0')
        di_body.append('    XGL_RESULT result = init_xgl(gpu_idx);')
        di_body.append('    if (result != XGL_SUCCESS) {')
        di_body.append('        glv_LogError("could not init xgl library");')
        di_body.append('        return -1;')
        di_body.append('    } else {')
        di_body.append('        m_initedXGL = true;')
        di_body.append('    }')
        di_body.append('#endif')
        di_body.append('#if defined(PLATFORM_LINUX)')
        di_body.append('    const xcb_setup_t *setup;')
        di_body.append('    xcb_screen_iterator_t iter;')
        di_body.append('    int scr;')
        di_body.append('    xcb_connection_t *pConnection;')
        di_body.append('    pConnection = xcb_connect(NULL, &scr);')
        di_body.append('    setup = xcb_get_setup(pConnection);')
        di_body.append('    iter = xcb_setup_roots_iterator(setup);')
        di_body.append('    while (scr-- > 0)')
        di_body.append('        xcb_screen_next(&iter);')
        di_body.append('    m_pXcbScreen = iter.data;')
        di_body.append('    m_WsiConnection.pConnection = pConnection;')
        di_body.append('    m_WsiConnection.root = m_pXcbScreen->root;')
        di_body.append('#endif')
        di_body.append('    return 0;')
        di_body.append('}')
        return "\n".join(di_body)

    def _generate_replay_display_structors(self):
        ds_body = []
        ds_body.append('xglDisplay::xglDisplay()')
        ds_body.append('    : m_initedXGL(false),')
        ds_body.append('    m_windowWidth(0),')
        ds_body.append('    m_windowHeight(0)')
        ds_body.append('{')
        ds_body.append('#if defined(WIN32)')
        ds_body.append('    m_windowHandle = NULL;')
        ds_body.append('#elif defined(PLATFORM_LINUX)')
        ds_body.append('    m_WsiConnection.pConnection = NULL;')
        ds_body.append('    m_WsiConnection.root = 0;')
        ds_body.append('    m_WsiConnection.provider = 0;')
        ds_body.append('    m_pXcbScreen = NULL;')
        ds_body.append('    m_XcbWindow = 0;')
        ds_body.append('#endif')
        ds_body.append('}')
        ds_body.append('xglDisplay::~xglDisplay()')
        ds_body.append('{')
        ds_body.append('#ifdef PLATFORM_LINUX')
        ds_body.append('    if (m_XcbWindow != 0)')
        ds_body.append('    {')
        ds_body.append('        xcb_destroy_window(m_WsiConnection.pConnection, m_XcbWindow);')
        ds_body.append('    }')
        ds_body.append('    if (m_WsiConnection.pConnection != NULL)')
        ds_body.append('    {')
        ds_body.append('        xcb_disconnect(m_WsiConnection.pConnection);')
        ds_body.append('    }')
        ds_body.append('#endif')
        ds_body.append('}')
        return "\n".join(ds_body)

    def _generate_replay_display_window(self):
        dw_body = []
        dw_body.append('#if defined(WIN32)')
        dw_body.append('LRESULT WINAPI WindowProcXgl( HWND window, unsigned int msg, WPARAM wp, LPARAM lp)')
        dw_body.append('{')
        dw_body.append('    switch(msg)')
        dw_body.append('    {')
        dw_body.append('        case WM_CLOSE:')
        dw_body.append('            DestroyWindow( window);')
        dw_body.append('            // fall-thru')
        dw_body.append('        case WM_DESTROY:')
        dw_body.append('            PostQuitMessage(0) ;')
        dw_body.append('            return 0L ;')
        dw_body.append('        default:')
        dw_body.append('            return DefWindowProc( window, msg, wp, lp ) ;')
        dw_body.append('    }')
        dw_body.append('}')
        dw_body.append('#endif')
        dw_body.append('int xglDisplay::set_window(glv_window_handle hWindow, unsigned int width, unsigned int height)')
        dw_body.append('{')
        dw_body.append('#if defined(WIN32)')
        dw_body.append('    m_windowHandle = hWindow;')
        dw_body.append('#elif defined(PLATFORM_LINUX)')
        dw_body.append('    m_XcbWindow = hWindow;')
        dw_body.append('#endif')
        dw_body.append('    m_windowWidth = width;')
        dw_body.append('    m_windowHeight = height;')
        dw_body.append('    return 0;')
        dw_body.append('}\n')
        dw_body.append('int xglDisplay::create_window(const unsigned int width, const unsigned int height)')
        dw_body.append('{')
        dw_body.append('#if defined(WIN32)')
        dw_body.append('    // Register Window class')
        dw_body.append('    WNDCLASSEX wcex = {};')
        dw_body.append('    wcex.cbSize = sizeof( WNDCLASSEX);')
        dw_body.append('    wcex.style = CS_HREDRAW | CS_VREDRAW;')
        dw_body.append('    wcex.lpfnWndProc = WindowProcXgl;')
        dw_body.append('    wcex.cbClsExtra = 0;')
        dw_body.append('    wcex.cbWndExtra = 0;')
        dw_body.append('    wcex.hInstance = GetModuleHandle(0);')
        dw_body.append('    wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE( IDI_ICON));')
        dw_body.append('    wcex.hCursor = LoadCursor( NULL, IDC_ARROW);')
        dw_body.append('    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1);')
        dw_body.append('    wcex.lpszMenuName = NULL;')
        dw_body.append('    wcex.lpszClassName = APP_NAME;')
        dw_body.append('    wcex.hIconSm = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_ICON));')
        dw_body.append('    if( !RegisterClassEx( &wcex))')
        dw_body.append('    {')
        dw_body.append('        glv_LogError("Failed to register windows class\\n");')
        dw_body.append('        return -1;')
        dw_body.append('    }\n')
        dw_body.append('    // create the window')
        dw_body.append('    m_windowHandle = CreateWindow(APP_NAME, APP_NAME, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 0, 0,')
        dw_body.append('                          width, height, NULL, NULL, wcex.hInstance, NULL);\n')
        dw_body.append('    if (m_windowHandle)')
        dw_body.append('    {')
        dw_body.append('        ShowWindow( m_windowHandle, SW_SHOWDEFAULT);')
        dw_body.append('        m_windowWidth = width;')
        dw_body.append('        m_windowHeight = height;')
        dw_body.append('    } else {')
        dw_body.append('        glv_LogError("Failed to create window\\n");')
        dw_body.append('        return -1;')
        dw_body.append('    }')
        dw_body.append('    return 0;')
        dw_body.append('#elif defined(PLATFORM_LINUX)\n')
        dw_body.append('    uint32_t value_mask, value_list[32];')
        dw_body.append('    m_XcbWindow = xcb_generate_id(m_WsiConnection.pConnection);\n')
        dw_body.append('    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;')
        dw_body.append('    value_list[0] = m_pXcbScreen->black_pixel;')
        dw_body.append('    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |')
        dw_body.append('                    XCB_EVENT_MASK_EXPOSURE;\n')
        dw_body.append('    xcb_create_window(m_WsiConnection.pConnection,')
        dw_body.append('            XCB_COPY_FROM_PARENT,')
        dw_body.append('            m_XcbWindow, m_WsiConnection.root,')
        dw_body.append('            0, 0, width, height, 0,')
        dw_body.append('            XCB_WINDOW_CLASS_INPUT_OUTPUT,')
        dw_body.append('            m_pXcbScreen->root_visual,')
        dw_body.append('            value_mask, value_list);\n')
        dw_body.append('    xcb_map_window(m_WsiConnection.pConnection, m_XcbWindow);')
        dw_body.append('    xcb_flush(m_WsiConnection.pConnection);')
        dw_body.append('    return 0;')
        dw_body.append('#endif')
        dw_body.append('}\n')
        dw_body.append('void xglDisplay::resize_window(const unsigned int width, const unsigned int height)')
        dw_body.append('{')
        dw_body.append('#if defined(WIN32)')
        dw_body.append('    if (width != m_windowWidth || height != m_windowHeight)')
        dw_body.append('    {')
        dw_body.append('        SetWindowPos(get_window_handle(), HWND_TOP, 0, 0, width, height, SWP_NOMOVE);')
        dw_body.append('        m_windowWidth = width;')
        dw_body.append('        m_windowHeight = height;')
        dw_body.append('    }')
        dw_body.append('#elif defined(PLATFORM_LINUX)')
        dw_body.append('    if (width != m_windowWidth || height != m_windowHeight)')
        dw_body.append('    {')
        dw_body.append('        uint32_t values[2];')
        dw_body.append('        values[0] = width;')
        dw_body.append('        values[1] = height;')
        dw_body.append('        xcb_configure_window(m_WsiConnection.pConnection, m_XcbWindow, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);')
        dw_body.append('        m_windowWidth = width;')
        dw_body.append('        m_windowHeight = height;')
        dw_body.append('    }')
        dw_body.append('#endif')
        dw_body.append('}\n')
        dw_body.append('void xglDisplay::process_event()')
        dw_body.append('{')
        dw_body.append('}')
        return "\n".join(dw_body)

    def _generate_replay_structors(self):
        rs_body = []
        rs_body.append('xglReplay::xglReplay(glvreplay_settings *pReplaySettings)')
        rs_body.append('{')
        rs_body.append('    g_pReplaySettings = pReplaySettings;')
        rs_body.append('    m_display = new xglDisplay();')
        rs_body.append('    if (g_pReplaySettings && g_pReplaySettings->screenshotList) {')
        rs_body.append('        process_screenshot_list(g_pReplaySettings->screenshotList);')
        rs_body.append('    }')
        rs_body.append('}\n')
        rs_body.append('xglReplay::~xglReplay()')
        rs_body.append('{')
        rs_body.append('    delete m_display;')
        rs_body.append('    glv_platform_close_library(m_xglFuncs.m_libHandle);')
        rs_body.append('}')
        return "\n".join(rs_body)

    def _generate_replay_init(self):
        ri_body = []
        ri_body.append('int xglReplay::init(glv_replay::Display & disp)')
        ri_body.append('{')
        ri_body.append('    int err;')
        ri_body.append('#if defined PLATFORM_LINUX')
        ri_body.append('    void * handle = dlopen("libXGL.so", RTLD_LAZY);')
        ri_body.append('#else')
        ri_body.append('    HMODULE handle = LoadLibrary("xgl.dll" );')
        ri_body.append('#endif\n')
        ri_body.append('    if (handle == NULL) {')
        ri_body.append('        glv_LogError("Failed to open xgl library.\\n");')
        ri_body.append('        return -1;')
        ri_body.append('    }')
        ri_body.append('    m_xglFuncs.init_funcs(handle);')
        ri_body.append('    disp.set_implementation(m_display);')
        ri_body.append('    if ((err = m_display->init(disp.get_gpu())) != 0) {')
        ri_body.append('        glv_LogError("Failed to init XGL display.\\n");')
        ri_body.append('        return err;')
        ri_body.append('    }')
        ri_body.append('    if (disp.get_window_handle() == 0)')
        ri_body.append('    {')
        ri_body.append('        if ((err = m_display->create_window(disp.get_width(), disp.get_height())) != 0) {')
        ri_body.append('            glv_LogError("Failed to create Window\\n");')
        ri_body.append('            return err;')
        ri_body.append('        }')
        ri_body.append('    }')
        ri_body.append('    else')
        ri_body.append('    {')
        ri_body.append('        if ((err = m_display->set_window(disp.get_window_handle(), disp.get_width(), disp.get_height())) != 0)')
        ri_body.append('        {')
        ri_body.append('            glv_LogError("Failed to set Window\\n");')
        ri_body.append('            return err;')
        ri_body.append('        }')
        ri_body.append('    }')
        ri_body.append('    return 0;')
        ri_body.append('}')
        return "\n".join(ri_body)

    def _generate_replay_errors(self):
        re_body = []
        re_body.append('glv_replay::GLV_REPLAY_RESULT xglReplay::handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn)')
        re_body.append('{')
        re_body.append('    glv_replay::GLV_REPLAY_RESULT res = resIn;')
        re_body.append('    if (resCall != resTrace) {')
        re_body.append('        glv_LogWarn("Mismatched return from API call (%s) traced result %s, replay result %s\\n", entrypointName,')
        re_body.append('                string_XGL_RESULT((XGL_RESULT)resTrace), string_XGL_RESULT((XGL_RESULT)resCall));')
        re_body.append('        res = glv_replay::GLV_REPLAY_BAD_RETURN;')
        re_body.append('    }')
        re_body.append('#if 0')
        re_body.append('    if (resCall != XGL_SUCCESS) {')
        re_body.append('        glv_LogWarn("API call (%s) returned failed result %s\\n", entrypointName, string_XGL_RESULT(resCall));')
        re_body.append('    }')
        re_body.append('#endif')
        re_body.append('    return res;')
        re_body.append('}')
        return "\n".join(re_body)

    def _generate_replay_validation_funcs(self):
        rvf_body = []
        rvf_body.append('void xglReplay::push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char * pMsg)')
        rvf_body.append('{')
        rvf_body.append('    struct validationMsg msgObj;')
        rvf_body.append('    msgObj.validationLevel = validationLevel;')
        rvf_body.append('    msgObj.srcObject = srcObject;')
        rvf_body.append('    msgObj.location = location;')
        rvf_body.append('    msgObj.msgCode = msgCode;')
        rvf_body.append('    strncpy(msgObj.msg, pMsg, 256);')
        rvf_body.append("    msgObj.msg[255] = '\\0';")
        rvf_body.append('    m_validationMsgs.push_back(msgObj);')
        rvf_body.append('}\n')
        rvf_body.append('glv_replay::GLV_REPLAY_RESULT xglReplay::pop_validation_msgs()')
        rvf_body.append('{')
        rvf_body.append('    if (m_validationMsgs.size() == 0)')
        rvf_body.append('        return glv_replay::GLV_REPLAY_SUCCESS;')
        rvf_body.append('    m_validationMsgs.clear();')
        rvf_body.append('    return glv_replay::GLV_REPLAY_VALIDATION_ERROR;')
        rvf_body.append('}')
        return "\n".join(rvf_body)

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
                    return 'remap(*pPacket->%s)' % (n)
                else: # TODO : Don't remap array ptrs?
                    return 'pPacket->%s' % (n)
            return 'remap(pPacket->%s)' % (n)
        return 'pPacket->%s' % (n)

    def _gen_replay_enum_gpus(self):
        ieg_body = []
        ieg_body.append('            if (!m_display->m_initedXGL)')
        ieg_body.append('            {')
        ieg_body.append('                uint32_t gpuCount;')
        ieg_body.append('                XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];')
        ieg_body.append('                uint32_t maxGpus = (pPacket->maxGpus < XGL_MAX_PHYSICAL_GPUS) ? pPacket->maxGpus : XGL_MAX_PHYSICAL_GPUS;')
        ieg_body.append('                replayResult = m_xglFuncs.real_xglEnumerateGpus(remap(pPacket->instance), maxGpus, &gpuCount, &gpus[0]);')
        ieg_body.append('                CHECK_RETURN_VALUE(xglEnumerateGpus);')
        ieg_body.append('                //TODO handle different number of gpus in trace versus replay')
        ieg_body.append('                if (gpuCount != *(pPacket->pGpuCount))')
        ieg_body.append('                {')
        ieg_body.append('                    glv_LogWarn("number of gpus mismatched in replay %u versus trace %u\\n", gpuCount, *(pPacket->pGpuCount));')
        ieg_body.append('                }')
        ieg_body.append('                else if (gpuCount == 0)')
        ieg_body.append('                {')
        ieg_body.append('                     glv_LogError("xglEnumerateGpus number of gpus is zero\\n");')
        ieg_body.append('                }')
        ieg_body.append('                else')
        ieg_body.append('                {')
        ieg_body.append('                    glv_LogInfo("Enumerated %d GPUs in the system\\n", gpuCount);')
        ieg_body.append('                }')
        ieg_body.append('                // TODO handle enumeration results in a different order from trace to replay')
        ieg_body.append('                for (uint32_t i = 0; i < gpuCount; i++)')
        ieg_body.append('                {')
        ieg_body.append('                    if (pPacket->pGpus)')
        ieg_body.append('                        add_to_map(&(pPacket->pGpus[i]), &(gpus[i]));')
        ieg_body.append('                }')
        ieg_body.append('            }')
        return "\n".join(ieg_body)

    def _gen_replay_get_gpu_info(self):
        ggi_body = []
        ggi_body.append('            if (!m_display->m_initedXGL)')
        ggi_body.append('            {')
        ggi_body.append('                switch (pPacket->infoType) {')
        ggi_body.append('                case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:')
        ggi_body.append('                {')
        ggi_body.append('                    XGL_PHYSICAL_GPU_PROPERTIES gpuProps;')
        ggi_body.append('                    size_t dataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);')
        ggi_body.append('                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,')
        ggi_body.append('                                    (pPacket->pData == NULL) ? NULL : &gpuProps);')
        ggi_body.append('                    if (pPacket->pData != NULL)')
        ggi_body.append('                    {')
        ggi_body.append('                        glv_LogInfo("Replay Gpu Properties\\n");')
        ggi_body.append('                        glv_LogInfo("Vendor ID %x, Device ID %x, name %s\\n",gpuProps.vendorId, gpuProps.deviceId, gpuProps.gpuName);')
        ggi_body.append('                        glv_LogInfo("API version %u, Driver version %u, gpu Type %u\\n",gpuProps.apiVersion, gpuProps.driverVersion, gpuProps.gpuType);')
        ggi_body.append('                    }')
        ggi_body.append('                    break;')
        ggi_body.append('                }')
        ggi_body.append('                case XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:')
        ggi_body.append('                {')
        ggi_body.append('                    XGL_PHYSICAL_GPU_PERFORMANCE gpuPerfs;')
        ggi_body.append('                    size_t dataSize = sizeof(XGL_PHYSICAL_GPU_PERFORMANCE);')
        ggi_body.append('                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,')
        ggi_body.append('                                    (pPacket->pData == NULL) ? NULL : &gpuPerfs);')
        ggi_body.append('                    if (pPacket->pData != NULL)')
        ggi_body.append('                    {')
        ggi_body.append('                        glv_LogInfo("Replay Gpu Performance\\n");')
        ggi_body.append('                        glv_LogInfo("Max GPU clock %f, max shader ALUs/clock %f, max texel fetches/clock %f\\n",gpuPerfs.maxGpuClock, gpuPerfs.aluPerClock, gpuPerfs.texPerClock);')
        ggi_body.append('                        glv_LogInfo("Max primitives/clock %f, Max pixels/clock %f\\n",gpuPerfs.primsPerClock, gpuPerfs.pixelsPerClock);')
        ggi_body.append('                    }')
        ggi_body.append('                    break;')
        ggi_body.append('                }')
        ggi_body.append('                case XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:')
        ggi_body.append('                {')
        ggi_body.append('                    XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *pGpuQueue, *pQ;')
        ggi_body.append('                    size_t dataSize = sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);')
        ggi_body.append('                    size_t numQueues = 1;')
        ggi_body.append('                    assert(pPacket->pDataSize);')
        ggi_body.append('                    if ((*(pPacket->pDataSize) % dataSize) != 0)')
        ggi_body.append('                        glv_LogWarn("xglGetGpuInfo() for GPU_QUEUE_PROPERTIES not an integral data size assuming 1\\n");')
        ggi_body.append('                    else')
        ggi_body.append('                        numQueues = *(pPacket->pDataSize) / dataSize;')
        ggi_body.append('                    dataSize = numQueues * dataSize;')
        ggi_body.append('                    pQ = static_cast < XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *> (glv_malloc(dataSize));')
        ggi_body.append('                    pGpuQueue = pQ;')
        ggi_body.append('                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,')
        ggi_body.append('                                    (pPacket->pData == NULL) ? NULL : pGpuQueue);')
        ggi_body.append('                    if (pPacket->pData != NULL)')
        ggi_body.append('                    {')
        ggi_body.append('                        for (unsigned int i = 0; i < numQueues; i++)')
        ggi_body.append('                        {')
        ggi_body.append('                            glv_LogInfo("Replay Gpu Queue Property for index %d, flags %u\\n", i, pGpuQueue->queueFlags);')
        ggi_body.append('                            glv_LogInfo("Max available count %u, max atomic counters %u, supports timestamps %u\\n",pGpuQueue->queueCount, pGpuQueue->maxAtomicCounters, pGpuQueue->supportsTimestamps);')
        ggi_body.append('                            pGpuQueue++;')
        ggi_body.append('                        }')
        ggi_body.append('                    }')
        ggi_body.append('                    glv_free(pQ);')
        ggi_body.append('                    break;')
        ggi_body.append('                }')
        ggi_body.append('                default:')
        ggi_body.append('                {')
        ggi_body.append('                    size_t size = 0;')
        ggi_body.append('                    void* pData = NULL;')
        ggi_body.append('                    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)')
        ggi_body.append('                    {')
        ggi_body.append('                        size = *pPacket->pDataSize;')
        ggi_body.append('                        pData = glv_malloc(*pPacket->pDataSize);')
        ggi_body.append('                    }')
        ggi_body.append('                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &size, pData);')
        ggi_body.append('                    if (replayResult == XGL_SUCCESS)')
        ggi_body.append('                    {')
        ggi_body.append('                        if (size != *pPacket->pDataSize && pData != NULL)')
        ggi_body.append('                        {')
        ggi_body.append('                            glv_LogWarn("xglGetGpuInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\\n", size, *pPacket->pDataSize);')
        ggi_body.append('                        }')
        ggi_body.append('                        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)')
        ggi_body.append('                        {')
        ggi_body.append('                            glv_LogWarn("xglGetGpuInfo returned differing data contents than the trace file contained.\\n");')
        ggi_body.append('                        }')
        ggi_body.append('                    }')
        ggi_body.append('                    glv_free(pData);')
        ggi_body.append('                    break;')
        ggi_body.append('                }')
        ggi_body.append('                };')
        ggi_body.append('                CHECK_RETURN_VALUE(xglGetGpuInfo);')
        ggi_body.append('            }')
        return "\n".join(ggi_body)

    def _gen_replay_create_device(self):
        cd_body = []
        cd_body.append('            if (!m_display->m_initedXGL)')
        cd_body.append('            {')
        cd_body.append('                XGL_DEVICE device;')
        cd_body.append('                if (g_xglReplaySettings.debugLevel > 0)')
        cd_body.append('                {')
        cd_body.append('                    XGL_DEVICE_CREATE_INFO cInfo, *ci, *pCreateInfoSaved;')
        cd_body.append('                    // TODO what is the real list of layers to be running with??')
#        cd_body.append('                    const char * layersStr[2] = {(char *) "DrawState", (char *) "ObjectTracker"};')

        cd_body.append('                    unsigned int numLayers = 0;')
        cd_body.append('                    char ** layersStr = get_enableLayers_list(&numLayers);')
        cd_body.append('                    apply_layerSettings_overrides();')
        cd_body.append('                    XGL_LAYER_CREATE_INFO layerInfo;')
        cd_body.append('                    pCreateInfoSaved = (XGL_DEVICE_CREATE_INFO *) pPacket->pCreateInfo;')
        cd_body.append('                    ci = (XGL_DEVICE_CREATE_INFO *) pPacket->pCreateInfo;')

        cd_body.append('                    if (layersStr != NULL && numLayers > 0)')
        cd_body.append('                    {')
        cd_body.append('                        while (ci->pNext != NULL)')
        cd_body.append('                            ci = (XGL_DEVICE_CREATE_INFO *) ci->pNext;')
        cd_body.append('                        ci->pNext = &layerInfo;')
        cd_body.append('                        layerInfo.sType = XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO;')
        cd_body.append('                        layerInfo.pNext = 0;')
        cd_body.append('                        layerInfo.layerCount = numLayers;')
        cd_body.append('                        layerInfo.ppActiveLayerNames = layersStr;')
        cd_body.append('                    }')

        cd_body.append('                    memcpy(&cInfo, pPacket->pCreateInfo, sizeof(XGL_DEVICE_CREATE_INFO));')
        cd_body.append('                    cInfo.flags = pPacket->pCreateInfo->flags | XGL_DEVICE_CREATE_VALIDATION_BIT;')
        cd_body.append('                    cInfo.maxValidationLevel = (XGL_VALIDATION_LEVEL)((g_xglReplaySettings.debugLevel <= 4) ? (unsigned int) XGL_VALIDATION_LEVEL_0 + g_xglReplaySettings.debugLevel : (unsigned int) XGL_VALIDATION_LEVEL_0);')
        cd_body.append('                    pPacket->pCreateInfo = &cInfo;')
        cd_body.append('                    replayResult = m_xglFuncs.real_xglCreateDevice(remap(pPacket->gpu), pPacket->pCreateInfo, &device);')
        cd_body.append('                    // restore the packet for next replay')
        cd_body.append('                    ci->pNext = NULL;')
        cd_body.append('                    pPacket->pCreateInfo = pCreateInfoSaved;')
        cd_body.append('                    release_enableLayer_list(layersStr);')
        cd_body.append('                    if (xglDbgRegisterMsgCallback(g_fpDbgMsgCallback, NULL) != XGL_SUCCESS)')
        cd_body.append('                        glv_LogError("Failed to register xgl callback for replayer error handling\\n");')
        cd_body.append('                }')
        cd_body.append('                else ')
        cd_body.append('                    replayResult = m_xglFuncs.real_xglCreateDevice(remap(pPacket->gpu), pPacket->pCreateInfo, &device);')
        cd_body.append('                CHECK_RETURN_VALUE(xglCreateDevice);')
        cd_body.append('                if (replayResult == XGL_SUCCESS)')
        cd_body.append('                {')
        cd_body.append('                    add_to_map(pPacket->pDevice, &device);')
        cd_body.append('                }')
        cd_body.append('            }')
        return "\n".join(cd_body)

    def _gen_replay_get_extension_support(self):
        ges_body = []
        ges_body.append('            if (!m_display->m_initedXGL) {')
        ges_body.append('                replayResult = m_xglFuncs.real_xglGetExtensionSupport(remap(pPacket->gpu), pPacket->pExtName);')
        ges_body.append('                CHECK_RETURN_VALUE(xglGetExtensionSupport);')
        ges_body.append('                if (replayResult == XGL_SUCCESS) {')
        ges_body.append('                    for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)')
        ges_body.append('                    {')
        ges_body.append('                        if (!strncmp(g_extensions[ext], pPacket->pExtName, strlen(g_extensions[ext]))) {')
        ges_body.append('                            bool extInList = false;')
        ges_body.append('                            for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {')
        ges_body.append('                                if (!strncmp(m_display->m_extensions[j], g_extensions[ext], strlen(g_extensions[ext])))')
        ges_body.append('                                    extInList = true;')
        ges_body.append('                                break;')
        ges_body.append('                            }')
        ges_body.append('                            if (!extInList)')
        ges_body.append('                                m_display->m_extensions.push_back((char *) g_extensions[ext]);')
        ges_body.append('                            break;')
        ges_body.append('                        }')
        ges_body.append('                    }')
        ges_body.append('                }')
        ges_body.append('            }')
        return "\n".join(ges_body)

    def _gen_replay_queue_submit(self):
        qs_body = []
        qs_body.append('            XGL_CMD_BUFFER *remappedBuffers = NULL;')
        qs_body.append('            if (pPacket->pCmdBuffers != NULL)')
        qs_body.append('            {')
        qs_body.append('                remappedBuffers = GLV_NEW_ARRAY( XGL_CMD_BUFFER, pPacket->cmdBufferCount);')
        qs_body.append('                for (uint32_t i = 0; i < pPacket->cmdBufferCount; i++)')
        qs_body.append('                {')
        qs_body.append('                    *(remappedBuffers + i) = remap(*(pPacket->pCmdBuffers + i));')
        qs_body.append('                }')
        qs_body.append('            }')
        qs_body.append('            XGL_MEMORY_REF* memRefs = NULL;')
        qs_body.append('            if (pPacket->pMemRefs != NULL)')
        qs_body.append('            {')
        qs_body.append('                memRefs = GLV_NEW_ARRAY(XGL_MEMORY_REF, pPacket->memRefCount);')
        qs_body.append('                memcpy(memRefs, pPacket->pMemRefs, sizeof(XGL_MEMORY_REF) * pPacket->memRefCount);')
        qs_body.append('                for (uint32_t i = 0; i < pPacket->memRefCount; i++)')
        qs_body.append('                {')
        qs_body.append('                    memRefs[i].mem = remap(pPacket->pMemRefs[i].mem);')
        qs_body.append('                }')
        qs_body.append('            }')
        qs_body.append('            replayResult = m_xglFuncs.real_xglQueueSubmit(remap(pPacket->queue), pPacket->cmdBufferCount, remappedBuffers, pPacket->memRefCount,')
        qs_body.append('                memRefs, remap(pPacket->fence));')
        qs_body.append('            GLV_DELETE(remappedBuffers);')
        qs_body.append('            GLV_DELETE(memRefs);')
        return "\n".join(qs_body)

    def _gen_replay_get_object_info(self):
        goi_body = []
        goi_body.append('            size_t size = 0;')
        goi_body.append('            void* pData = NULL;')
        goi_body.append('            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)')
        goi_body.append('            {')
        goi_body.append('                size = *pPacket->pDataSize;')
        goi_body.append('                pData = glv_malloc(*pPacket->pDataSize);')
        goi_body.append('                memcpy(pData, pPacket->pData, *pPacket->pDataSize);')
        goi_body.append('            }')
        goi_body.append('            replayResult = m_xglFuncs.real_xglGetObjectInfo(remap(pPacket->object), pPacket->infoType, &size, pData);')
        goi_body.append('            if (replayResult == XGL_SUCCESS)')
        goi_body.append('            {')
        goi_body.append('                if (size != *pPacket->pDataSize && pData != NULL)')
        goi_body.append('                {')
        goi_body.append('                    glv_LogWarn("xglGetObjectInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\\n", size, *pPacket->pDataSize);')
        goi_body.append('                }')
        goi_body.append('                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)')
        goi_body.append('                {')
        goi_body.append('                    glv_LogWarn("xglGetObjectInfo returned differing data contents than the trace file contained.\\n");')
        goi_body.append('                }')
        goi_body.append('            }')
        goi_body.append('            glv_free(pData);')
        return "\n".join(goi_body)

    def _gen_replay_get_format_info(self):
        gfi_body = []
        gfi_body.append('            size_t size = 0;')
        gfi_body.append('            void* pData = NULL;')
        gfi_body.append('            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)')
        gfi_body.append('            {')
        gfi_body.append('                size = *pPacket->pDataSize;')
        gfi_body.append('                pData = glv_malloc(*pPacket->pDataSize);')
        gfi_body.append('            }')
        gfi_body.append('            replayResult = m_xglFuncs.real_xglGetFormatInfo(remap(pPacket->device), pPacket->format, pPacket->infoType, &size, pData);')
        gfi_body.append('            if (replayResult == XGL_SUCCESS)')
        gfi_body.append('            {')
        gfi_body.append('                if (size != *pPacket->pDataSize && pData != NULL)')
        gfi_body.append('                {')
        gfi_body.append('                    glv_LogWarn("xglGetFormatInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\\n", size, *pPacket->pDataSize);')
        gfi_body.append('                }')
        gfi_body.append('                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)')
        gfi_body.append('                {')
        gfi_body.append('                    glv_LogWarn("xglGetFormatInfo returned differing data contents than the trace file contained.\\n");')
        gfi_body.append('                }')
        gfi_body.append('            }')
        gfi_body.append('            glv_free(pData);')
        return "\n".join(gfi_body)

    def _gen_replay_get_image_subresource_info(self):
        isi_body = []
        isi_body.append('            size_t size = 0;')
        isi_body.append('            void* pData = NULL;')
        isi_body.append('            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)')
        isi_body.append('            {')
        isi_body.append('                size = *pPacket->pDataSize;')
        isi_body.append('                pData = glv_malloc(*pPacket->pDataSize);')
        isi_body.append('            }')
        isi_body.append('            replayResult = m_xglFuncs.real_xglGetImageSubresourceInfo(remap(pPacket->image), pPacket->pSubresource, pPacket->infoType, &size, pData);')
        isi_body.append('            if (replayResult == XGL_SUCCESS)')
        isi_body.append('            {')
        isi_body.append('                if (size != *pPacket->pDataSize && pData != NULL)')
        isi_body.append('                {')
        isi_body.append('                    glv_LogWarn("xglGetImageSubresourceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\\n", size, *pPacket->pDataSize);')
        isi_body.append('                }')
        isi_body.append('                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)')
        isi_body.append('                {')
        isi_body.append('                    glv_LogWarn("xglGetImageSubresourceInfo returned differing data contents than the trace file contained.\\n");')
        isi_body.append('                }')
        isi_body.append('            }')
        isi_body.append('            glv_free(pData);')
        return "\n".join(isi_body)

    def _gen_replay_update_descriptors(self):
        ud_body = []
        # We have to remap handles internal to the structures so save the handles prior to remap and then restore
        # Rather than doing a deep memcpy of the entire struct and fixing any intermediate pointers, do save and restores via STL queue
        ud_body.append('            XGL_UPDATE_SAMPLERS* pUpdateChain = (XGL_UPDATE_SAMPLERS*)pPacket->pUpdateChain;')
        ud_body.append('            std::queue<XGL_SAMPLER> saveSamplers;')
        ud_body.append('            std::queue<XGL_BUFFER_VIEW> saveBufferViews;')
        ud_body.append('            std::queue<XGL_IMAGE_VIEW> saveImageViews;')
        ud_body.append('            std::queue<XGL_DESCRIPTOR_SET> saveDescSets;')
        ud_body.append('            while (pUpdateChain) {')
        ud_body.append('                switch(pUpdateChain->sType)')
        ud_body.append('                {')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:')
        ud_body.append('                        for (uint32_t i = 0; i < ((XGL_UPDATE_SAMPLERS*)pUpdateChain)->count; i++) {')
        ud_body.append('                            XGL_SAMPLER* pLocalSampler = (XGL_SAMPLER*) &((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i];')
        ud_body.append('                            saveSamplers.push(*pLocalSampler);')
        ud_body.append('                            *pLocalSampler = remap(((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i]);')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:')
        ud_body.append('                    {')
        ud_body.append('                        XGL_UPDATE_SAMPLER_TEXTURES *pUST = (XGL_UPDATE_SAMPLER_TEXTURES *) pUpdateChain;')
        ud_body.append('                        for (uint32_t i = 0; i < pUST->count; i++) {')
        ud_body.append('                            XGL_SAMPLER *pLocalSampler = (XGL_SAMPLER *) &pUST->pSamplerImageViews[i].pSampler;')
        ud_body.append('                            saveSamplers.push(*pLocalSampler);')
        ud_body.append('                            *pLocalSampler = remap(pUST->pSamplerImageViews[i].pSampler);')
        ud_body.append('                            XGL_IMAGE_VIEW *pLocalView = (XGL_IMAGE_VIEW *) &pUST->pSamplerImageViews[i].pImageView->view;')
        ud_body.append('                            saveImageViews.push(*pLocalView);')
        ud_body.append('                            *pLocalView = remap(pUST->pSamplerImageViews[i].pImageView->view);')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    }')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:')
        ud_body.append('                    {')
        ud_body.append('                        XGL_UPDATE_IMAGES *pUI = (XGL_UPDATE_IMAGES*) pUpdateChain;')
        ud_body.append('                        for (uint32_t i = 0; i < pUI->count; i++) {')
        ud_body.append('                            XGL_IMAGE_VIEW* pLocalView = (XGL_IMAGE_VIEW*) &pUI->pImageViews[i]->view;')
        ud_body.append('                            saveImageViews.push(*pLocalView);')
        ud_body.append('                            *pLocalView = remap(pUI->pImageViews[i]->view);')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    }')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:')
        ud_body.append('                    {')
        ud_body.append('                        XGL_UPDATE_BUFFERS *pUB = (XGL_UPDATE_BUFFERS *) pUpdateChain;')
        ud_body.append('                        for (uint32_t i = 0; i < pUB->count; i++) {')
        ud_body.append('                            XGL_BUFFER_VIEW* pLocalView = (XGL_BUFFER_VIEW*) &pUB->pBufferViews[i]->view;')
        ud_body.append('                            saveBufferViews.push(*pLocalView);')
        ud_body.append('                            *pLocalView = remap(pUB->pBufferViews[i]->view);')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    }')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:')
        ud_body.append('                        saveDescSets.push(((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet);')
        ud_body.append('                        ((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet = remap(((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet);')
        ud_body.append('                        break;')
        ud_body.append('                    default:')
        ud_body.append('                        assert(0);')
        ud_body.append('                        break;')
        ud_body.append('                }')
        ud_body.append('                pUpdateChain = (XGL_UPDATE_SAMPLERS*) pUpdateChain->pNext;')
        ud_body.append('            }')
        ud_body.append('            m_xglFuncs.real_xglUpdateDescriptors(remap(pPacket->descriptorSet), pPacket->pUpdateChain);')
        ud_body.append('            pUpdateChain = (XGL_UPDATE_SAMPLERS*) pPacket->pUpdateChain;')
        ud_body.append('            while (pUpdateChain) {')
        ud_body.append('                switch(pUpdateChain->sType)')
        ud_body.append('                {')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:')
        ud_body.append('                        for (uint32_t i = 0; i < ((XGL_UPDATE_SAMPLERS*)pUpdateChain)->count; i++) {')
        ud_body.append('                            XGL_SAMPLER* pLocalSampler = (XGL_SAMPLER*) &((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i];')
        ud_body.append('                            *pLocalSampler = saveSamplers.front();')
        ud_body.append('                            saveSamplers.pop();')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:')
        ud_body.append('                    {')
        ud_body.append('                        XGL_UPDATE_SAMPLER_TEXTURES *pUST = (XGL_UPDATE_SAMPLER_TEXTURES *) pUpdateChain;')
        ud_body.append('                        for (uint32_t i = 0; i < pUST->count; i++) {')
        ud_body.append('                            XGL_SAMPLER *plocalSampler = (XGL_SAMPLER *) &pUST->pSamplerImageViews[i].pSampler;')
        ud_body.append('                            *plocalSampler = saveSamplers.front();')
        ud_body.append('                            saveSamplers.pop();')
        ud_body.append('                            XGL_IMAGE_VIEW *pLocalView = (XGL_IMAGE_VIEW *) &pUST->pSamplerImageViews[i].pImageView->view;')
        ud_body.append('                            *pLocalView = saveImageViews.front();')
        ud_body.append('                            saveImageViews.pop();')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    }')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:')
        ud_body.append('                    {')
        ud_body.append('                        XGL_UPDATE_IMAGES *pUI = (XGL_UPDATE_IMAGES*) pUpdateChain;')
        ud_body.append('                        for (uint32_t i = 0; i < pUI->count; i++) {')
        ud_body.append('                            XGL_IMAGE_VIEW* pLocalView = (XGL_IMAGE_VIEW*) &pUI->pImageViews[i]->view;')
        ud_body.append('                            *pLocalView = saveImageViews.front();')
        ud_body.append('                            saveImageViews.pop();')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    }')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:')
        ud_body.append('                    {')
        ud_body.append('                        XGL_UPDATE_BUFFERS *pUB = (XGL_UPDATE_BUFFERS *) pUpdateChain;')
        ud_body.append('                        for (uint32_t i = 0; i < pUB->count; i++) {')
        ud_body.append('                            XGL_BUFFER_VIEW* pLocalView = (XGL_BUFFER_VIEW*) &pUB->pBufferViews[i]->view;')
        ud_body.append('                            *pLocalView = saveBufferViews.front();')
        ud_body.append('                            saveBufferViews.pop();')
        ud_body.append('                        }')
        ud_body.append('                        break;')
        ud_body.append('                    }')
        ud_body.append('                    case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:')
        ud_body.append('                        ((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet = saveDescSets.front();')
        ud_body.append('                        saveDescSets.pop();')
        ud_body.append('                        //pFreeMe = (XGL_UPDATE_SAMPLERS*)pLocalUpdateChain;')
        ud_body.append('                        //pLocalUpdateChain = (void*)((XGL_UPDATE_SAMPLERS*)pLocalUpdateChain)->pNext;')
        ud_body.append('                        //free(pFreeMe);')
        ud_body.append('                        break;')
        ud_body.append('                    default:')
        ud_body.append('                        assert(0);')
        ud_body.append('                        break;')
        ud_body.append('                }')
        ud_body.append('                pUpdateChain = (XGL_UPDATE_SAMPLERS*) pUpdateChain->pNext;')
        ud_body.append('            }')
        return "\n".join(ud_body)

    def _gen_replay_create_descriptor_set_layout(self):
        cdsl_body = []
        cdsl_body.append('            XGL_SAMPLER saveSampler;')
        cdsl_body.append('            if (pPacket->pSetLayoutInfoList != NULL) {')
        cdsl_body.append('                XGL_SAMPLER *pSampler = (XGL_SAMPLER *) &pPacket->pSetLayoutInfoList->immutableSampler;')
        cdsl_body.append('                saveSampler = pPacket->pSetLayoutInfoList->immutableSampler;')
        cdsl_body.append('                *pSampler = remap(saveSampler);')
        cdsl_body.append('            }')
        cdsl_body.append('            XGL_DESCRIPTOR_SET_LAYOUT setLayout;')
        cdsl_body.append('            replayResult = m_xglFuncs.real_xglCreateDescriptorSetLayout(remap(pPacket->device), pPacket->stageFlags, pPacket->pSetBindPoints, remap(pPacket->priorSetLayout), pPacket->pSetLayoutInfoList, &setLayout);')
        cdsl_body.append('            if (replayResult == XGL_SUCCESS)')
        cdsl_body.append('            {')
        cdsl_body.append('                add_to_map(pPacket->pSetLayout, &setLayout);')
        cdsl_body.append('            }')
        cdsl_body.append('            if (pPacket->pSetLayoutInfoList != NULL) {')
        cdsl_body.append('                XGL_SAMPLER *pSampler = (XGL_SAMPLER *) &pPacket->pSetLayoutInfoList->immutableSampler;')
        cdsl_body.append('                *pSampler = saveSampler;')
        cdsl_body.append('            }')
        return "\n".join(cdsl_body)

    def _gen_replay_create_graphics_pipeline(self):
        cgp_body = []
        cgp_body.append('            XGL_GRAPHICS_PIPELINE_CREATE_INFO createInfo;')
        cgp_body.append('            struct shaderPair saveShader[10];')
        cgp_body.append('            unsigned int idx = 0;')
        cgp_body.append('            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));')
        cgp_body.append('            createInfo.lastSetLayout = remap(createInfo.lastSetLayout);')
        cgp_body.append('            // Cast to shader type, as those are of primariy interest and all structs in LL have same header w/ sType & pNext')
        cgp_body.append('            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;')
        cgp_body.append('            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)createInfo.pNext;')
        cgp_body.append('            while (XGL_NULL_HANDLE != pPacketNext)')
        cgp_body.append('            {')
        cgp_body.append('                if (XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO == pNext->sType)')
        cgp_body.append('                {')
        cgp_body.append('                    saveShader[idx].val = pNext->shader.shader;')
        cgp_body.append('                    saveShader[idx++].addr = &(pNext->shader.shader);')
        cgp_body.append('                    pNext->shader.shader = remap(pPacketNext->shader.shader);')
        cgp_body.append('                }')
        cgp_body.append('                pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacketNext->pNext;')
        cgp_body.append('                pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;')
        cgp_body.append('            }')
        cgp_body.append('            XGL_PIPELINE pipeline;')
        cgp_body.append('            replayResult = m_xglFuncs.real_xglCreateGraphicsPipeline(remap(pPacket->device), &createInfo, &pipeline);')
        cgp_body.append('            if (replayResult == XGL_SUCCESS)')
        cgp_body.append('            {')
        cgp_body.append('                add_to_map(pPacket->pPipeline, &pipeline);')
        cgp_body.append('            }')
        cgp_body.append('            for (unsigned int i = 0; i < idx; i++)')
        cgp_body.append('                *(saveShader[i].addr) = saveShader[i].val;')
        return "\n".join(cgp_body)

    def _gen_replay_cmd_wait_events(self):
        cwe_body = []
        cwe_body.append('            XGL_EVENT saveEvent[100];')
        cwe_body.append('            uint32_t idx, numRemapBuf=0, numRemapImg=0;')
        cwe_body.append('            assert(pPacket->pWaitInfo && pPacket->pWaitInfo->eventCount <= 100);')
        cwe_body.append('            for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++)')
        cwe_body.append('            {')
        cwe_body.append('                XGL_EVENT *pEvent = (XGL_EVENT *) &(pPacket->pWaitInfo->pEvents[idx]);')
        cwe_body.append('                saveEvent[idx] = pPacket->pWaitInfo->pEvents[idx];')
        cwe_body.append('                *pEvent = remap(pPacket->pWaitInfo->pEvents[idx]);')
        cwe_body.append('            }')
        cwe_body.append('')
        cwe_body.append('            XGL_BUFFER saveBuf[100];')
        cwe_body.append('            XGL_IMAGE saveImg[100];')
        cwe_body.append('            for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++)')
        cwe_body.append('            {')
        cwe_body.append('                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];')
        cwe_body.append('                assert(pNext);')
        cwe_body.append('                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {')
        cwe_body.append('                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];')
        cwe_body.append('                    assert(numRemapBuf < 100);')
        cwe_body.append('                    saveBuf[numRemapBuf++] = pNextBuf->buffer;')
        cwe_body.append('                    pNextBuf->buffer = remap(pNextBuf->buffer);')
        cwe_body.append('                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {')
        cwe_body.append('                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];')
        cwe_body.append('                    assert(numRemapImg < 100);')
        cwe_body.append('                    saveImg[numRemapImg++] = pNextImg->image;')
        cwe_body.append('                    pNextImg->image = remap(pNextImg->image);')
        cwe_body.append('                }')
        cwe_body.append('            }')
        cwe_body.append('            m_xglFuncs.real_xglCmdWaitEvents(remap(pPacket->cmdBuffer), pPacket->pWaitInfo);')
        cwe_body.append('            for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++) {')
        cwe_body.append('                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];')
        cwe_body.append('                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {')
        cwe_body.append('                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];')
        cwe_body.append('                    pNextBuf->buffer = saveBuf[idx];')
        cwe_body.append('                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {')
        cwe_body.append('                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];')
        cwe_body.append('                    pNextImg->image = saveImg[idx];')
        cwe_body.append('                }')
        cwe_body.append('            }')
        cwe_body.append('            for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++) {')
        cwe_body.append('                XGL_EVENT *pEvent = (XGL_EVENT *) &(pPacket->pWaitInfo->pEvents[idx]);')
        cwe_body.append('                *pEvent = saveEvent[idx];')
        cwe_body.append('            }')
        return "\n".join(cwe_body)

    def _gen_replay_cmd_pipeline_barrier(self):
        cpb_body = []
        cpb_body.append('            uint32_t idx, numRemapBuf=0, numRemapImg=0;')
        cpb_body.append('            XGL_BUFFER saveBuf[100];')
        cpb_body.append('            XGL_IMAGE saveImg[100];')
        cpb_body.append('            for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++)')
        cpb_body.append('            {')
        cpb_body.append('                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];')
        cpb_body.append('                assert(pNext);')
        cpb_body.append('                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {')
        cpb_body.append('                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];')
        cpb_body.append('                    assert(numRemapBuf < 100);')
        cpb_body.append('                    saveBuf[numRemapBuf++] = pNextBuf->buffer;')
        cpb_body.append('                    pNextBuf->buffer = remap(pNextBuf->buffer);')
        cpb_body.append('                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {')
        cpb_body.append('                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];')
        cpb_body.append('                    assert(numRemapImg < 100);')
        cpb_body.append('                    saveImg[numRemapImg++] = pNextImg->image;')
        cpb_body.append('                    pNextImg->image = remap(pNextImg->image);')
        cpb_body.append('                }')
        cpb_body.append('            }')
        cpb_body.append('            m_xglFuncs.real_xglCmdPipelineBarrier(remap(pPacket->cmdBuffer), pPacket->pBarrier);')
        cpb_body.append('            for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++) {')
        cpb_body.append('                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];')
        cpb_body.append('                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {')
        cpb_body.append('                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];')
        cpb_body.append('                    pNextBuf->buffer = saveBuf[idx];')
        cpb_body.append('                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {')
        cpb_body.append('                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];')
        cpb_body.append('                    pNextImg->image = saveImg[idx];')
        cpb_body.append('                }')
        cpb_body.append('            }')
        return "\n".join(cpb_body)

    def _gen_replay_create_framebuffer(self):
        cf_body = []
        cf_body.append('            XGL_FRAMEBUFFER_CREATE_INFO *pInfo = (XGL_FRAMEBUFFER_CREATE_INFO *) pPacket->pCreateInfo;')
        cf_body.append('            XGL_COLOR_ATTACHMENT_BIND_INFO *pColorAttachments, *pSavedColor = (XGL_COLOR_ATTACHMENT_BIND_INFO*)pInfo->pColorAttachments;')
        cf_body.append('            bool allocatedColorAttachments = false;')
        cf_body.append('            if (pSavedColor != NULL)')
        cf_body.append('            {')
        cf_body.append('                allocatedColorAttachments = true;')
        cf_body.append('                pColorAttachments = GLV_NEW_ARRAY(XGL_COLOR_ATTACHMENT_BIND_INFO, pInfo->colorAttachmentCount);')
        cf_body.append('                memcpy(pColorAttachments, pSavedColor, sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO) * pInfo->colorAttachmentCount);')
        cf_body.append('                for (uint32_t i = 0; i < pInfo->colorAttachmentCount; i++)')
        cf_body.append('                {')
        cf_body.append('                    pColorAttachments[i].view = remap(pInfo->pColorAttachments[i].view);')
        cf_body.append('                }')
        cf_body.append('                pInfo->pColorAttachments = pColorAttachments;')
        cf_body.append('            }')
        cf_body.append('            // remap depth stencil target')
        cf_body.append('            const XGL_DEPTH_STENCIL_BIND_INFO *pSavedDS = pInfo->pDepthStencilAttachment;')
        cf_body.append('            XGL_DEPTH_STENCIL_BIND_INFO depthTarget;')
        cf_body.append('            if (pSavedDS != NULL)')
        cf_body.append('            {')
        cf_body.append('                memcpy(&depthTarget, pSavedDS, sizeof(XGL_DEPTH_STENCIL_BIND_INFO));')
        cf_body.append('                depthTarget.view = remap(pSavedDS->view);')
        cf_body.append('                pInfo->pDepthStencilAttachment = &depthTarget;')
        cf_body.append('            }')
        cf_body.append('            XGL_FRAMEBUFFER local_framebuffer;')
        cf_body.append('            replayResult = m_xglFuncs.real_xglCreateFramebuffer(remap(pPacket->device), pPacket->pCreateInfo, &local_framebuffer);')
        cf_body.append('            pInfo->pColorAttachments = pSavedColor;')
        cf_body.append('            pInfo->pDepthStencilAttachment = pSavedDS;')
        cf_body.append('            if (replayResult == XGL_SUCCESS)')
        cf_body.append('            {')
        cf_body.append('                add_to_map(pPacket->pFramebuffer, &local_framebuffer);')
        cf_body.append('            }')
        cf_body.append('            if (allocatedColorAttachments)')
        cf_body.append('            {')
        cf_body.append('                GLV_DELETE((void*)pColorAttachments);')
        cf_body.append('            }')
        return "\n".join(cf_body)

    def _gen_replay_create_renderpass(self):
        cr_body = []
        cr_body.append('            XGL_RENDER_PASS_CREATE_INFO *pInfo = (XGL_RENDER_PASS_CREATE_INFO *) pPacket->pCreateInfo;')
        cr_body.append('            // remap framebuffer')
        cr_body.append('            XGL_FRAMEBUFFER savedFB, *pFB = &(pInfo->framebuffer);')
        cr_body.append('            if (*pFB != NULL)')
        cr_body.append('            {')
        cr_body.append('                savedFB = pInfo->framebuffer;')
        cr_body.append('                *pFB = remap(pInfo->framebuffer);')
        cr_body.append('            }')
        cr_body.append('            XGL_RENDER_PASS local_renderpass;')
        cr_body.append('            replayResult = m_xglFuncs.real_xglCreateRenderPass(remap(pPacket->device), pPacket->pCreateInfo, &local_renderpass);')
        cr_body.append('            pInfo->framebuffer = savedFB;')
        cr_body.append('            if (replayResult == XGL_SUCCESS)')
        cr_body.append('            {')
        cr_body.append('                add_to_map(pPacket->pRenderPass, &local_renderpass);')
        cr_body.append('            }')
        return "\n".join(cr_body)

    def _gen_replay_begin_command_buffer(self):
        bcb_body = []
        bcb_body.append('            XGL_CMD_BUFFER_BEGIN_INFO* pInfo = (XGL_CMD_BUFFER_BEGIN_INFO*)pPacket->pBeginInfo;')
        bcb_body.append('            // assume only zero or one graphics_begin_info in the chain')
        bcb_body.append('            XGL_RENDER_PASS savedRP, *pRP;')
        bcb_body.append('            XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO *pGInfo = NULL;')
        bcb_body.append('            while (pInfo != NULL)')
        bcb_body.append('            {')
        bcb_body.append('')
        bcb_body.append('                if (pInfo->sType == XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO)')
        bcb_body.append('                {')
        bcb_body.append('                    pGInfo = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO *) pInfo;')
        bcb_body.append('                    savedRP = pGInfo->renderPass;')
        bcb_body.append('                    pRP = &(pGInfo->renderPass);')
        bcb_body.append('                    *pRP = remap(pGInfo->renderPass);')
        bcb_body.append('                    break;')
        bcb_body.append('                }')
        bcb_body.append('                pInfo = (XGL_CMD_BUFFER_BEGIN_INFO*) pInfo->pNext;')
        bcb_body.append('            }')
        bcb_body.append('            replayResult = m_xglFuncs.real_xglBeginCommandBuffer(remap(pPacket->cmdBuffer), pPacket->pBeginInfo);')
        bcb_body.append('            if (pGInfo != NULL)')
        bcb_body.append('                pGInfo->renderPass = savedRP;')
        return "\n".join(bcb_body)

    def _gen_replay_store_pipeline(self):
        sp_body = []
        sp_body.append('            size_t size = 0;')
        sp_body.append('            void* pData = NULL;')
        sp_body.append('            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)')
        sp_body.append('            {')
        sp_body.append('                size = *pPacket->pDataSize;')
        sp_body.append('                pData = glv_malloc(*pPacket->pDataSize);')
        sp_body.append('            }')
        sp_body.append('            replayResult = m_xglFuncs.real_xglStorePipeline(remap(pPacket->pipeline), &size, pData);')
        sp_body.append('            if (replayResult == XGL_SUCCESS)')
        sp_body.append('            {')
        sp_body.append('                if (size != *pPacket->pDataSize && pData != NULL)')
        sp_body.append('                {')
        sp_body.append('                    glv_LogWarn("xglStorePipeline returned a differing data size: replay (%d bytes) vs trace (%d bytes)\\n", size, *pPacket->pDataSize);')
        sp_body.append('                }')
        sp_body.append('                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)')
        sp_body.append('                {')
        sp_body.append('                    glv_LogWarn("xglStorePipeline returned differing data contents than the trace file contained.\\n");')
        sp_body.append('                }')
        sp_body.append('            }')
        sp_body.append('            glv_free(pData);')
        return "\n".join(sp_body)

    def _gen_replay_get_multi_gpu_compatibility(self):
        gmgc_body = []
        gmgc_body.append('            XGL_GPU_COMPATIBILITY_INFO cInfo;')
        gmgc_body.append('            XGL_PHYSICAL_GPU handle0, handle1;')
        gmgc_body.append('            handle0 = remap(pPacket->gpu0);')
        gmgc_body.append('            handle1 = remap(pPacket->gpu1);')
        gmgc_body.append('            replayResult = m_xglFuncs.real_xglGetMultiGpuCompatibility(handle0, handle1, &cInfo);')
        return "\n".join(gmgc_body)

    def _gen_replay_destroy_object(self):
        do_body = []
        do_body.append('            XGL_OBJECT object = remap(pPacket->object);')
        do_body.append('            if (object != XGL_NULL_HANDLE)')
        do_body.append('                replayResult = m_xglFuncs.real_xglDestroyObject(object);')
        do_body.append('            if (replayResult == XGL_SUCCESS)')
        do_body.append('                rm_from_map(pPacket->object);')
        return "\n".join(do_body)

    def _gen_replay_wait_for_fences(self):
        wf_body = []
        wf_body.append('            XGL_FENCE *pFence = GLV_NEW_ARRAY(XGL_FENCE, pPacket->fenceCount);')
        wf_body.append('            for (uint32_t i = 0; i < pPacket->fenceCount; i++)')
        wf_body.append('            {')
        wf_body.append('                *(pFence + i) = remap(*(pPacket->pFences + i));')
        wf_body.append('            }')
        wf_body.append('            replayResult = m_xglFuncs.real_xglWaitForFences(remap(pPacket->device), pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);')
        wf_body.append('            GLV_DELETE(pFence);')
        return "\n".join(wf_body)

    def _gen_replay_wsi_associate_connection(self):
        wac_body = []
        wac_body.append('#ifdef WIN32')
        wac_body.append('            //TBD')
        wac_body.append('            replayResult = XGL_SUCCESS;')
        wac_body.append('#else')
        wac_body.append('            //associate with the replayers Wsi connection rather than tracers')
        wac_body.append('            replayResult = m_xglFuncs.real_xglWsiX11AssociateConnection(remap(pPacket->gpu), &(m_display->m_WsiConnection));')
        wac_body.append('#endif')
        return "\n".join(wac_body)

    def _gen_replay_wsi_get_msc(self):
        wgm_body = []
        wgm_body.append('#ifdef WIN32')
        wgm_body.append('            //TBD')
        wgm_body.append('            replayResult = XGL_SUCCESS;')
        wgm_body.append('#else')
        wgm_body.append('            xcb_window_t window = m_display->m_XcbWindow;')
        wgm_body.append('            replayResult = m_xglFuncs.real_xglWsiX11GetMSC(remap(pPacket->device), window, pPacket->crtc, pPacket->pMsc);')
        wgm_body.append('#endif')
        return "\n".join(wgm_body)

    def _gen_replay_wsi_create_presentable_image(self):
        cpi_body = []
        cpi_body.append('#ifdef WIN32')
        cpi_body.append('            //TBD')
        cpi_body.append('            replayResult = XGL_SUCCESS;')
        cpi_body.append('#else')
        cpi_body.append('            XGL_IMAGE img;')
        cpi_body.append('            XGL_GPU_MEMORY mem;')
        cpi_body.append('            m_display->imageHeight.push_back(pPacket->pCreateInfo->extent.height);')
        cpi_body.append('            m_display->imageWidth.push_back(pPacket->pCreateInfo->extent.width);')
        cpi_body.append('            replayResult = m_xglFuncs.real_xglWsiX11CreatePresentableImage(remap(pPacket->device), pPacket->pCreateInfo, &img, &mem);')
        cpi_body.append('            if (replayResult == XGL_SUCCESS)')
        cpi_body.append('            {')
        cpi_body.append('                if (pPacket->pImage != NULL)')
        cpi_body.append('                    add_to_map(pPacket->pImage, &img);')
        cpi_body.append('                if(pPacket->pMem != NULL)')
        cpi_body.append('                    add_to_map(pPacket->pMem, &mem);')
        cpi_body.append('                m_display->imageHandles.push_back(img);')
        cpi_body.append('                m_display->imageMemory.push_back(mem);')
        cpi_body.append('            }')
        cpi_body.append('#endif')
        return "\n".join(cpi_body)

    def _gen_replay_wsi_queue_present(self):
        wqp_body = []
        wqp_body.append('#ifdef WIN32')
        wqp_body.append('            //TBD')
        wqp_body.append('            replayResult = XGL_SUCCESS;')
        wqp_body.append('#else')
        wqp_body.append('            XGL_WSI_X11_PRESENT_INFO pInfo;')
        wqp_body.append('            std::vector<int>::iterator it;')
        wqp_body.append('            memcpy(&pInfo, pPacket->pPresentInfo, sizeof(XGL_WSI_X11_PRESENT_INFO));')
        wqp_body.append('            pInfo.srcImage = remap(pPacket->pPresentInfo->srcImage);')
        wqp_body.append('            // use replayers Xcb window')
        wqp_body.append('            pInfo.destWindow = m_display->m_XcbWindow;')
        wqp_body.append('            replayResult = m_xglFuncs.real_xglWsiX11QueuePresent(remap(pPacket->queue), &pInfo, remap(pPacket->fence));')
        wqp_body.append('            it = std::find(m_screenshotFrames.begin(), m_screenshotFrames.end(), m_display->m_frameNumber);')
        wqp_body.append('            if (it != m_screenshotFrames.end())')
        wqp_body.append('            {')
        wqp_body.append('                for(unsigned int i=0; i<m_display->imageHandles.size(); i++)')
        wqp_body.append('                {')
        wqp_body.append('                    if (m_display->imageHandles[i] == pInfo.srcImage)')
        wqp_body.append('                    {')
        wqp_body.append('                        char frameName[32];')
        wqp_body.append('                        sprintf(frameName, "%d",m_display->m_frameNumber);')
        wqp_body.append('                        glvWritePPM(frameName, m_display->imageWidth[i], m_display->imageHeight[i],')
        wqp_body.append('                            m_display->imageHandles[i], m_display->imageMemory[i], &m_xglFuncs);')
        wqp_body.append('                        break;')
        wqp_body.append('                    }')
        wqp_body.append('                }')
        wqp_body.append('            }')
        wqp_body.append('#endif')
        wqp_body.append('            m_display->m_frameNumber++;')
        return "\n".join(wqp_body)

    # I don't like making these 3 mem functions 'fully' custom, but just doing it for now to avoid being too cute
    def _gen_replay_free_memory(self):
        fm_body = []
        fm_body.append('            XGL_GPU_MEMORY handle = remap(pPacket->mem);')
        fm_body.append('            replayResult = m_xglFuncs.real_xglFreeMemory(handle);')
        fm_body.append('            if (replayResult == XGL_SUCCESS) ')
        fm_body.append('            {')
        fm_body.append('                rm_entry_from_mapData(handle);')
        fm_body.append('                rm_from_map(pPacket->mem);')
        fm_body.append('            }')
        return "\n".join(fm_body)

    def _gen_replay_map_memory(self):
        mm_body = []
        mm_body.append('            XGL_GPU_MEMORY handle = remap(pPacket->mem);')
        mm_body.append('            void* pData;')
        mm_body.append('            replayResult = m_xglFuncs.real_xglMapMemory(handle, pPacket->flags, &pData);')
        mm_body.append('            if (replayResult == XGL_SUCCESS)')
        mm_body.append('                add_mapping_to_mapData(handle, pData);')
        return "\n".join(mm_body)
        
    def _gen_replay_unmap_memory(self):
        um_body = []
        um_body.append('            XGL_GPU_MEMORY handle = remap(pPacket->mem);')
        um_body.append('            rm_mapping_from_mapData(handle, pPacket->pData);  // copies data from packet into memory buffer')
        um_body.append('            replayResult = m_xglFuncs.real_xglUnmapMemory(handle);')
        return "\n".join(um_body)

    def _gen_replay_bind_dynamic_memory_view(self):
        bdmv_body = []
        bdmv_body.append('            XGL_MEMORY_VIEW_ATTACH_INFO memView;')
        bdmv_body.append('            memcpy(&memView, pPacket->pMemView, sizeof(XGL_MEMORY_VIEW_ATTACH_INFO));')
        bdmv_body.append('            memView.mem = remap(pPacket->pMemView->mem);')
        bdmv_body.append('            m_xglFuncs.real_xglCmdBindDynamicMemoryView(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, &memView);')
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
        custom_check_ret_val = ['EnumerateGpus', 'GetGpuInfo', 'CreateDevice', 'GetExtensionSupport']
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
        rbody.append('#define CHECK_RETURN_VALUE(entrypoint) returnValue = handle_replay_errors(#entrypoint, replayResult, pPacket->result, returnValue);\n')
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
                    rbody.append('            handle = remap(pPacket->device);')
                elif create_view:
                    rbody.append('            %s createInfo;' % (proto.params[1].ty.strip('*').replace('const ', '')))
                    rbody.append('            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(%s));' % (proto.params[1].ty.strip('*').replace('const ', '')))
                    if 'CreateComputePipeline' == proto.name:
                        rbody.append('            createInfo.cs.shader = remap(pPacket->pCreateInfo->cs.shader);')
                    elif 'CreateBufferView' == proto.name:
                        rbody.append('            createInfo.buffer = remap(pPacket->pCreateInfo->buffer);')
                    else:
                        rbody.append('            createInfo.image = remap(pPacket->pCreateInfo->image);')
                    rbody.append('            %s local_%s;' % (proto.params[-1].ty.strip('*').replace('const ', ''), proto.params[-1].name))
                elif create_func: # Declare local var to store created handle into
                    rbody.append('            %s local_%s;' % (proto.params[-1].ty.strip('*').replace('const ', ''), proto.params[-1].name))
                    if 'AllocDescriptorSets' == proto.name:
                        rbody.append('            %s local_%s[100];' % (proto.params[-2].ty.strip('*').replace('const ', ''), proto.params[-2].name))
                        rbody.append('            XGL_DESCRIPTOR_SET_LAYOUT localDescSets[100];')
                        rbody.append('            assert(pPacket->count <= 100);')
                        rbody.append('            for (uint32_t i = 0; i < pPacket->count; i++)')
                        rbody.append('            {')
                        rbody.append('                localDescSets[i] = remap(pPacket->%s[i]);' % (proto.params[-3].name))
                        rbody.append('            }')
                elif proto.name == 'ClearDescriptorSets':
                    rbody.append('            XGL_DESCRIPTOR_SET localDescSets[100];')
                    rbody.append('            assert(pPacket->count <= 100);')
                    rbody.append('            for (uint32_t i = 0; i < pPacket->count; i++)')
                    rbody.append('            {')
                    rbody.append('                localDescSets[i] = remap(pPacket->%s[i]);' % (proto.params[-1].name))
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
                    rr_list[0] = rr_list[0].replace('remap(pPacket->device)', 'handle')
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
                    rbody.append('                rm_from_map(pPacket->device);')
                    rbody.append('                m_display->m_initedXGL = false;')
                    rbody.append('            }')
                if 'DestroyInstance' in proto.name:
                    rbody.append('            if (replayResult == XGL_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                // TODO need to handle multiple instances and only clearing maps within an instance.')
                    rbody.append('                // TODO this only works with a single instance used at any given time.')
                    rbody.append('                clear_all_map_handles();')
                    rbody.append('            }')
                elif 'AllocDescriptorSets' in proto.name:
                    rbody.append('            if (replayResult == XGL_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                for (uint32_t i = 0; i < local_pCount; i++) {')
                    rbody.append('                    add_to_map(&pPacket->%s[i], &local_%s[i]);' % (proto.params[-2].name, proto.params[-2].name))
                    rbody.append('                }')
                    rbody.append('            }')
                elif create_func: # save handle mapping if create successful
                    rbody.append('            if (replayResult == XGL_SUCCESS)')
                    rbody.append('            {')
                    rbody.append('                add_to_map(pPacket->%s, &local_%s);' % (proto.params[-1].name, proto.params[-1].name))
                    if 'AllocMemory' == proto.name:
                        rbody.append('                add_entry_to_mapData(local_%s, pPacket->pAllocInfo->allocationSize);' % (proto.params[-1].name))
                    rbody.append('            }')
                elif proto.name in do_while_dict:
                    rbody[-1] = '    %s' % rbody[-1]
                    rbody.append('            } while (%s);' % do_while_dict[proto.name])
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
        header_txt.append('#include "glvtrace_xgl_xgl_structs.h"')
        header_txt.append('#include "glvtrace_xgl_packet_id.h"\n')
        header_txt.append('void AttachHooks();')
        header_txt.append('void DetachHooks();')
        header_txt.append('void InitTracer();\n')
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
        header_txt.append('#include "glvtrace_xgl_xgl_structs.h"')
        header_txt.append('#include "glvtrace_xgl_xgldbg_structs.h"')
        header_txt.append('#include "glvtrace_xgl_xglwsix11ext_structs.h"')
        header_txt.append('#include "xgl_enum_string_helper.h"')
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
        header_txt.append('#if defined(PLATFORM_LINUX)')
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
        header_txt.append('#include "glvtrace_xgl_xglwsix11ext_structs.h"')
        header_txt.append('#include "glvtrace_xgl_packet_id.h"')
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
        header_txt.append('#if defined(PLATFORM_LINUX)')
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
        header_txt.append('#include "glvtrace_xgl_xgldbg_structs.h"')
        header_txt.append('#include "glvtrace_xgl_packet_id.h"')
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

class GlaveReplayHeader(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include <set>')
        header_txt.append('#include <map>')
        header_txt.append('#include <vector>')
        header_txt.append('#include <string>')
        header_txt.append('#if defined(PLATFORM_LINUX)')
        header_txt.append('#include <xcb/xcb.h>\n')
        header_txt.append('#endif')
        header_txt.append('#include "glvreplay_window.h"')
        header_txt.append('#include "glvreplay_factory.h"')
        header_txt.append('#include "glv_trace_packet_identifiers.h"\n')
        header_txt.append('#include "xgl.h"')
        header_txt.append('#include "xglDbg.h"')
        header_txt.append('#if defined(PLATFORM_LINUX)')
        header_txt.append('#include "xglWsiX11Ext.h"')
        header_txt.append('#else')
        header_txt.append('#include "xglWsiWinExt.h"')
        header_txt.append('#endif')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_class_decls(),
                self._generate_replay_func_ptrs(),
                self._generate_replay_class()]

        return "\n".join(body)

class GlaveReplayC(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include "glvreplay_xgl_replay.h"\n')
        header_txt.append('#include "glvreplay_xgl.h"\n')
        header_txt.append('#include "glvreplay_xgl_write_ppm.h"\n')
        header_txt.append('#include "glvreplay_main.h"\n')
        header_txt.append('#include <algorithm>')
        header_txt.append('#include <queue>')
        header_txt.append('glvreplay_settings *g_pReplaySettings;')
        header_txt.append('extern "C" {')
        header_txt.append('#include "glvtrace_xgl_xgl_structs.h"')
        header_txt.append('#include "glvtrace_xgl_xgldbg_structs.h"')
        header_txt.append('#include "glvtrace_xgl_xglwsix11ext_structs.h"')
        header_txt.append('#include "glvtrace_xgl_packet_id.h"')
        header_txt.append('#include "xgl_enum_string_helper.h"\n}\n')
        header_txt.append('#define APP_NAME "glvreplay_xgl"')
        header_txt.append('#define IDI_ICON 101\n')
        header_txt.append('static const char* g_extensions[] =')
        header_txt.append('{')
        header_txt.append('        "XGL_WSI_WINDOWS",')
        header_txt.append('        "XGL_TIMER_QUEUE",')
        header_txt.append('        "XGL_GPU_TIMESTAMP_CALIBRATION",')
        header_txt.append('        "XGL_DMA_QUEUE",')
        header_txt.append('        "XGL_COMMAND_BUFFER_CONTROL_FLOW",')
        header_txt.append('        "XGL_COPY_OCCLUSION_QUERY_DATA",')
        header_txt.append('        "XGL_ADVANCED_MULTISAMPLING",')
        header_txt.append('        "XGL_BORDER_COLOR_PALETTE"')
        header_txt.append('};')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_display_init_xgl(),
                self._generate_replay_display_init(),
                self._generate_replay_display_structors(),
                self._generate_replay_display_window(),
                self._generate_replay_structors(),
                self._generate_replay_init(),
                self._generate_replay_errors(),
                self._generate_replay_validation_funcs(),
                self._generate_replay_init_funcs(),
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
            "glave-replay-h" : GlaveReplayHeader,
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
