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
# Authors:
#   Chia-I Wu <olv@lunarg.com>

import sys

import xgl

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.protos = ()
        self.headers = ()

    def run(self):
        self.protos = xgl.core + xgl.ext_wsi_x11
        self.headers = xgl.core_headers + xgl.ext_wsi_x11_headers
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

    # Return set of printf '%' qualifier and input to that qualifier
    def _get_printf_params(self, xgl_type, name, output_param):
        # TODO : Need ENUM and STRUCT checks here
        if "_TYPE" in xgl_type: # TODO : This should be generic ENUM check
            return ("%s", "string_%s(%s)" % (xgl_type.strip('const ').strip('*'), name))
        if "XGL_CHAR*" == xgl_type:
            return ("%s", name)
        if "UINT64" in xgl_type:
            if '*' in xgl_type:
                return ("%lu", "*%s" % name)
            return ("%lu", name)
        if "FLOAT" in xgl_type:
            if '[' in xgl_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                return ("[%f, %f, %f, %f]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name))
            return ("%f", name)
        if "BOOL" in xgl_type or 'xcb_randr_crtc_t' in xgl_type:
            return ("%u", name)
        if True in [t in xgl_type for t in ["INT", "SIZE", "FLAGS", "MASK", "xcb_window_t"]]:
            if '[' in xgl_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                return ("[%i, %i, %i, %i]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name))
            if '*' in xgl_type:
                return ("%i", "*%s" % name)
            return ("%i", name)
        # TODO : This is special-cased as there's only one "format" param currently and it's nice to expand it
        if "XGL_FORMAT" == xgl_type and "format" == name:
            return ("{format.channelFormat = %s, format.numericFormat = %s}", "string_XGL_CHANNEL_FORMAT(format.channelFormat), string_XGL_NUM_FORMAT(format.numericFormat)")
        if output_param:
            return ("%p", "(void*)*%s" % name)
        return ("%p", "(void*)%s" % name)

    def _generate_icd_dispatch_table(self):
        proto_map = {}
        for proto in self.protos:
            proto_map[proto.name] = proto

        entries = []
        for name in xgl.icd_dispatch_table:
            proto = proto_map[name]
            entries.append(proto.c_typedef(attr="XGLAPI"))

        return """struct icd_dispatch_table {
    %s;
};""" % ";\n    ".join(entries)

    def _generate_dispatch_entrypoints(self, qual="", layer="Generic"):
        if qual:
            qual += " "

        layer_name = layer
        funcs = []
        for proto in self.protos:
            if proto.name != "GetProcAddr" and proto.name != "InitAndEnumerateGpus":
                if "Generic" == layer:
                    decl = proto.c_func(prefix="xgl", attr="XGLAPI")
                    param0_name = proto.params[0].name
                    ret_val = ''
                    stmt = ''
                    if proto.ret != "XGL_VOID":
                        ret_val = "XGL_RESULT result = "
                        stmt = "    return result;\n"
                    if proto.name == "EnumerateLayers":
                        c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    if (gpu != NULL) {\n'
                                 '        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                                 '        printf("At start of layered %s\\n");\n'
                                 '        pCurObj = gpuw;\n'
                                 '        pthread_once(&tabOnce, initLayerTable);\n'
                                 '        %snextTable.%s;\n'
                                 '        printf("Completed layered %s\\n");\n'
                                 '        fflush(stdout);\n'
                                 '    %s'
                                 '    } else {\n'
                                 '        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)\n'
                                 '            return XGL_ERROR_INVALID_POINTER;\n'
                                 '        // This layer compatible with all GPUs\n'
                                 '        *pOutLayerCount = 1;\n'
                                 '        strncpy(pOutLayers[0], "%s", maxStringSize);\n'
                                 '        return XGL_SUCCESS;\n'
                                 '    }\n'
                                     '}' % (qual, decl, proto.params[0].name, proto.name, ret_val, c_call, proto.name, stmt, layer))
                    elif proto.params[0].ty != "XGL_PHYSICAL_GPU":
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    %snextTable.%s;\n'
                                 '%s'
                                 '}' % (qual, decl, ret_val, proto.c_call(), stmt))
                    else:
                        c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                                 '    printf("At start of layered %s\\n");\n'
                                 '    pCurObj = gpuw;\n'
                                 '    pthread_once(&tabOnce, initLayerTable);\n'
                                 '    %snextTable.%s;\n'
                                 '    printf("Completed layered %s\\n");\n'
                                 '    fflush(stdout);\n'
                                 '%s'
                                 '}' % (qual, decl, proto.params[0].name, proto.name, ret_val, c_call, proto.name, stmt))
                elif "APIDump" in layer:
                    decl = proto.c_func(prefix="xgl", attr="XGLAPI")
                    param0_name = proto.params[0].name
                    ret_val = ''
                    stmt = ''
                    cis_param_index = [] # Store list of indices when func has struct params
                    create_params = 0 # Num of params at end of function that are created and returned as output values
                    if 'WsiX11CreatePresentableImage' in proto.name:
                        create_params = -2
                    elif 'Create' in proto.name or 'Alloc' in proto.name or 'MapMemory' in proto.name:
                        create_params = -1
                    if proto.ret != "XGL_VOID":
                        ret_val = "XGL_RESULT result = "
                        stmt = "    return result;\n"
                    f_open = ''
                    f_close = ''
                    if "File" in layer:
                        file_mode = "a"
                        if 'CreateDevice' in proto.name:
                            file_mode = "w"
                        f_open = 'unsigned int tid = pthread_self();\n    pthread_mutex_lock( &file_lock );\n    pOutFile = fopen(outFileName, "%s");\n    ' % (file_mode)
                        log_func = 'fprintf(pOutFile, "t{%%u} xgl%s(' % proto.name
                        f_close = '\n    fclose(pOutFile);\n    pthread_mutex_unlock( &file_lock );'
                    else:
                        f_open = 'unsigned int tid = pthread_self();\n    pthread_mutex_lock( &print_lock );\n    '
                        log_func = 'printf("t{%%u} xgl%s(' % proto.name
                        f_close = '\n    pthread_mutex_unlock( &print_lock );'
                    print_vals = ', getTIDIndex()'
                    pindex = 0
                    for p in proto.params:
                        # TODO : Need to handle xglWsiX11CreatePresentableImage for which the last 2 params are returned vals
                        cp = False
                        if 0 != create_params:
                            # If this is any of the N last params of the func, treat as output
                            for y in range(-1, create_params-1, -1):
                                if p.name == proto.params[y].name:
                                    cp = True
                        (pft, pfi) = self._get_printf_params(p.ty, p.name, cp)
                        log_func += '%s = %s, ' % (p.name, pft)
                        print_vals += ', %s' % (pfi)
                        # TODO : Just want this to be simple check for params of STRUCT type
                        if "pCreateInfo" in p.name or ('const' in p.ty and '*' in p.ty and False not in [tmp_ty not in p.ty for tmp_ty in ['XGL_CHAR', 'XGL_VOID', 'XGL_CMD_BUFFER', 'XGL_QUEUE_SEMAPHORE', 'XGL_FENCE', 'XGL_SAMPLER', 'XGL_UINT32']]):
                            if 'Wsi' not in proto.name:
                                cis_param_index.append(pindex)
                        pindex += 1
                    log_func = log_func.strip(', ')
                    if proto.ret != "XGL_VOID":
                        log_func += ') = %s\\n"'
                        print_vals += ', string_XGL_RESULT(result)'
                    else:
                        log_func += ')\\n"'
                    log_func = '%s%s);' % (log_func, print_vals)
                    if len(cis_param_index) > 0:
                        log_func += '\n    char *pTmpStr;'
                        for sp_index in cis_param_index:
                            cis_print_func = 'xgl_print_%s' % (proto.params[sp_index].ty.strip('const ').strip('*').lower())
                            log_func += '\n    if (%s) {' % (proto.params[sp_index].name)
                            log_func += '\n        pTmpStr = %s(%s, "    ");' % (cis_print_func, proto.params[sp_index].name)
                            if "file" in layer:
                                log_func += '\n        fprintf(pOutFile, "   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                            else:
                                log_func += '\n        printf("   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                                log_func += '\n        fflush(stdout);'
                            log_func += '\n        free(pTmpStr);\n    }'
                    if proto.name == "EnumerateLayers":
                        c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    if (gpu != NULL) {\n'
                                 '        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                                 '        pCurObj = gpuw;\n'
                                 '        pthread_once(&tabOnce, initLayerTable);\n'
                                 '        %snextTable.%s;\n'
                                 '        %s    %s    %s\n'
                                 '    %s'
                                 '    } else {\n'
                                 '        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)\n'
                                 '            return XGL_ERROR_INVALID_POINTER;\n'
                                 '        // This layer compatible with all GPUs\n'
                                 '        *pOutLayerCount = 1;\n'
                                 '        strncpy(pOutLayers[0], "%s", maxStringSize);\n'
                                 '        return XGL_SUCCESS;\n'
                                 '    }\n'
                                     '}' % (qual, decl, proto.params[0].name, ret_val, c_call,f_open, log_func, f_close, stmt, layer))
                    elif proto.params[0].ty != "XGL_PHYSICAL_GPU":
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    %snextTable.%s;\n'
                                 '    %s%s%s\n'
                                 '%s'
                                 '}' % (qual, decl, ret_val, proto.c_call(), f_open, log_func, f_close, stmt))
                    else:
                        c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                                 '    pCurObj = gpuw;\n'
                                 '    pthread_once(&tabOnce, initLayerTable);\n'
                                 '    %snextTable.%s;\n'
                                 '    %s%s%s\n'
                                 '%s'
                                 '}' % (qual, decl, proto.params[0].name, ret_val, c_call, f_open, log_func, f_close, stmt))
                elif "ObjectTracker" == layer:
                    obj_type_mapping = {"XGL_PHYSICAL_GPU" : "XGL_OBJECT_TYPE_PHYSICAL_GPU", "XGL_DEVICE" : "XGL_OBJECT_TYPE_DEVICE",
                                        "XGL_QUEUE" : "XGL_OBJECT_TYPE_QUEUE", "XGL_QUEUE_SEMAPHORE" : "XGL_OBJECT_TYPE_QUEUE_SEMAPHORE",
                                        "XGL_GPU_MEMORY" : "XGL_OBJECT_TYPE_GPU_MEMORY", "XGL_FENCE" : "XGL_OBJECT_TYPE_FENCE",
                                        "XGL_QUERY_POOL" : "XGL_OBJECT_TYPE_QUERY_POOL", "XGL_EVENT" : "XGL_OBJECT_TYPE_EVENT",
                                        "XGL_IMAGE" : "XGL_OBJECT_TYPE_IMAGE", "XGL_DESCRIPTOR_SET" : "XGL_OBJECT_TYPE_DESCRIPTOR_SET",
                                        "XGL_CMD_BUFFER" : "XGL_OBJECT_TYPE_CMD_BUFFER", "XGL_SAMPLER" : "XGL_OBJECT_TYPE_SAMPLER",
                                        "XGL_PIPELINE" : "XGL_OBJECT_TYPE_PIPELINE", "XGL_PIPELINE_DELTA" : "XGL_OBJECT_TYPE_PIPELINE_DELTA",
                                        "XGL_SHADER" : "XGL_OBJECT_TYPE_SHADER", "XGL_IMAGE_VIEW" : "XGL_OBJECT_TYPE_IMAGE_VIEW",
                                        "XGL_COLOR_ATTACHMENT_VIEW" : "XGL_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW", "XGL_DEPTH_STENCIL_VIEW" : "XGL_OBJECT_TYPE_DEPTH_STENCIL_VIEW",
                                        "XGL_VIEWPORT_STATE_OBJECT" : "XGL_OBJECT_TYPE_VIEWPORT_STATE", "XGL_RASTER_STATE_OBJECT" : "XGL_OBJECT_TYPE_RASTER_STATE",
                                        "XGL_MSAA_STATE_OBJECT" : "XGL_OBJECT_TYPE_MSAA_STATE", "XGL_COLOR_BLEND_STATE_OBJECT" : "XGL_OBJECT_TYPE_COLOR_BLEND_STATE",
                                        "XGL_DEPTH_STENCIL_STATE_OBJECT" : "XGL_OBJECT_TYPE_DEPTH_STENCIL_STATE", "XGL_BASE_OBJECT" : "ll_get_obj_type(object)",
                                        "XGL_OBJECT" : "ll_get_obj_type(object)"}

                    decl = proto.c_func(prefix="xgl", attr="XGLAPI")
                    param0_name = proto.params[0].name
                    p0_type = proto.params[0].ty
                    create_line = ''
                    destroy_line = ''
                    if 'DbgRegisterMsgCallback' in proto.name:
                        using_line =  '    // This layer intercepts callbacks\n'
                        using_line += '    XGL_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (XGL_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(XGL_LAYER_DBG_FUNCTION_NODE));\n'
                        using_line += '    if (!pNewDbgFuncNode)\n'
                        using_line += '        return XGL_ERROR_OUT_OF_MEMORY;\n'
                        using_line += '    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;\n'
                        using_line += '    pNewDbgFuncNode->pUserData = pUserData;\n'
                        using_line += '    pNewDbgFuncNode->pNext = pDbgFunctionHead;\n'
                        using_line += '    pDbgFunctionHead = pNewDbgFuncNode;\n'
                    elif 'DbgUnregisterMsgCallback' in proto.name:
                        using_line =  '    XGL_LAYER_DBG_FUNCTION_NODE *pTrav = pDbgFunctionHead;\n'
                        using_line += '    XGL_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;\n'
                        using_line += '    while (pTrav) {\n'
                        using_line += '        if (pTrav->pfnMsgCallback == pfnMsgCallback) {\n'
                        using_line += '            pPrev->pNext = pTrav->pNext;\n'
                        using_line += '            if (pDbgFunctionHead == pTrav)\n'
                        using_line += '                pDbgFunctionHead = pTrav->pNext;\n'
                        using_line += '            free(pTrav);\n'
                        using_line += '            break;\n'
                        using_line += '        }\n'
                        using_line += '        pPrev = pTrav;\n'
                        using_line += '        pTrav = pTrav->pNext;\n'
                        using_line += '    }\n'
                    elif 'GlobalOption' in proto.name:
                        using_line = ''
                    else:
                        using_line = '    ll_increment_use_count((XGL_VOID*)%s, %s);\n' % (param0_name, obj_type_mapping[p0_type])
                    if 'Create' in proto.name or 'Alloc' in proto.name:
                        create_line = '    ll_insert_obj((XGL_VOID*)*%s, %s);\n' % (proto.params[-1].name, obj_type_mapping[proto.params[-1].ty.strip('*')])
                    if 'DestroyObject' in proto.name:
                        destroy_line = '    ll_destroy_obj((XGL_VOID*)%s);\n' % (param0_name)
                        using_line = ''
                    else:
                        if 'Destroy' in proto.name or 'Free' in proto.name:
                            destroy_line = '    ll_remove_obj_type((XGL_VOID*)%s, %s);\n' % (param0_name, obj_type_mapping[p0_type])
                            using_line = ''
                        if 'DestroyDevice' in proto.name:
                            destroy_line += '    // Report any remaining objects in LL\n    objNode *pTrav = pGlobalHead;\n    while (pTrav) {\n'
                            destroy_line += '        char str[1024];\n'
                            destroy_line += '        sprintf(str, "OBJ ERROR : %s object %p has not been destroyed (was used %lu times).", string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, pTrav->obj.numUses);\n'
                            destroy_line += '        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, device, 0, OBJTRACK_OBJECT_LEAK, "OBJTRACK", str);\n'
                            destroy_line += '        pTrav = pTrav->pNextGlobal;\n    }\n'
                    ret_val = ''
                    stmt = ''
                    if proto.ret != "XGL_VOID":
                        ret_val = "XGL_RESULT result = "
                        stmt = "    return result;\n"
                    if proto.name == "EnumerateLayers":
                        c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    if (gpu != NULL) {\n'
                                 '        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                                 '    %s'
                                 '        pCurObj = gpuw;\n'
                                 '        pthread_once(&tabOnce, initLayerTable);\n'
                                 '        %snextTable.%s;\n'
                                 '    %s%s'
                                 '    %s'
                                 '    } else {\n'
                                 '        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)\n'
                                 '            return XGL_ERROR_INVALID_POINTER;\n'
                                 '        // This layer compatible with all GPUs\n'
                                 '        *pOutLayerCount = 1;\n'
                                 '        strncpy(pOutLayers[0], "%s", maxStringSize);\n'
                                 '        return XGL_SUCCESS;\n'
                                 '    }\n'
                                     '}' % (qual, decl, proto.params[0].name, using_line, ret_val, c_call, create_line, destroy_line, stmt, layer))
                    elif proto.params[0].ty != "XGL_PHYSICAL_GPU":
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '%s'
                                 '    %snextTable.%s;\n'
                                 '%s%s'
                                 '%s'
                                 '}' % (qual, decl, using_line, ret_val, proto.c_call(), create_line, destroy_line, stmt))
                    else:
                        c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                                 '%s'
                                 '    pCurObj = gpuw;\n'
                                 '    pthread_once(&tabOnce, initLayerTable);\n'
                                 '    %snextTable.%s;\n'
                                 '%s%s'
                                 '%s'
                                 '}' % (qual, decl, proto.params[0].name, using_line, ret_val, c_call, create_line, destroy_line, stmt))

        # TODO : Put this code somewhere so it gets called at the end if objects not deleted :
        #     // Report any remaining objects in LL
        #     objNode *pTrav = pObjLLHead;
        #     while (pTrav) {
        #         printf("WARN : %s object %p has not been destroyed.\n", pTrav->objType, pTrav->pObj);
        #    }

        return "\n\n".join(funcs)

    def _generate_extensions(self):
        exts = []
        exts.append('XGL_UINT64 objTrackGetObjectCount(XGL_OBJECT_TYPE type)')
        exts.append('{')
        exts.append('    return (type == XGL_OBJECT_TYPE_ANY) ? numTotalObjs : numObjs[type];')
        exts.append('}')
        exts.append('')
        exts.append('XGL_RESULT objTrackGetObjects(XGL_OBJECT_TYPE type, XGL_UINT64 objCount, OBJTRACK_NODE* pObjNodeArray)')
        exts.append('{')
        exts.append("    // This bool flags if we're pulling all objs or just a single class of objs")
        exts.append('    XGL_BOOL bAllObjs = (type == XGL_OBJECT_TYPE_ANY);')
        exts.append('    // Check the count first thing')
        exts.append('    XGL_UINT64 maxObjCount = (bAllObjs) ? numTotalObjs : numObjs[type];')
        exts.append('    if (objCount > maxObjCount) {')
        exts.append('        char str[1024];')
        exts.append('        sprintf(str, "OBJ ERROR : Received objTrackGetObjects() request for %lu objs, but there are only %lu objs of type %s", objCount, maxObjCount, string_XGL_OBJECT_TYPE(type));')
        exts.append('        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_OBJCOUNT_MAX_EXCEEDED, "OBJTRACK", str);')
        exts.append('        return XGL_ERROR_INVALID_VALUE;')
        exts.append('    }')
        exts.append('    objNode* pTrav = (bAllObjs) ? pGlobalHead : pObjectHead[type];')
        exts.append('    for (XGL_UINT64 i = 0; i < objCount; i++) {')
        exts.append('        if (!pTrav) {')
        exts.append('            char str[1024];')
        exts.append('            sprintf(str, "OBJ INTERNAL ERROR : Ran out of %s objs! Should have %lu, but only copied %lu and not the requested %lu.", string_XGL_OBJECT_TYPE(type), maxObjCount, i, objCount);')
        exts.append('            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK", str);')
        exts.append('            return XGL_ERROR_UNKNOWN;')
        exts.append('        }')
        exts.append('        memcpy(&pObjNodeArray[i], pTrav, sizeof(OBJTRACK_NODE));')
        exts.append('        pTrav = (bAllObjs) ? pTrav->pNextGlobal : pTrav->pNextObj;')
        exts.append('    }')
        exts.append('    return XGL_SUCCESS;')
        exts.append('}')

        return "\n".join(exts)

    def _generate_layer_gpa_function(self, prefix="xgl", extensions=[]):
        func_body = []
        func_body.append("XGL_LAYER_EXPORT XGL_VOID* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* funcName)\n"
                         "{\n"
                         "    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;\n"
                         "    if (gpu == NULL)\n"
                         "        return NULL;\n"
                         "    pCurObj = gpuw;\n"
                         "    pthread_once(&tabOnce, initLayerTable);\n\n"
                         '    if (!strncmp("xglGetProcAddr", (const char *) funcName, sizeof("xglGetProcAddr")))\n'
                         '        return xglGetProcAddr;')
        if 0 != len(extensions):
            for ext_name in extensions:
                func_body.append('    else if (!strncmp("%s", (const char *) funcName, sizeof("%s")))\n'
                                 '        return %s;' % (ext_name, ext_name, ext_name))
        for name in xgl.icd_dispatch_table:
            if name == "GetProcAddr":
                continue
            if name == "InitAndEnumerateGpus":
                func_body.append('    else if (!strncmp("%s%s", (const char *) funcName, sizeof("%s%s")))\n'
                             '        return nextTable.%s;' % (prefix, name, prefix, name, name))
            else:
                func_body.append('    else if (!strncmp("%s%s", (const char *) funcName, sizeof("%s%s")))\n'
                             '        return %s%s;' % (prefix, name, prefix, name, prefix, name))

        func_body.append("    else {\n"
                         "        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;\n"
                         "        if (gpuw->pGPA == NULL)\n"
                         "            return NULL;\n"
                         "        return gpuw->pGPA(gpuw->nextObject, funcName);\n"
                         "    }\n"
                         "}\n")
        return "\n".join(func_body)

    def _generate_layer_dispatch_table(self, prefix='xgl'):
        func_body = []
        func_body.append('static void initLayerTable()\n'
                         '{\n'
                         '    GetProcAddrType fpNextGPA;\n'
                         '    fpNextGPA = pCurObj->pGPA;\n'
                         '    assert(fpNextGPA);\n');

        for name in xgl.icd_dispatch_table:
            func_body.append('    %sType fp%s = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "%s%s");\n'
                             '    nextTable.%s = fp%s;' % (name, name, prefix, name, name, name))

        func_body.append("}\n")
        return "\n".join(func_body)

