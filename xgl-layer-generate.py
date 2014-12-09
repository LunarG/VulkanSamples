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

    def _generate_dispatch_entrypoints(self, qual="", layer="Generic", no_addr=False):
        if qual:
            qual += " "

        layer_name = layer
        if no_addr:
            layer_name = "%sNoAddr" % layer
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
                                 '        strncpy((char *) pOutLayers[0], "%s", maxStringSize);\n'
                                 '        return XGL_SUCCESS;\n'
                                 '    }\n'
                                     '}' % (qual, decl, proto.params[0].name, proto.name, ret_val, c_call, proto.name, stmt, layer_name))
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
                        f_open = 'pthread_mutex_lock( &file_lock );\n    pOutFile = fopen(outFileName, "%s");\n    ' % (file_mode)
                        log_func = 'fprintf(pOutFile, "t{%%u} xgl%s(' % proto.name
                        f_close = '\n    fclose(pOutFile);\n    pthread_mutex_unlock( &file_lock );'
                    else:
                        f_open = 'pthread_mutex_lock( &print_lock );\n    '
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
                        if no_addr and "%p" == pft:
                            (pft, pfi) = ("%s", '"addr"')
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
                            if "File" in layer:
                                if no_addr:
                                    log_func += '\n        fprintf(pOutFile, "   %s (addr)\\n%%s\\n", pTmpStr);' % (proto.params[sp_index].name)
                                else:
                                    log_func += '\n        fprintf(pOutFile, "   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                            else:
                                if no_addr:
                                    log_func += '\n        printf("   %s (addr)\\n%%s\\n", pTmpStr);' % (proto.params[sp_index].name)
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
                                 '        strncpy((char *) pOutLayers[0], "%s", maxStringSize);\n'
                                 '        return XGL_SUCCESS;\n'
                                 '    }\n'
                                     '}' % (qual, decl, proto.params[0].name, ret_val, c_call,f_open, log_func, f_close, stmt, layer_name))
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
                                 '        strncpy((char *) pOutLayers[0], "%s", maxStringSize);\n'
                                 '        return XGL_SUCCESS;\n'
                                 '    }\n'
                                     '}' % (qual, decl, proto.params[0].name, using_line, ret_val, c_call, create_line, destroy_line, stmt, layer_name))
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
        hooks_txt.append('    if (real_xglInitAndEnumerateGpus != NULL)')
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
        hooks_txt.append('    if (real_xglInitAndEnumerateGpus == xglInitAndEnumerateGpus)')
        hooks_txt.append('        hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_xglInitAndEnumerateGpus,"xglInitAndEnumerateGpus");')
        hooks_txt.append('    isHooked = TRUE;')
        for proto in self.protos:
            if 'Dbg' not in proto.name and 'Wsi' not in proto.name and 'InitAndEnumerateGpus' not in proto.name:
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

    def _generate_init_tracer(self):
        init_tracer = []
        init_tracer.append('void InitTracer()\n{')
        init_tracer.append('    gMessageStream = glv_MessageStream_create(FALSE, "127.0.0.1", GLV_BASE_PORT + GLV_TID_XGL);')
        init_tracer.append('    glv_trace_set_trace_file(glv_FileLike_create_msg(gMessageStream));')
        init_tracer.append('//    glv_tracelog_set_log_file(glv_FileLike_create_file(fopen("glv_log_traceside.txt","w")));')
        init_tracer.append('    glv_tracelog_set_tracer_id(GLV_TID_XGL);\n}\n')
        return "\n".join(init_tracer)

    # InitAndEnumerateGpus is unique enough that it gets custom generation code
    def _gen_iande_gpus(self):
        iae_body = []
        iae_body.append('GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglInitAndEnumerateGpus(')
        iae_body.append('    const XGL_APPLICATION_INFO* pAppInfo,')
        iae_body.append('    const XGL_ALLOC_CALLBACKS* pAllocCb,')
        iae_body.append('    XGL_UINT maxGpus,')
        iae_body.append('    XGL_UINT* pGpuCount,')
        iae_body.append('    XGL_PHYSICAL_GPU* pGpus)')
        iae_body.append('{')
        iae_body.append('    glv_trace_packet_header* pHeader;')
        iae_body.append('    XGL_RESULT result;')
        iae_body.append('    uint64_t startTime;')
        iae_body.append('    struct_xglInitAndEnumerateGpus* pPacket;')
        iae_body.append('')
        iae_body.append('    glv_platform_thread_once(&gInitOnce, InitTracer);')
        iae_body.append('    SEND_ENTRYPOINT_ID(xglInitAndEnumerateGpus);')
        iae_body.append('    if (real_xglInitAndEnumerateGpus == xglInitAndEnumerateGpus)')
        iae_body.append('    {')
        iae_body.append('        glv_platform_get_next_lib_sym((void **) &real_xglInitAndEnumerateGpus,"xglInitAndEnumerateGpus");')
        iae_body.append('    }')
        iae_body.append('    startTime = glv_get_time();')
        iae_body.append('    result = real_xglInitAndEnumerateGpus(pAppInfo, pAllocCb, maxGpus, pGpuCount, pGpus);')
        iae_body.append('')
        iae_body.append('    // since we do not know how many gpus will be found must create trace packet after calling xglInit')
        iae_body.append('    CREATE_TRACE_PACKET(xglInitAndEnumerateGpus, calc_size_XGL_APPLICATION_INFO(pAppInfo) + ((pAllocCb == NULL) ? 0 :sizeof(XGL_ALLOC_CALLBACKS))')
        iae_body.append('        + sizeof(XGL_UINT) + ((pGpus && pGpuCount) ? *pGpuCount * sizeof(XGL_PHYSICAL_GPU) : 0));')
        iae_body.append('    pHeader->entrypoint_begin_time = startTime;')
        iae_body.append('    if (isHooked == FALSE) {')
        iae_body.append('        AttachHooks();')
        iae_body.append('        AttachHooks_xgldbg();')
        iae_body.append('        AttachHooks_xglwsix11ext();')
        iae_body.append('    }')
        iae_body.append('    pPacket = interpret_body_as_xglInitAndEnumerateGpus(pHeader);')
        iae_body.append('    add_XGL_APPLICATION_INFO_to_packet(pHeader, (XGL_APPLICATION_INFO**)&(pPacket->pAppInfo), pAppInfo);')
        iae_body.append('    if (pAllocCb) {')
        iae_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocCb), sizeof(XGL_ALLOC_CALLBACKS), pAllocCb);')
        iae_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocCb));')
        iae_body.append('    }')
        iae_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpuCount), sizeof(XGL_UINT), pGpuCount);')
        iae_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpuCount));')
        iae_body.append('    if (pGpuCount && pGpus)')
        iae_body.append('    {')
        iae_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpus), sizeof(XGL_PHYSICAL_GPU) * *pGpuCount, pGpus);')
        iae_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpus));')
        iae_body.append('    }')
        iae_body.append('    pPacket->maxGpus = maxGpus;')
        iae_body.append('    pPacket->result = result;')
        iae_body.append('    FINISH_TRACE_PACKET();')
        iae_body.append('    return result;')
        iae_body.append('}\n')
        return "\n".join(iae_body)

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

#EL == EnumerateLayers
#I think this another case where you have to make the real call prior to CREATE_TRACE_PACKET(). SInce you
#don't know how many layers will be returned  or how big the strings will be. Alternatively, could be
#on the safe side of CREATE_TRACE_PACKET with maxStringSize*maxLayerCount.
#EL also needs a loop where add a trace buffer  for each layer, depending on how you CREATE_TRACE_PACKET.

    def _generate_trace_funcs(self):
        func_body = []
        for proto in self.protos:
            if 'InitAndEnumerateGpus' == proto.name:
                func_body.append(self._gen_iande_gpus())
            elif 'UnmapMemory' == proto.name:
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
                                packet_size += 'sizeof(%s) + ' % p.ty.strip('*').strip('const ')
                            else:
                                packet_size += '((pDataSize != NULL && pData != NULL) ? *pDataSize : 0) + '
                        elif '**' in p.ty and 'VOID' in p.ty:
                            packet_size += 'sizeof(XGL_VOID*) + '
                        elif 'VOID' in p.ty:
                            packet_size += 'sizeof(%s) + ' % p.name
                        elif 'CHAR' in p.ty:
                            packet_size += '((%s != NULL) ? strlen((const char *)%s) + 1 : 0) + ' % (p.name, p.name)
                        elif 'DEVICE_CREATE_INFO' in p.ty:
                            packet_size += 'calc_size_XGL_DEVICE_CREATE_INFO(pCreateInfo) + '
                        elif 'pDataSize' in p.name:
                            packet_size += '((pDataSize != NULL) ? sizeof(XGL_SIZE) : 0) + '
                            in_data_size = True;
                        elif 'IMAGE_SUBRESOURCE' in p.ty and 'pSubresource' == p.name:
                            packet_size += '((pSubresource != NULL) ? sizeof(XGL_IMAGE_SUBRESOURCE) : 0) + '
                        else:
                            packet_size += 'sizeof(%s) + ' % p.ty.strip('*').strip('const ')
                        buff_ptr_indices.append(proto.params.index(p))
                    else:
                        if 'color' == p.name:
                            packet_update_txt += '    memcpy((void*)pPacket->color, color, 4 * sizeof(XGL_UINT32));\n'
                        else:
                            packet_update_txt += '    pPacket->%s = %s;\n' % (p.name, p.name)
                    if 'Count' in p.name and proto.params[-1].name != p.name and p.name not in ['queryCount', 'vertexCount', 'indexCount', 'startCounter'] and proto.name not in ['CmdLoadAtomicCounters', 'CmdSaveAtomicCounters']:
                        packet_size += '%s*' % p.name
                if '' == packet_size:
                    packet_size = '0'
                else:
                    packet_size = packet_size.strip(' + ')
                func_body[-1] = func_body[-1].replace(',', ')')
                func_body.append('{\n    glv_trace_packet_header* pHeader;')
                if 'VOID' not in proto.ret or '*' in proto.ret:
                    func_body.append('    %s result;' % proto.ret)
                    return_txt = 'result = '
                if in_data_size:
                    func_body.append('    XGL_SIZE dataSizeIn = (pDataSize == NULL) ? 0 : *pDataSize;')
                func_body.append('    struct_xgl%s* pPacket = NULL;' % proto.name)
                func_body.append('    SEND_ENTRYPOINT_ID(xgl%s);' % proto.name)
                if 'EnumerateLayers' == proto.name:
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                    func_body.append('    XGL_SIZE totStringSize = 0;')
                    func_body.append('    uint32_t i = 0;')
                    func_body.append('    for (i = 0; i < *pOutLayerCount; i++)')
                    func_body.append('        totStringSize += (pOutLayers[i] != NULL) ? strlen((const char*)pOutLayers[i]) + 1: 0;')
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, totStringSize + sizeof(XGL_SIZE));' % (proto.name))
                elif proto.name in ['CreateShader', 'CreateGraphicsPipeline', 'CreateComputePipeline']:
                    func_body.append('    size_t customSize;')
                    if 'CreateShader' == proto.name:
                        func_body.append('    customSize = (pCreateInfo != NULL) ? pCreateInfo->codeSize : 0;')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateShader, sizeof(XGL_SHADER_CREATE_INFO) + sizeof(XGL_SHADER) + customSize);')
                    elif 'CreateGraphicsPipeline' == proto.name:
                        func_body.append('    customSize = calculate_pipeline_state_size(pCreateInfo->pNext);')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateGraphicsPipeline, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO) + sizeof(XGL_PIPELINE) + customSize);')
                    else: #'CreateComputePipeline'
                        func_body.append('    customSize = calculate_pipeline_state_size(pCreateInfo->pNext);')
                        func_body.append('    CREATE_TRACE_PACKET(xglCreateComputePipeline, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO) + sizeof(XGL_PIPELINE) + customSize + calculate_pipeline_shader_size(&pCreateInfo->cs));')
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                else:
                    func_body.append('    CREATE_TRACE_PACKET(xgl%s, %s);' % (proto.name, packet_size))
                    func_body.append('    %sreal_xgl%s;' % (return_txt, proto.c_call()))
                func_body.append('    pPacket = interpret_body_as_xgl%s(pHeader);' % proto.name)
                func_body.append(packet_update_txt.strip('\n'))
                if 'MapMemory' == proto.name: # Custom code for MapMem case
                    func_body.append('    if (ppData != NULL)')
                    func_body.append('    {')
                    func_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppData), sizeof(XGL_VOID*), *ppData);')
                    func_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));')
                    func_body.append('        add_data_to_mem_info(mem, *ppData);')
                    func_body.append('    }')
                    func_body.append('    pPacket->result = result;')
                    func_body.append('    FINISH_TRACE_PACKET();')
                elif 'EnumerateLayers' == proto.name: #custom code for EnumerateLayers case
                    func_body.append('    pPacket->gpu = gpu;')
                    func_body.append('    pPacket->maxLayerCount = maxLayerCount;')
                    func_body.append('    pPacket->maxStringSize = maxStringSize;')
                    func_body.append('    for (i = 0; i < *pOutLayerCount; i++) {')
                    func_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOutLayers[i]), ((pOutLayers[i] != NULL) ? strlen((const char *)pOutLayers[i]) + 1 : 0), pOutLayers[i]);')
                    func_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOutLayers[i]));')
                    func_body.append('    }')
                    func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOutLayerCount), sizeof(XGL_SIZE), pOutLayerCount);')
                    func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOutLayerCount));')
                    func_body.append('    pPacket->pReserved = pReserved;')
                    func_body.append('    pPacket->result = result;')
                    func_body.append('    FINISH_TRACE_PACKET();')
                else:
                    # TODO : Clean this up.  Too much custom code and branching.
                    for idx in buff_ptr_indices:
                        if 'DEVICE_CREATE_INFO' in proto.params[idx].ty:
                            func_body.append('    add_XGL_DEVICE_CREATE_INFO_to_packet(pHeader, (XGL_DEVICE_CREATE_INFO**) &(pPacket->pCreateInfo), pCreateInfo);')
                        elif 'CHAR' in proto.params[idx].ty:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), ((%s != NULL) ? strlen((const char *)%s) + 1 : 0), %s);' % (proto.params[idx].name, proto.params[idx].name, proto.params[idx].name, proto.params[idx].name))
                        elif 'Count' in proto.params[idx-1].name and 'queryCount' != proto.params[idx-1].name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s*sizeof(%s), %s);' % (proto.params[idx].name, proto.params[idx-1].name, proto.params[idx].ty.strip('*').strip('const '), proto.params[idx].name))
                        elif 'dataSize' == proto.params[idx].name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), dataSize, %s);' % (proto.params[idx].name, proto.params[idx].name))
                        elif 'pDataSize' == proto.params[idx].name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(XGL_SIZE), &dataSizeIn);' % (proto.params[idx].name))
                        elif 'pData' == proto.params[idx].name:
                            if 'dataSize' == proto.params[idx-1].name:
                                func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), dataSize, %s);' % (proto.params[idx].name, proto.params[idx].name))
                            elif in_data_size:
                                func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), dataSizeIn, %s);' % (proto.params[idx].name, proto.params[idx].name))
                            else:
                                func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), (pDataSize != NULL && pData != NULL) ? *pDataSize : 0, %s);' % (proto.params[idx].name, proto.params[idx].name))
                        else:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(%s), %s);' % (proto.params[idx].name, proto.params[idx].ty.strip('*').strip('const '), proto.params[idx].name))
                    # Some custom add_* and finalize_* function calls for Create* API calls
                    if proto.name in ['CreateShader', 'CreateGraphicsPipeline', 'CreateComputePipeline']:
                        if 'CreateShader' == proto.name:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), customSize, pCreateInfo->pCode);')
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));')
                        elif 'CreateGraphicsPipeline' == proto.name:
                            func_body.append('    add_pipeline_state_to_trace_packet(pHeader, (XGL_VOID**)&pPacket->pCreateInfo->pNext, pCreateInfo->pNext);')
                        else:
                            func_body.append('    add_pipeline_state_to_trace_packet(pHeader, (XGL_VOID**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);')
                            func_body.append('    add_pipeline_shader_to_trace_packet(pHeader, (XGL_PIPELINE_SHADER*)&pPacket->pCreateInfo->cs, &pCreateInfo->cs);')
                            func_body.append('    finalize_pipeline_shader_address(pHeader, &pPacket->pCreateInfo->cs);')
                    if 'VOID' not in proto.ret or '*' in proto.ret:
                        func_body.append('    pPacket->result = result;')
                    for idx in buff_ptr_indices:
                        if 'DEVICE_CREATE_INFO' not in proto.params[idx].ty:
                            func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s));' % (proto.params[idx].name))
                    func_body.append('    FINISH_TRACE_PACKET();')
                    if 'AllocMemory' in proto.name:
                        func_body.append('    add_new_handle_to_mem_info(*pMem, pAllocInfo->allocationSize, NULL);')
                    elif 'FreeMemory' in proto.name:
                        func_body.append('    rm_handle_from_mem_info(mem);')
                if 'VOID' not in proto.ret or '*' in proto.ret:
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
                        if 'CHAR' in p.ty:
                            packet_size += '((%s != NULL) ? strlen((const char *)%s) + 1 : 0) + ' % (p.name, p.name)
                        elif 'Size' not in packet_size:
                            packet_size += 'sizeof(%s) + ' % p.ty.strip('*').strip('const ')
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
                if 'VOID' not in proto.ret or '*' in proto.ret:
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
                    if 'CHAR' in proto.params[idx].ty:
                            func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), ((%s != NULL) ? strlen((const char *)%s) + 1 : 0), %s);' % (proto.params[idx].name, proto.params[idx].name, proto.params[idx].name, proto.params[idx].name))
                    elif 'Size' in proto.params[idx-1].name:
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), %s, %s);' % (proto.params[idx].name, proto.params[idx-1].name, proto.params[idx].name))
                    else:
                        func_body.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->%s), sizeof(%s), %s);' % (proto.params[idx].name, proto.params[idx].ty.strip('*').strip('const '), proto.params[idx].name))
                if 'WsiX11AssociateConnection' in proto.name:
                    func_body.append('    if (pConnectionInfo->pConnection != NULL) {')
                    func_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection), sizeof(void *), pConnectionInfo->pConnection);')
                    func_body.append('        glv_finalize_buffer_address(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection));')
                    func_body.append('    }')
                if 'VOID' not in proto.ret or '*' in proto.ret:
                    func_body.append('    pPacket->result = result;')
                for idx in buff_ptr_indices:
                    func_body.append('    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->%s));' % (proto.params[idx].name))
                func_body.append('    FINISH_TRACE_PACKET();')
                if 'VOID' not in proto.ret or '*' in proto.ret:
                    func_body.append('    return result;')
                func_body.append('}\n')
        return "\n".join(func_body)

    def _generate_helper_funcs(self):
        hf_body = []
        hf_body.append('// Support for shadowing CPU mapped memory')
        hf_body.append('typedef struct _XGLAllocInfo {')
        hf_body.append('    XGL_GPU_SIZE   size;')
        hf_body.append('    XGL_GPU_MEMORY handle;')
        hf_body.append('    XGL_VOID       *pData;')
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
        hf_body.append('static void add_new_handle_to_mem_info(const XGL_GPU_MEMORY handle, XGL_GPU_SIZE size, XGL_VOID *pData)')
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
        hf_body.append('static void add_data_to_mem_info(const XGL_GPU_MEMORY handle, XGL_VOID *pData)')
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
        hf_body.append('static size_t calculate_pipeline_shader_size(const XGL_PIPELINE_SHADER* shader)')
        hf_body.append('{')
        hf_body.append('    size_t size = 0;')
        hf_body.append('    XGL_UINT i, j;')
        hf_body.append('    ')
        hf_body.append('    size += sizeof(XGL_PIPELINE_SHADER);')
        hf_body.append('    // descriptor sets')
        hf_body.append('    for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)')
        hf_body.append('    {')
        hf_body.append('        for (j = 0; j < shader->descriptorSetMapping[i].descriptorCount; j++)')
        hf_body.append('        {')
        hf_body.append('            size += sizeof(XGL_DESCRIPTOR_SLOT_INFO);')
        hf_body.append('            if (shader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)')
        hf_body.append('            {')
        hf_body.append('                size += sizeof(XGL_DESCRIPTOR_SET_MAPPING);')
        hf_body.append('            }')
        hf_body.append('        }')
        hf_body.append('    }')
        hf_body.append('')
        hf_body.append('    // constant buffers')
        hf_body.append('    if (shader->linkConstBufferCount > 0 && shader->pLinkConstBufferInfo != NULL)')
        hf_body.append('    {')
        hf_body.append('        XGL_UINT i;')
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
        hf_body.append('    XGL_UINT i, j;')
        hf_body.append('    // descriptor sets')
        hf_body.append('    for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)')
        hf_body.append('    {')
        hf_body.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo), sizeof(XGL_DESCRIPTOR_SLOT_INFO)* paramShader->descriptorSetMapping[i].descriptorCount, paramShader->descriptorSetMapping[i].pDescriptorInfo);')
        hf_body.append('        for (j = 0; j < paramShader->descriptorSetMapping[i].descriptorCount; j++)')
        hf_body.append('        {')
        hf_body.append('            if (paramShader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)')
        hf_body.append('            {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet), sizeof(XGL_DESCRIPTOR_SET_MAPPING), paramShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet);')
        hf_body.append('            }')
        hf_body.append('        }')
        hf_body.append('        packetShader->descriptorSetMapping[i].descriptorCount = paramShader->descriptorSetMapping[i].descriptorCount;')
        hf_body.append('    }')
        hf_body.append('')
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
        hf_body.append('    XGL_UINT i, j;')
        hf_body.append('    // descriptor sets')
        hf_body.append('    for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)')
        hf_body.append('    {')
        hf_body.append('        for (j = 0; j < packetShader->descriptorSetMapping[i].descriptorCount; j++)')
        hf_body.append('        {')
        hf_body.append('            if (packetShader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)')
        hf_body.append('            {')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet));')
        hf_body.append('            }')
        hf_body.append('        }')
        hf_body.append('        glv_finalize_buffer_address(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo));')
        hf_body.append('    }')
        hf_body.append('')
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
        hf_body.append('static size_t calculate_pipeline_state_size(const XGL_VOID* pState)')
        hf_body.append('{')
        hf_body.append('    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pNext = pState;')
        hf_body.append('    size_t totalStateSize = 0;')
        hf_body.append('    while (pNext)')
        hf_body.append('    {')
        hf_body.append('        switch (pNext->sType)')
        hf_body.append('        {')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:')
        hf_body.append('                totalStateSize += sizeof(XGL_PIPELINE_IA_STATE_CREATE_INFO);')
        hf_body.append('                break;')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:')
        hf_body.append('                totalStateSize += sizeof(XGL_PIPELINE_TESS_STATE_CREATE_INFO);')
        hf_body.append('                break;')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:')
        hf_body.append('                totalStateSize += sizeof(XGL_PIPELINE_RS_STATE_CREATE_INFO);')
        hf_body.append('                break;')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:')
        hf_body.append('                totalStateSize += sizeof(XGL_PIPELINE_DB_STATE_CREATE_INFO);')
        hf_body.append('                break;')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:')
        hf_body.append('                totalStateSize += sizeof(XGL_PIPELINE_CB_STATE);')
        hf_body.append('                break;')
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
        hf_body.append('static void add_pipeline_state_to_trace_packet(glv_trace_packet_header* pHeader, XGL_VOID** ppOut, const XGL_VOID* pIn)')
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
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_IA_STATE_CREATE_INFO), pInNow);')
        hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_TESS_STATE_CREATE_INFO), pInNow);')
        hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_RS_STATE_CREATE_INFO), pInNow);')
        hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_DB_STATE_CREATE_INFO), pInNow);')
        hf_body.append('                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;')
        hf_body.append('                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));')
        hf_body.append('                break;')
        hf_body.append('            }')
        hf_body.append('            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:')
        hf_body.append('            {')
        hf_body.append('                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_CB_STATE), pInNow);')
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
                pid_enum.append('    GLV_TPI_XGL_xgl%s = GLV_TPI_BEGIN_API_HERE,' % proto.name)
            else:
                pid_enum.append('    GLV_TPI_XGL_xgl%s,' % proto.name)
        pid_enum.append('};\n')
        return "\n".join(pid_enum)

    def _generate_stringify_func(self):
        func_body = []
        func_body.append('static const char *stringify_xgl_packet_id(const enum GLV_TRACE_PACKET_ID_XGL id)')
        func_body.append('{')
        func_body.append('    switch(id) {')
        for proto in self.protos:
            func_body.append('    case GLV_TPI_XGL_xgl%s:' % proto.name)
            func_body.append('        return "xgl%s";' % proto.name)
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
        pid_enum.append('    return ((pStruct == NULL) ? 0 : sizeof(XGL_APPLICATION_INFO)) + strlen((const char *)pStruct->pAppName) + 1 + strlen((const char *)pStruct->pEngineName) + 1;')
        pid_enum.append('}\n')
        pid_enum.append('static void add_XGL_APPLICATION_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_APPLICATION_INFO** ppStruct, const XGL_APPLICATION_INFO *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_APPLICATION_INFO), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pAppName), strlen((const char *)pInStruct->pAppName) + 1, (const char *)pInStruct->pAppName);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pEngineName), strlen((const char *)pInStruct->pEngineName) + 1, (const char *)pInStruct->pEngineName);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pAppName));')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pEngineName));')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&*ppStruct);')
        pid_enum.append('};\n')
        pid_enum.append('//=============================================================================\n')
        pid_enum.append('static uint64_t calc_size_XGL_DEVICE_CREATE_INFO(const XGL_DEVICE_CREATE_INFO* pStruct)')
        pid_enum.append('{')
        pid_enum.append('    uint64_t total_size_ppEnabledExtensionNames = pStruct->extensionCount * sizeof(XGL_CHAR *);')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    for (i = 0; i < pStruct->extensionCount; i++)')
        pid_enum.append('    {')
        pid_enum.append('        total_size_ppEnabledExtensionNames += strlen((const char *)pStruct->ppEnabledExtensionNames[i]) + 1;')
        pid_enum.append('    }')
        pid_enum.append('    return sizeof(XGL_DEVICE_CREATE_INFO) + (pStruct->queueRecordCount*sizeof(XGL_DEVICE_CREATE_INFO)) + total_size_ppEnabledExtensionNames;')
        pid_enum.append('}\n')
        pid_enum.append('static void add_XGL_DEVICE_CREATE_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_DEVICE_CREATE_INFO** ppStruct, const XGL_DEVICE_CREATE_INFO *pInStruct)')
        pid_enum.append('{')
        pid_enum.append('    uint32_t i;')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_DEVICE_CREATE_INFO), pInStruct);')
        pid_enum.append('    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pRequestedQueues, pInStruct->queueRecordCount*sizeof(XGL_DEVICE_CREATE_INFO), pInStruct->pRequestedQueues);')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)&(*ppStruct)->pRequestedQueues);')
        pid_enum.append('    if (pInStruct->extensionCount > 0) ')
        pid_enum.append('    {')
        pid_enum.append('        glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledExtensionNames), pInStruct->extensionCount * sizeof(XGL_CHAR *), pInStruct->ppEnabledExtensionNames);')
        pid_enum.append('        for (i = 0; i < pInStruct->extensionCount; i++)')
        pid_enum.append('        {')
        pid_enum.append('            glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])), strlen((const char *)pInStruct->ppEnabledExtensionNames[i]) + 1, (const char *)pInStruct->ppEnabledExtensionNames[i]);')
        pid_enum.append('            glv_finalize_buffer_address(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])));')
        pid_enum.append('        }')
        pid_enum.append('        glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames);')
        pid_enum.append('    }')
        pid_enum.append('    glv_finalize_buffer_address(pHeader, (void**)ppStruct);')
        pid_enum.append('}\n')
        pid_enum.append('static XGL_DEVICE_CREATE_INFO* interpret_XGL_DEVICE_CREATE_INFO(glv_trace_packet_header*  pHeader, intptr_t ptr_variable)')
        pid_enum.append('{')
        pid_enum.append('    XGL_DEVICE_CREATE_INFO* pXGL_DEVICE_CREATE_INFO = (XGL_DEVICE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)ptr_variable);\n')
        pid_enum.append('    if (pXGL_DEVICE_CREATE_INFO != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        pXGL_DEVICE_CREATE_INFO->pRequestedQueues = (const XGL_DEVICE_QUEUE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->pRequestedQueues);\n')
        pid_enum.append('        if (pXGL_DEVICE_CREATE_INFO->extensionCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            const XGL_CHAR** pNames;')
        pid_enum.append('            uint32_t i;')
        pid_enum.append('            pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames = (const XGL_CHAR *const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames);')
        pid_enum.append('            pNames = (const XGL_CHAR**)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames;')
        pid_enum.append('            for (i = 0; i < pXGL_DEVICE_CREATE_INFO->extensionCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                pNames[i] = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames[i]));')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('    }\n')
        pid_enum.append('    return pXGL_DEVICE_CREATE_INFO;')
        pid_enum.append('}\n')
        pid_enum.append('static void interpret_pipeline_shader(glv_trace_packet_header*  pHeader, XGL_PIPELINE_SHADER* pShader)')
        pid_enum.append('{')
        pid_enum.append('    XGL_UINT i, j;')
        pid_enum.append('    if (pShader != NULL)')
        pid_enum.append('    {')
        pid_enum.append('        // descriptor sets')
        pid_enum.append('        // TODO: need to ensure XGL_MAX_DESCRIPTOR_SETS is equal in replay as it was at trace time - meta data')
        pid_enum.append('        for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)')
        pid_enum.append('        {')
        pid_enum.append('            pShader->descriptorSetMapping[i].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->descriptorSetMapping[i].pDescriptorInfo);')
        pid_enum.append('            for (j = 0; j < pShader->descriptorSetMapping[i].descriptorCount; j++)')
        pid_enum.append('            {')
        pid_enum.append('                if (pShader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)')
        pid_enum.append('                {')
        pid_enum.append('                    XGL_DESCRIPTOR_SLOT_INFO* pInfo = (XGL_DESCRIPTOR_SLOT_INFO*)pShader->descriptorSetMapping[i].pDescriptorInfo;')
        pid_enum.append('                    pInfo[j].pNextLevelSet = (const XGL_DESCRIPTOR_SET_MAPPING*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet);')
        pid_enum.append('                }')
        pid_enum.append('            }')
        pid_enum.append('        }\n')
        pid_enum.append('        // constant buffers')
        pid_enum.append('        if (pShader->linkConstBufferCount > 0)')
        pid_enum.append('        {')
        pid_enum.append('            XGL_UINT i;')
        pid_enum.append('            pShader->pLinkConstBufferInfo = (const XGL_LINK_CONST_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo);')
        pid_enum.append('            for (i = 0; i < pShader->linkConstBufferCount; i++)')
        pid_enum.append('            {')
        pid_enum.append('                XGL_LINK_CONST_BUFFER* pBuffer = (XGL_LINK_CONST_BUFFER*)pShader->pLinkConstBufferInfo;')
        pid_enum.append('                pBuffer[i].pBufferData = (const XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo[i].pBufferData);')
        pid_enum.append('            }')
        pid_enum.append('        }')
        pid_enum.append('    }')
        pid_enum.append('}\n')
        pid_enum.append('//=============================================================================')
        return "\n".join(pid_enum)

    def _generate_interp_funcs(self):
        # Custom txt for given function and parameter.  First check if param is NULL, then insert txt if not
        custom_case_dict = { 'InitAndEnumerateGpus' : {'param': 'pAppInfo', 'txt': ['XGL_APPLICATION_INFO* pInfo = (XGL_APPLICATION_INFO*)pPacket->pAppInfo;\n', 'pInfo->pAppName = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pAppName);\n', 'pInfo->pEngineName = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pEngineName);']},
                             'CreateShader' : {'param': 'pCreateInfo', 'txt': ['XGL_SHADER_CREATE_INFO* pInfo = (XGL_SHADER_CREATE_INFO*)pPacket->pCreateInfo;\n', 'pInfo->pCode = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pCode);']},
                             'CreateGraphicsPipeline' : {'param': 'pCreateInfo', 'txt': ['assert(pPacket->pCreateInfo->sType == XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);\n', '// need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one\n','XGL_VOID** ppNextVoidPtr = (XGL_VOID**)&pPacket->pCreateInfo->pNext;\n','*ppNextVoidPtr = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pNext);\n',
                                                                                         'XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;\n', 'while ((NULL != pNext) && (XGL_NULL_HANDLE != pNext))\n', '{\n',
                                                                                         '    switch(pNext->sType)\n', '    {\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            XGL_VOID** ppNextVoidPtr = (XGL_VOID**)&pNext->pNext;\n',
                                                                                         '            *ppNextVoidPtr = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            XGL_VOID** ppNextVoidPtr = (XGL_VOID**)&pNext->pNext;\n',
                                                                                         '            *ppNextVoidPtr = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
                                                                                         '            interpret_pipeline_shader(pHeader, &pNext->shader);\n',
                                                                                         '            break;\n',
                                                                                         '        }\n',
                                                                                         '        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:\n',
                                                                                         '        {\n',
                                                                                         '            XGL_VOID** ppNextVoidPtr = (XGL_VOID**)&pNext->pNext;\n',
                                                                                         '            XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pVi = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *) pNext;\n',
                                                                                         '            *ppNextVoidPtr = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);\n',
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
        for proto in self.protos:
            if 'Wsi' not in proto.name and 'Dbg' not in proto.name:
                if 'UnmapMemory' == proto.name:
                    proto.params = proto.params + (xgl.Param("XGL_VOID*", "pData"),)
                if_body.append('typedef struct struct_xgl%s {' % proto.name)
                if_body.append('    glv_trace_packet_header* header;')
                for p in proto.params:
                    if '[4]' in p.ty:
                        if_body.append('    %s %s[4];' % (p.ty.strip('[4]'), p.name))
                    else:
                        if_body.append('    %s %s;' % (p.ty, p.name))
                if 'XGL_VOID' != proto.ret:
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
                if 'XGL_VOID' != proto.ret:
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
        cd_body.append('#if 0')
        cd_body.append('    XGL_DEVICE m_dev[XGL_MAX_PHYSICAL_GPUS];')
        cd_body.append('    XGL_UINT32 m_gpuCount;')
        cd_body.append('    unsigned int m_gpuIdx;')
        cd_body.append('    XGL_PHYSICAL_GPU m_gpus[XGL_MAX_PHYSICAL_GPUS];')
        cd_body.append('    XGL_PHYSICAL_GPU_PROPERTIES m_gpuProps[XGL_MAX_PHYSICAL_GPUS];')
        cd_body.append('#endif')
        cd_body.append('    std::vector<XGL_CHAR *>m_extensions;')
        cd_body.append('};\n')
        cd_body.append('typedef struct _XGLAllocInfo {')
        cd_body.append('    XGL_GPU_SIZE size;')
        cd_body.append('    XGL_VOID *pData;')
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
        obj_map_dict = {'m_gpus': 'XGL_PHYSICAL_GPU',
                        'm_devices': 'XGL_DEVICE',
                        'm_queues': 'XGL_QUEUE',
                        'm_memories': 'XGL_GPU_MEMORY',
                        'm_images': 'XGL_IMAGE',
                        'm_imageViews': 'XGL_IMAGE_VIEW',
                        'm_colorTargetViews': 'XGL_COLOR_ATTACHMENT_VIEW',
                        'm_depthStencilViews': 'XGL_DEPTH_STENCIL_VIEW',
                        'm_shader': 'XGL_SHADER',
                        'm_pipeline': 'XGL_PIPELINE',
                        'm_pipelineDelta': 'XGL_PIPELINE_DELTA',
                        'm_sampler': 'XGL_SAMPLER',
                        'm_descriptorSets': 'XGL_DESCRIPTOR_SET',
                        'm_viewportStates': 'XGL_VIEWPORT_STATE_OBJECT',
                        'm_rasterStates': 'XGL_RASTER_STATE_OBJECT',
                        'm_msaaStates': 'XGL_MSAA_STATE_OBJECT',
                        'm_colorBlendStates': 'XGL_COLOR_BLEND_STATE_OBJECT',
                        'm_depthStencilStates': 'XGL_DEPTH_STENCIL_STATE_OBJECT',
                        'm_cmdBuffers': 'XGL_CMD_BUFFER',
                        'm_fences': 'XGL_FENCE',
                        'm_queue_semaphores': 'XGL_QUEUE_SEMAPHORE',
                        'm_events': 'XGL_EVENT',
                        'm_queryPools': 'XGL_QUERY_POOL',
                        }
        rc_body = []
        rc_body.append('class xglReplay : public ApiReplay {')
        rc_body.append('public:')
        rc_body.append('    ~xglReplay();')
        rc_body.append('    xglReplay(unsigned int debugLevel);\n')
        rc_body.append('    int init(glv_replay::Display & disp);')
        rc_body.append('    xglDisplay * get_display() {return m_display;}')
        rc_body.append('    glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header *packet);')
        rc_body.append('    glv_replay::GLV_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn);\n')
        rc_body.append('private:')
        rc_body.append('    struct xglFuncs m_xglFuncs;')
        rc_body.append('    void copy_mem_remap_range_struct(XGL_VIRTUAL_MEMORY_REMAP_RANGE *outRange, const XGL_VIRTUAL_MEMORY_REMAP_RANGE *inRange);')
        rc_body.append('    unsigned int m_debugLevel;')
        rc_body.append('    xglDisplay *m_display;')
        rc_body.append('    XGL_MEMORY_HEAP_PROPERTIES m_heapProps[XGL_MAX_MEMORY_HEAPS];')
        rc_body.append('    struct shaderPair {')
        rc_body.append('        XGL_SHADER *addr;')
        rc_body.append('        XGL_SHADER val;')
        rc_body.append('    };')
        rc_body.append(self._map_decl('XGL_GPU_MEMORY', 'XGLAllocInfo', 'm_mapData'))
        # Custom code for 1-off memory mapping functions
        rc_body.append('    void add_entry_to_mapData(XGL_GPU_MEMORY handle, XGL_GPU_SIZE size)')
        rc_body.append('    {')
        rc_body.append('        XGLAllocInfo info;')
        rc_body.append('        info.pData = NULL;')
        rc_body.append('        info.size = size;')
        rc_body.append('        m_mapData.insert(std::pair<XGL_GPU_MEMORY, XGLAllocInfo>(handle, info));')
        rc_body.append('    }')
        rc_body.append('    void add_mapping_to_mapData(XGL_GPU_MEMORY handle, XGL_VOID *pData)')
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
        rc_body.append('    void rm_mapping_from_mapData(XGL_GPU_MEMORY handle, XGL_VOID* pData)')
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
        # XGL_STATE_OBJECT code
        state_obj_remap_types = ['XGL_VIEWPORT_STATE_OBJECT', 'XGL_RASTER_STATE_OBJECT', 'XGL_MSAA_STATE_OBJECT', 'XGL_COLOR_BLEND_STATE_OBJECT', 'XGL_DEPTH_STENCIL_STATE_OBJECT']
        rc_body.append('    XGL_STATE_OBJECT remap(const XGL_STATE_OBJECT& state)\n    {')
        rc_body.append('        XGL_STATE_OBJECT obj;')
        for t in state_obj_remap_types:
            rc_body.append('        if ((obj = remap(static_cast <%s> (state))) != XGL_NULL_HANDLE)' % t)
            rc_body.append('            return obj;')
        rc_body.append('        return XGL_NULL_HANDLE;\n    }')
        rc_body.append('    void rm_from_map(const XGL_STATE_OBJECT& state)\n    {')
        for t in state_obj_remap_types:
            rc_body.append('        rm_from_map(static_cast <%s> (state));' % t)
        rc_body.append('    }')
        # OBJECT code
        rc_body.append('    XGL_OBJECT remap(const XGL_OBJECT& object)\n    {')
        rc_body.append('        XGL_OBJECT obj;')
        obj_remap_types = ['XGL_CMD_BUFFER', 'XGL_IMAGE', 'XGL_IMAGE_VIEW', 'XGL_COLOR_ATTACHMENT_VIEW', 'XGL_DEPTH_STENCIL_VIEW', 'XGL_SHADER', 'XGL_PIPELINE', 'XGL_PIPELINE_DELTA', 'XGL_SAMPLER', 'XGL_DESCRIPTOR_SET', 'XGL_STATE_OBJECT', 'XGL_FENCE', 'XGL_QUEUE_SEMAPHORE', 'XGL_EVENT', 'XGL_QUERY_POOL']
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
        rc_body.append('};')
        return "\n".join(rc_body)

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

class ApiDumpNoAddrSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n#include <pthread.h>\n#include "xglLayer.h"\n#include "xgl_struct_string_helper_no_addr.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;\nstatic pthread_once_t tabOnce = PTHREAD_ONCE_INIT;\npthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;\n')
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
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "APIDump", True),
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
        header_txt.append('    const char*          pLayerPrefix,')
        header_txt.append('    const char*          pMsg)')
        header_txt.append('{')
        header_txt.append('    XGL_LAYER_DBG_FUNCTION_NODE *pTrav = pDbgFunctionHead;')
        header_txt.append('    if (pTrav) {')
        header_txt.append('        while (pTrav) {')
        header_txt.append('            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, (const XGL_CHAR *) pMsg, pTrav->pUserData);')
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
        header_txt.append('    if (0) ll_print_lists();')
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
        header_txt.append('#if 0')
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
        header_txt.append('#endif')
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
                self._generate_init_tracer(),
                self._generate_helper_funcs(),
                self._generate_trace_funcs()]

        return "\n".join(body)

class GlavePacketID(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#pragma once\n')
        header_txt.append('#include "glv_trace_packet_utils.h"')
        header_txt.append('#include "glv_interconnect.h"')
        header_txt.append('#include "glvtrace_xgl_xgl_structs.h"')
        header_txt.append('#include "glvtrace_xgl_xgldbg_structs.h"')
        header_txt.append('#include "glvtrace_xgl_xglwsix11ext_structs.h"')
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
        header_txt.append('#include "xglWsiX11Ext.h"\n')
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
        header_txt.append('#include "xglWsiX11Ext.h"')
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
        header_txt.append('#include <xcb/xcb.h>\n')
        header_txt.append('#include "glvreplay_window.h"')
        header_txt.append('#include "glvreplay_factory.h"')
        header_txt.append('#include "glv_trace_packet_identifiers.h"\n')
        header_txt.append('#include "xgl.h"')
        header_txt.append('#include "xglDbg.h"')
        header_txt.append('#include "xglWsiX11Ext.h"')
        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_replay_class_decls(),
                self._generate_replay_func_ptrs(),
                self._generate_replay_class()]

        return "\n".join(body)

class GlaveReplayC(Subcommand):
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

def main():
    subcommands = {
            "layer-funcs" : LayerFuncsSubcommand,
            "layer-dispatch" : LayerDispatchSubcommand,
            "Generic" : GenericLayerSubcommand,
            "ApiDump" : ApiDumpSubcommand,
            "ApiDumpFile" : ApiDumpFileSubcommand,
            "ApiDumpNoAddr" : ApiDumpNoAddrSubcommand,
            "ObjectTracker" : ObjectTrackerSubcommand,
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