class LayerFuncsSubcommand(Subcommand):
    def generate_header(self):
        return '#include <xglLayer.h>\n#include "loader.h"'

    def generate_body(self):
        return self._generate_dispatch_entrypoints("static", True)

class LayerDispatchSubcommand(Subcommand):
    def generate_header(self):
        return '#include "layer_wrappers.h"'

    def generate_body(self):
        return self._generate_layer_dispatch_table()

class GenericLayerSubcommand(Subcommand):
    def generate_header(self):
        return '#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n#include <pthread.h>\n#include "xglLayer.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;\nstatic pthread_once_t tabOnce = PTHREAD_ONCE_INIT;\n'

    def generate_body(self):
        body = [self._generate_layer_dispatch_table(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "Generic"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ApiDumpSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n#include <pthread.h>\n#include "xglLayer.h"\n#include "xgl_struct_string_helper.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;\nstatic pthread_once_t tabOnce = PTHREAD_ONCE_INIT;\npthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;\n')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static pthread_t tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    pthread_t tid = pthread_self();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t)maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_layer_dispatch_table(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "APIDump"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ApiDumpFileSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n#include <pthread.h>\n#include "xglLayer.h"\n#include "xgl_struct_string_helper.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;\nstatic pthread_once_t tabOnce = PTHREAD_ONCE_INIT;\n\nstatic FILE* pOutFile;\nstatic char* outFileName = "xgl_apidump.txt";\npthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;\n')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static pthread_t tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    pthread_t tid = pthread_self();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t)maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_layer_dispatch_table(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "APIDumpFile"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ObjectTrackerSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n#include <pthread.h>')
        header_txt.append('#include "object_track.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;')
        header_txt.append('static pthread_once_t tabOnce = PTHREAD_ONCE_INIT;\nstatic long long unsigned int object_track_index = 0;')
        header_txt.append('// Ptr to LL of dbg functions')
        header_txt.append('static XGL_LAYER_DBG_FUNCTION_NODE *pDbgFunctionHead = NULL;')
        header_txt.append('// Utility function to handle reporting')
        header_txt.append('//  If callbacks are enabled, use them, otherwise use printf')
        header_txt.append('static XGL_VOID layerCbMsg(XGL_DBG_MSG_TYPE msgType,')
        header_txt.append('    XGL_VALIDATION_LEVEL validationLevel,')
        header_txt.append('    XGL_BASE_OBJECT      srcObject,')
        header_txt.append('    XGL_SIZE             location,')
        header_txt.append('    XGL_INT              msgCode,')
        header_txt.append('    const XGL_CHAR*      pLayerPrefix,')
        header_txt.append('    const XGL_CHAR*      pMsg)')
        header_txt.append('{')
        header_txt.append('    XGL_LAYER_DBG_FUNCTION_NODE *pTrav = pDbgFunctionHead;')
        header_txt.append('    if (pTrav) {')
        header_txt.append('        while (pTrav) {')
        header_txt.append('            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);')
        header_txt.append('            pTrav = pTrav->pNext;')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('    else {')
        header_txt.append('        switch (msgType) {')
        header_txt.append('            case XGL_DBG_MSG_ERROR:')
        header_txt.append('                printf("{%s}ERROR : %s\\n", pLayerPrefix, pMsg);')
        header_txt.append('                break;')
        header_txt.append('            case XGL_DBG_MSG_WARNING:')
        header_txt.append('                printf("{%s}WARN : %s\\n", pLayerPrefix, pMsg);')
        header_txt.append('                break;')
        header_txt.append('            case XGL_DBG_MSG_PERF_WARNING:')
        header_txt.append('                printf("{%s}PERF_WARN : %s\\n", pLayerPrefix, pMsg);')
        header_txt.append('                break;')
        header_txt.append('            default:')
        header_txt.append('                printf("{%s}INFO : %s\\n", pLayerPrefix, pMsg);')
        header_txt.append('                break;')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('// We maintain a "Global" list which links every object and a')
        header_txt.append('//  per-Object list which just links objects of a given type')
        header_txt.append('// The object node has both pointers so the actual nodes are shared between the two lists')
        header_txt.append('typedef struct _objNode {')
        header_txt.append('    OBJTRACK_NODE   obj;')
        header_txt.append('    struct _objNode *pNextObj;')
        header_txt.append('    struct _objNode *pNextGlobal;')
        header_txt.append('} objNode;')
        header_txt.append('static objNode *pObjectHead[XGL_NUM_OBJECT_TYPE] = {0};')
        header_txt.append('static objNode *pGlobalHead = NULL;')
        header_txt.append('static uint64_t numObjs[XGL_NUM_OBJECT_TYPE] = {0};')
        header_txt.append('static uint64_t numTotalObjs = 0;')
        header_txt.append('// Debug function to print global list and each individual object list')
        header_txt.append('static void ll_print_lists()')
        header_txt.append('{')
        header_txt.append('    objNode* pTrav = pGlobalHead;')
        header_txt.append('    printf("=====GLOBAL OBJECT LIST (%lu total objs):\\n", numTotalObjs);')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        printf("   ObjNode (%p) w/ %s obj %p has pNextGlobal %p\\n", (void*)pTrav, string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, (void*)pTrav->pNextGlobal);')
        header_txt.append('        pTrav = pTrav->pNextGlobal;')
        header_txt.append('    }')
        header_txt.append('    for (uint32_t i = 0; i < XGL_NUM_OBJECT_TYPE; i++) {')
        header_txt.append('        pTrav = pObjectHead[i];')
        header_txt.append('        if (pTrav) {')
        header_txt.append('            printf("=====%s OBJECT LIST (%lu objs):\\n", string_XGL_OBJECT_TYPE(pTrav->obj.objType), numObjs[i]);')
        header_txt.append('            while (pTrav) {')
        header_txt.append('                printf("   ObjNode (%p) w/ %s obj %p has pNextObj %p\\n", (void*)pTrav, string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, (void*)pTrav->pNextObj);')
        header_txt.append('                pTrav = pTrav->pNextObj;')
        header_txt.append('            }')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('static void ll_insert_obj(XGL_VOID* pObj, XGL_OBJECT_TYPE objType) {')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "OBJ[%llu] : CREATE %s object %p", object_track_index++, string_XGL_OBJECT_TYPE(objType), (void*)pObj);')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('    objNode* pNewObjNode = (objNode*)malloc(sizeof(objNode));')
        header_txt.append('    pNewObjNode->obj.pObj = pObj;')
        header_txt.append('    pNewObjNode->obj.objType = objType;')
        header_txt.append('    pNewObjNode->obj.numUses = 0;')
        header_txt.append('    // insert at front of global list')
        header_txt.append('    pNewObjNode->pNextGlobal = pGlobalHead;')
        header_txt.append('    pGlobalHead = pNewObjNode;')
        header_txt.append('    // insert at front of object list')
        header_txt.append('    pNewObjNode->pNextObj = pObjectHead[objType];')
        header_txt.append('    pObjectHead[objType] = pNewObjNode;')
        header_txt.append('    // increment obj counts')
        header_txt.append('    numObjs[objType]++;')
        header_txt.append('    numTotalObjs++;')
        header_txt.append('    //sprintf(str, "OBJ_STAT : %lu total objs & %lu %s objs.", numTotalObjs, numObjs[objType], string_XGL_OBJECT_TYPE(objType));')
        header_txt.append('    //ll_print_lists();')
        header_txt.append('}')
        header_txt.append('// Traverse global list and return type for given object')
        header_txt.append('static XGL_OBJECT_TYPE ll_get_obj_type(XGL_OBJECT object) {')
        header_txt.append('    objNode *pTrav = pGlobalHead;')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == object)')
        header_txt.append('            return pTrav->obj.objType;')
        header_txt.append('        pTrav = pTrav->pNextGlobal;')
        header_txt.append('    }')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Attempting look-up on obj %p but it is NOT in the global list!", (void*)object);')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, OBJTRACK_MISSING_OBJECT, "OBJTRACK", str);')
        header_txt.append('    return XGL_OBJECT_TYPE_UNKNOWN;')
        header_txt.append('}')
        header_txt.append('static uint64_t ll_get_obj_uses(XGL_VOID* pObj, XGL_OBJECT_TYPE objType) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            return pTrav->obj.numUses;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    return 0;')
        header_txt.append('}')
        header_txt.append('static void ll_increment_use_count(XGL_VOID* pObj, XGL_OBJECT_TYPE objType) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            pTrav->obj.numUses++;')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJ[%llu] : USING %s object %p (%lu total uses)", object_track_index++, string_XGL_OBJECT_TYPE(objType), (void*)pObj, pTrav->obj.numUses);')
        header_txt.append('            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    // If we do not find obj, insert it and then increment count')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to increment count for obj %p, will add to list as %s type and increment count", pObj, string_XGL_OBJECT_TYPE(objType));')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('')
        header_txt.append('    ll_insert_obj(pObj, objType);')
        header_txt.append('    ll_increment_use_count(pObj, objType);')
        header_txt.append('}')
        header_txt.append('// We usually do not know Obj type when we destroy it so have to fetch')
        header_txt.append('//  Type from global list w/ ll_destroy_obj()')
        header_txt.append('//   and then do the full removal from both lists w/ ll_remove_obj_type()')
        header_txt.append('static void ll_remove_obj_type(XGL_VOID* pObj, XGL_OBJECT_TYPE objType) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    objNode *pPrev = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            pPrev->pNextObj = pTrav->pNextObj;')
        header_txt.append('            // update HEAD of Obj list as needed')
        header_txt.append('            if (pObjectHead[objType] == pTrav)')
        header_txt.append('                pObjectHead[objType] = pTrav->pNextObj;')
        header_txt.append('            assert(numObjs[objType] > 0);')
        header_txt.append('            numObjs[objType]--;')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJ[%llu] : DESTROY %s object %p", object_track_index++, string_XGL_OBJECT_TYPE(objType), (void*)pObj);')
        header_txt.append('            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pPrev = pTrav;')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "OBJ INTERNAL ERROR : Obj %p was in global list but not in %s list", pObj, string_XGL_OBJECT_TYPE(objType));')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('// Parse global list to find obj type, then remove obj from obj type list, finally')
        header_txt.append('//   remove obj from global list')
        header_txt.append('static void ll_destroy_obj(XGL_VOID* pObj) {')
        header_txt.append('    objNode *pTrav = pGlobalHead;')
        header_txt.append('    objNode *pPrev = pGlobalHead;')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            ll_remove_obj_type(pObj, pTrav->obj.objType);')
        header_txt.append('            pPrev->pNextGlobal = pTrav->pNextGlobal;')
        header_txt.append('            // update HEAD of global list if needed')
        header_txt.append('            if (pGlobalHead == pTrav)')
        header_txt.append('                pGlobalHead = pTrav->pNextGlobal;')
        header_txt.append('            free(pTrav);')
        header_txt.append('            assert(numTotalObjs > 0);')
        header_txt.append('            numTotalObjs--;')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJ_STAT Removed %s obj %p that was used %lu times (%lu total objs & %lu %s objs).", string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, pTrav->obj.numUses, numTotalObjs, numObjs[pTrav->obj.objType], string_XGL_OBJECT_TYPE(pTrav->obj.objType));')
        header_txt.append('            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pPrev = pTrav;')
        header_txt.append('        pTrav = pTrav->pNextGlobal;')
        header_txt.append('    }')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to remove obj %p. Was it created? Has it already been destroyed?", pObj);')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_DESTROY_OBJECT_FAILED, "OBJTRACK", str);')
        header_txt.append('}')

        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_layer_dispatch_table(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "ObjectTracker"),
                self._generate_extensions(),
                self._generate_layer_gpa_function(extensions=['objTrackGetObjectCount', 'objTrackGetObjects'])]

        return "\n\n".join(body)

def main():
    subcommands = {
            "layer-funcs" : LayerFuncsSubcommand,
            "layer-dispatch" : LayerDispatchSubcommand,
            "Generic" : GenericLayerSubcommand,
            "ApiDump" : ApiDumpSubcommand,
            "ApiDumpFile" : ApiDumpFileSubcommand,
            "ObjectTracker" : ObjectTrackerSubcommand,
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
