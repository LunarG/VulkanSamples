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
import os

import xgl
import xgl_helper

def generate_get_proc_addr_check(name):
    return "    if (!%s || %s[0] != 'x' || %s[1] != 'g' || %s[2] != 'l')\n" \
           "        return NULL;" % ((name,) * 4)

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.headers = xgl.headers
        self.protos = xgl.protos
        self.no_addr = False
        self.layer_name = ""

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

    # Return set of printf '%' qualifier and input to that qualifier
    def _get_printf_params(self, xgl_type, name, output_param, cpp=False):
        # TODO : Need ENUM and STRUCT checks here
        if xgl_helper.is_type(xgl_type, 'enum'):#"_TYPE" in xgl_type: # TODO : This should be generic ENUM check
            return ("%s", "string_%s(%s)" % (xgl_type.strip('const ').strip('*'), name))
        if "char*" == xgl_type:
            return ("%s", name)
        if "uint64" in xgl_type:
            if '*' in xgl_type:
                return ("%lu", "*%s" % name)
            return ("%lu", name)
        if "size" in xgl_type:
            if '*' in xgl_type:
                return ("%zu", "*%s" % name)
            return ("%zu", name)
        if "float" in xgl_type:
            if '[' in xgl_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                if cpp:
                    return ("[%i, %i, %i, %i]", '"[" << %s[0] << "," << %s[1] << "," << %s[2] << "," << %s[3] << "]"' % (name, name, name, name))
                return ("[%f, %f, %f, %f]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name))
            return ("%f", name)
        if "bool" in xgl_type or 'xcb_randr_crtc_t' in xgl_type:
            return ("%u", name)
        if True in [t in xgl_type for t in ["int", "FLAGS", "MASK", "xcb_window_t"]]:
            if '[' in xgl_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                if cpp:
                    return ("[%i, %i, %i, %i]", "%s[0] << %s[1] << %s[2] << %s[3]" % (name, name, name, name))
                return ("[%i, %i, %i, %i]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name))
            if '*' in xgl_type:
                if 'pUserData' == name:
                    return ("%i", "((pUserData == 0) ? 0 : *(pUserData))")
                return ("%i", "*(%s)" % name)
            return ("%i", name)
        # TODO : This is special-cased as there's only one "format" param currently and it's nice to expand it
        if "XGL_FORMAT" == xgl_type:
            if cpp:
                return ("%p", "&%s" % name)
            return ("{%s.channelFormat = %%s, %s.numericFormat = %%s}" % (name, name), "string_XGL_CHANNEL_FORMAT(%s.channelFormat), string_XGL_NUM_FORMAT(%s.numericFormat)" % (name, name))
        if output_param:
            return ("%p", "(void*)*%s" % name)
        if xgl_helper.is_type(xgl_type, 'struct') and '*' not in xgl_type:
            return ("%p", "(void*)(&%s)" % name)
        return ("%p", "(void*)(%s)" % name)

    def _gen_layer_dbg_callback_register(self):
        r_body = []
        r_body.append('XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_INSTANCE instance, XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)')
        r_body.append('{')
        r_body.append('    // This layer intercepts callbacks')
        r_body.append('    XGL_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (XGL_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(XGL_LAYER_DBG_FUNCTION_NODE));')
        r_body.append('    if (!pNewDbgFuncNode)')
        r_body.append('        return XGL_ERROR_OUT_OF_MEMORY;')
        r_body.append('    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;')
        r_body.append('    pNewDbgFuncNode->pUserData = pUserData;')
        r_body.append('    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;')
        r_body.append('    g_pDbgFunctionHead = pNewDbgFuncNode;')
        r_body.append('    // force callbacks if DebugAction hasn\'t been set already other than initial value')
        r_body.append('    if (g_actionIsDefault) {')
        r_body.append('        g_debugAction = XGL_DBG_LAYER_ACTION_CALLBACK;')
        r_body.append('    }')
        r_body.append('    XGL_RESULT result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);')
        r_body.append('    return result;')
        r_body.append('}')
        return "\n".join(r_body)

    def _gen_layer_dbg_callback_unregister(self):
        ur_body = []
        ur_body.append('XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_INSTANCE instance, XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)')
        ur_body.append('{')
        ur_body.append('    XGL_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;')
        ur_body.append('    XGL_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;')
        ur_body.append('    while (pTrav) {')
        ur_body.append('        if (pTrav->pfnMsgCallback == pfnMsgCallback) {')
        ur_body.append('            pPrev->pNext = pTrav->pNext;')
        ur_body.append('            if (g_pDbgFunctionHead == pTrav)')
        ur_body.append('                g_pDbgFunctionHead = pTrav->pNext;')
        ur_body.append('            free(pTrav);')
        ur_body.append('            break;')
        ur_body.append('        }')
        ur_body.append('        pPrev = pTrav;')
        ur_body.append('        pTrav = pTrav->pNext;')
        ur_body.append('    }')
        ur_body.append('    if (g_pDbgFunctionHead == NULL)')
        ur_body.append('    {')
        ur_body.append('        if (g_actionIsDefault)')
        ur_body.append('            g_debugAction = XGL_DBG_LAYER_ACTION_LOG_MSG;')
        ur_body.append('        else')
        ur_body.append('            g_debugAction &= ~XGL_DBG_LAYER_ACTION_CALLBACK;')
        ur_body.append('    }')
        ur_body.append('    XGL_RESULT result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);')
        ur_body.append('    return result;')
        ur_body.append('}')
        return "\n".join(ur_body)

    def _generate_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        intercepted = []
        for proto in self.protos:
            if proto.name != "GetProcAddr" and proto.name != "InitAndEnumerateGpus":
                intercept = self.generate_intercept(proto, qual)
                if intercept is None:
                    # fill in default intercept for certain entrypoints
                    if 'DbgRegisterMsgCallback' == proto.name:
                        intercept = self._gen_layer_dbg_callback_register()
                    if 'DbgUnregisterMsgCallback' == proto.name:
                        intercept = self._gen_layer_dbg_callback_unregister()
                if intercept is not None:
                    funcs.append(intercept)
                    intercepted.append(proto)

        prefix="xgl"
        lookups = []
        for proto in intercepted:
            if 'WsiX11' in proto.name:
                lookups.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (void*) %s%s;" %
                    (prefix, proto.name))
            if 'WsiX11' in proto.name:
                lookups.append("#endif")

        # add customized layer_intercept_proc
        body = []
        body.append("static inline void* layer_intercept_proc(const char *name)")
        body.append("{")
        body.append(generate_get_proc_addr_check("name"))
        body.append("")
        body.append("    name += 3;")
        body.append("    %s" % "\n    ".join(lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")
        funcs.append("\n".join(body))

        return "\n\n".join(funcs)


    def _generate_extensions(self):
        exts = []
        exts.append('uint64_t objTrackGetObjectCount(XGL_OBJECT_TYPE type)')
        exts.append('{')
        exts.append('    return (type == XGL_OBJECT_TYPE_ANY) ? numTotalObjs : numObjs[type];')
        exts.append('}')
        exts.append('')
        exts.append('XGL_RESULT objTrackGetObjects(XGL_OBJECT_TYPE type, uint64_t objCount, OBJTRACK_NODE* pObjNodeArray)')
        exts.append('{')
        exts.append("    // This bool flags if we're pulling all objs or just a single class of objs")
        exts.append('    bool32_t bAllObjs = (type == XGL_OBJECT_TYPE_ANY);')
        exts.append('    // Check the count first thing')
        exts.append('    uint64_t maxObjCount = (bAllObjs) ? numTotalObjs : numObjs[type];')
        exts.append('    if (objCount > maxObjCount) {')
        exts.append('        char str[1024];')
        exts.append('        sprintf(str, "OBJ ERROR : Received objTrackGetObjects() request for %lu objs, but there are only %lu objs of type %s", objCount, maxObjCount, string_XGL_OBJECT_TYPE(type));')
        exts.append('        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_OBJCOUNT_MAX_EXCEEDED, "OBJTRACK", str);')
        exts.append('        return XGL_ERROR_INVALID_VALUE;')
        exts.append('    }')
        exts.append('    objNode* pTrav = (bAllObjs) ? pGlobalHead : pObjectHead[type];')
        exts.append('    for (uint64_t i = 0; i < objCount; i++) {')
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

    def _generate_layer_gpa_function(self, extensions=[]):
        func_body = []
        func_body.append("XGL_LAYER_EXPORT void* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const char* funcName)\n"
                         "{\n"
                         "    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;\n"
                         "    void* addr;\n"
                         "    if (gpu == NULL)\n"
                         "        return NULL;\n"
                         "    pCurObj = gpuw;\n"
                         "    loader_platform_thread_once(&tabOnce, init%s);\n\n"
                         "    addr = layer_intercept_proc(funcName);\n"
                         "    if (addr)\n"
                         "        return addr;" % self.layer_name)

        if 0 != len(extensions):
            for ext_name in extensions:
                func_body.append('    else if (!strncmp("%s", funcName, sizeof("%s")))\n'
                                 '        return %s;' % (ext_name, ext_name, ext_name))
        func_body.append("    else {\n"
                         "        if (gpuw->pGPA == NULL)\n"
                         "            return NULL;\n"
                         "        return gpuw->pGPA((XGL_PHYSICAL_GPU)gpuw->nextObject, funcName);\n"
                         "    }\n"
                         "}\n")
        return "\n".join(func_body)

    def _generate_layer_initialization(self, init_opts=False, prefix='xgl', lockname=None):
        func_body = ["#include \"xgl_dispatch_table_helper.h\""]
        func_body.append('static void init%s(void)\n'
                         '{\n' % self.layer_name)
        if init_opts:
            func_body.append('    const char *strOpt;')
            func_body.append('    // initialize %s options' % self.layer_name)
            func_body.append('    getLayerOptionEnum("%sReportLevel", (uint32_t *) &g_reportingLevel);' % self.layer_name)
            func_body.append('    g_actionIsDefault = getLayerOptionEnum("%sDebugAction", (uint32_t *) &g_debugAction);' % self.layer_name)
            func_body.append('')
            func_body.append('    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)')
            func_body.append('    {')
            func_body.append('        strOpt = getLayerOption("%sLogFilename");' % self.layer_name)
            func_body.append('        if (strOpt)')
            func_body.append('        {')
            func_body.append('            g_logFile = fopen(strOpt, "w");')
            func_body.append('        }')
            func_body.append('        if (g_logFile == NULL)')
            func_body.append('            g_logFile = stdout;')
            func_body.append('    }')
            func_body.append('')
        func_body.append('    xglGetProcAddrType fpNextGPA;\n'
                         '    fpNextGPA = pCurObj->pGPA;\n'
                         '    assert(fpNextGPA);\n')

        func_body.append("    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (XGL_PHYSICAL_GPU) pCurObj->nextObject);")
        if lockname is not None:
            func_body.append("    if (!%sLockInitialized)" % lockname)
            func_body.append("    {")
            func_body.append("        // TODO/TBD: Need to delete this mutex sometime.  How???")
            func_body.append("        loader_platform_thread_create_mutex(&%sLock);" % lockname)
            func_body.append("        %sLockInitialized = 1;" % lockname)
            func_body.append("    }")
        func_body.append("}\n")
        return "\n".join(func_body)

    def _generate_layer_initialization_with_lock(self, prefix='xgl'):
        func_body = ["#include \"xgl_dispatch_table_helper.h\""]
        func_body.append('static void init%s(void)\n'
                         '{\n'
                         '    xglGetProcAddrType fpNextGPA;\n'
                         '    fpNextGPA = pCurObj->pGPA;\n'
                         '    assert(fpNextGPA);\n' % self.layer_name);

        func_body.append("    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (XGL_PHYSICAL_GPU) pCurObj->nextObject);\n")
        func_body.append("    if (!printLockInitialized)")
        func_body.append("    {")
        func_body.append("        // TODO/TBD: Need to delete this mutex sometime.  How???")
        func_body.append("        loader_platform_thread_create_mutex(&printLock);")
        func_body.append("        printLockInitialized = 1;")
        func_body.append("    }")
        func_body.append("}\n")
        return "\n".join(func_body)

class LayerFuncsSubcommand(Subcommand):
    def generate_header(self):
        return '#include <xglLayer.h>\n#include "loader.h"'

    def generate_body(self):
        return self._generate_dispatch_entrypoints("static")

class LayerDispatchSubcommand(Subcommand):
    def generate_header(self):
        return '#include "layer_wrappers.h"'

    def generate_body(self):
        return self._generate_layer_initialization()

class GenericLayerSubcommand(Subcommand):
    def generate_header(self):
        return '#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include "loader_platform.h"\n#include "xglLayer.h"\n//The following is #included again to catch certain OS-specific functions being used:\n#include "loader_platform.h"\n\n#include "layers_config.h"\n#include "layers_msg.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;\n\nstatic LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);'

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback' ]:
            # use default version
            return None
        decl = proto.c_func(prefix="xgl", attr="XGLAPI")
        param0_name = proto.params[0].name
        ret_val = ''
        stmt = ''
        funcs = []
        if proto.ret != "void":
            ret_val = "XGL_RESULT result = "
            stmt = "    return result;\n"
        if 'WsiX11AssociateConnection' == proto.name:
            funcs.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
        if proto.name == "EnumerateLayers":
            c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
            funcs.append('%s%s\n'
                     '{\n'
                     '    char str[1024];\n'
                     '    if (gpu != NULL) {\n'
                     '        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                     '        sprintf(str, "At start of layered %s\\n");\n'
                     '        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, gpu, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '        pCurObj = gpuw;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
                     '        %snextTable.%s;\n'
                     '        sprintf(str, "Completed layered %s\\n");\n'
                     '        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, gpu, 0, 0, (char *) "GENERIC", (char *) str);\n'
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
                         '}' % (qual, decl, proto.params[0].name, proto.name, self.layer_name, ret_val, c_call, proto.name, stmt, self.layer_name))
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
                     '    char str[1024];'
                     '    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                     '    sprintf(str, "At start of layered %s\\n");\n'
                     '    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, gpuw, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '    pCurObj = gpuw;\n'
                     '    loader_platform_thread_once(&tabOnce, init%s);\n'
                     '    %snextTable.%s;\n'
                     '    sprintf(str, "Completed layered %s\\n");\n'
                     '    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, gpuw, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '    fflush(stdout);\n'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, proto.name, self.layer_name, ret_val, c_call, proto.name, stmt))
        if 'WsiX11QueuePresent' == proto.name:
            funcs.append("#endif")
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "Generic"
        body = [self._generate_layer_initialization(True),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ApiDumpSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "xglLayer.h"\n#include "xgl_struct_string_helper.h"\n')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('static XGL_LAYER_DISPATCH_TABLE nextTable;')
        header_txt.append('static XGL_BASE_LAYER_OBJECT *pCurObj;\n')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;\n')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static loader_platform_thread_id tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t) maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        decl = proto.c_func(prefix="xgl", attr="XGLAPI")
        param0_name = proto.params[0].name
        ret_val = ''
        stmt = ''
        funcs = []
        sp_param_dict = {} # Store 'index' for struct param to print, or an name of binding "Count" param for array to print
        create_params = 0 # Num of params at end of function that are created and returned as output values
        if 'WsiX11CreatePresentableImage' in proto.name:
            create_params = -2
        elif 'Create' in proto.name or 'Alloc' in proto.name or 'MapMemory' in proto.name:
            create_params = -1
        if proto.ret != "void":
            ret_val = "XGL_RESULT result = "
            stmt = "    return result;\n"
        f_open = ''
        f_close = ''
        if "File" in self.layer_name:
            file_mode = "a"
            if 'CreateDevice' in proto.name:
                file_mode = "w"
            f_open = 'loader_platform_thread_lock_mutex(&printLock);\n    pOutFile = fopen(outFileName, "%s");\n    ' % (file_mode)
            log_func = 'fprintf(pOutFile, "t{%%u} xgl%s(' % proto.name
            f_close = '\n    fclose(pOutFile);\n    loader_platform_thread_unlock_mutex(&printLock);'
        else:
            f_open = 'loader_platform_thread_lock_mutex(&printLock);\n    '
            log_func = 'printf("t{%%u} xgl%s(' % proto.name
            f_close = '\n    loader_platform_thread_unlock_mutex(&printLock);'
        print_vals = ', getTIDIndex()'
        pindex = 0
        prev_count_name = ''
        for p in proto.params:
            cp = False
            if 0 != create_params:
                # If this is any of the N last params of the func, treat as output
                for y in range(-1, create_params-1, -1):
                    if p.name == proto.params[y].name:
                        cp = True
            (pft, pfi) = self._get_printf_params(p.ty, p.name, cp)
            if self.no_addr and "%p" == pft:
                (pft, pfi) = ("%s", '"addr"')
            log_func += '%s = %s, ' % (p.name, pft)
            print_vals += ', %s' % (pfi)
            # Catch array inputs that are bound by a "Count" param
            if prev_count_name != '' and (prev_count_name.strip('Count')[1:] in p.name or 'slotCount' == prev_count_name):
                sp_param_dict[pindex] = prev_count_name
            elif 'pDescriptorSets' == p.name and proto.params[-1].name == 'pCount':
                sp_param_dict[pindex] = '*pCount'
            elif 'Wsi' not in proto.name and xgl_helper.is_type(p.ty.strip('*').strip('const '), 'struct'):
                sp_param_dict[pindex] = 'index'
            pindex += 1
            if p.name.endswith('Count'):
                if '*' in p.ty:
                    prev_count_name = "*%s" % p.name
                else:
                    prev_count_name = p.name
            else:
                prev_count_name = ''
        log_func = log_func.strip(', ')
        if proto.ret != "void":
            log_func += ') = %s\\n"'
            print_vals += ', string_XGL_RESULT(result)'
        else:
            log_func += ')\\n"'
        log_func = '%s%s);' % (log_func, print_vals)
        if len(sp_param_dict) > 0:
            i_decl = False
            log_func += '\n    char *pTmpStr = "";'
            for sp_index in sorted(sp_param_dict):
                # TODO : Clean this if/else block up, too much duplicated code
                if 'index' == sp_param_dict[sp_index]:
                    cis_print_func = 'xgl_print_%s' % (proto.params[sp_index].ty.strip('const ').strip('*').lower())
                    var_name = proto.params[sp_index].name
                    if proto.params[sp_index].name != 'color':
                        log_func += '\n    if (%s) {' % (proto.params[sp_index].name)
                    else:
                        var_name = "&%s" % proto.params[sp_index].name
                    log_func += '\n        pTmpStr = %s(%s, "    ");' % (cis_print_func, var_name)
                    if "File" in self.layer_name:
                        if self.no_addr:
                            log_func += '\n        fprintf(pOutFile, "   %s (addr)\\n%%s\\n", pTmpStr);' % (var_name)
                        else:
                            log_func += '\n        fprintf(pOutFile, "   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (var_name, var_name)
                    else:
                        if self.no_addr:
                            log_func += '\n        printf("   %s (addr)\\n%%s\\n", pTmpStr);' % (proto.params[sp_index].name)
                        else:
                            log_func += '\n        printf("   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, var_name)
                        log_func += '\n        fflush(stdout);'
                    log_func += '\n        free(pTmpStr);'
                    if proto.params[sp_index].name != 'color':
                        log_func += '\n    }'
                else: # should have a count value stored to iterate over array
                    if xgl_helper.is_type(proto.params[sp_index].ty.strip('*').strip('const '), 'struct'):
                        cis_print_func = 'pTmpStr = xgl_print_%s(&%s[i], "    ");' % (proto.params[sp_index].ty.strip('const ').strip('*').lower(), proto.params[sp_index].name)
                    else:
                        cis_print_func = 'pTmpStr = (char*)malloc(32);\n        sprintf(pTmpStr, "    %%p", %s[i]);' % proto.params[sp_index].name
                    if not i_decl:
                        log_func += '\n    uint32_t i;'
                        i_decl = True
                    log_func += '\n    for (i = 0; i < %s; i++) {' % (sp_param_dict[sp_index])
                    log_func += '\n        %s' % (cis_print_func)
                    if "File" in self.layer_name:
                        if self.no_addr:
                            log_func += '\n        fprintf(pOutFile, "   %s[%%i] (addr)\\n%%s\\n", i, pTmpStr);' % (proto.params[sp_index].name)
                        else:
                            log_func += '\n        fprintf(pOutFile, "   %s[%%i] (%%p)\\n%%s\\n", i, (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                    else:
                        if self.no_addr:
                            log_func += '\n        printf("   %s[%%i] (addr)\\n%%s\\n", i, pTmpStr);' % (proto.params[sp_index].name)
                        else:
                            log_func += '\n        printf("   %s[%%i] (%%p)\\n%%s\\n", i, (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                        log_func += '\n        fflush(stdout);'
                    log_func += '\n        free(pTmpStr);\n    }'
        if 'WsiX11AssociateConnection' == proto.name:
            funcs.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
        if proto.name == "EnumerateLayers":
            c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
            funcs.append('%s%s\n'
                     '{\n'
                     '    if (gpu != NULL) {\n'
                     '        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                     '        pCurObj = gpuw;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
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
                         '}' % (qual, decl, proto.params[0].name, self.layer_name, ret_val, c_call,f_open, log_func, f_close, stmt, self.layer_name))
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
                     '    loader_platform_thread_once(&tabOnce, init%s);\n'
                     '    %snextTable.%s;\n'
                     '    %s%s%s\n'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, self.layer_name, ret_val, c_call, f_open, log_func, f_close, stmt))
        if 'WsiX11QueuePresent' == proto.name:
            funcs.append("#endif")
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "APIDump"
        body = [self._generate_layer_initialization_with_lock(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ApiDumpCppSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "xglLayer.h"\n#include "xgl_struct_string_helper_cpp.h"\n')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('static XGL_LAYER_DISPATCH_TABLE nextTable;')
        header_txt.append('static XGL_BASE_LAYER_OBJECT *pCurObj;\n')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;\n')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static loader_platform_thread_id tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t) maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        decl = proto.c_func(prefix="xgl", attr="XGLAPI")
        param0_name = proto.params[0].name
        ret_val = ''
        stmt = ''
        funcs = []
        sp_param_dict = {} # Store 'index' for struct param to print, or an name of binding "Count" param for array to print
        create_params = 0 # Num of params at end of function that are created and returned as output values
        if 'WsiX11CreatePresentableImage' in proto.name or 'AllocDescriptorSets' in proto.name:
            create_params = -2
        elif 'Create' in proto.name or 'Alloc' in proto.name or 'MapMemory' in proto.name:
            create_params = -1
        if proto.ret != "void":
            ret_val = "XGL_RESULT result = "
            stmt = "    return result;\n"
        f_open = ''
        f_close = ''
        if "File" in self.layer_name:
            file_mode = "a"
            if 'CreateDevice' in proto.name:
                file_mode = "w"
            f_open = 'loader_platform_thread_lock_mutex(&printLock);\n    pOutFile = fopen(outFileName, "%s");\n    ' % (file_mode)
            log_func = 'fprintf(pOutFile, "t{%%u} xgl%s(' % proto.name
            f_close = '\n    fclose(pOutFile);\n    loader_platform_thread_unlock_mutex(&printLock);'
        else:
            f_open = 'loader_platform_thread_lock_mutex(&printLock);\n    '
            log_func = 'cout << "t{" << getTIDIndex() << "} xgl%s(' % proto.name
            f_close = '\n    loader_platform_thread_unlock_mutex(&printLock);'
        pindex = 0
        prev_count_name = ''
        for p in proto.params:
            cp = False
            if 0 != create_params:
                # If this is any of the N last params of the func, treat as output
                for y in range(-1, create_params-1, -1):
                    if p.name == proto.params[y].name:
                        cp = True
            (pft, pfi) = self._get_printf_params(p.ty, p.name, cp, cpp=True)
            if self.no_addr and "%p" == pft:
                (pft, pfi) = ("%s", '"addr"')
            log_func += '%s = " << %s << ", ' % (p.name, pfi)
            #print_vals += ', %s' % (pfi)
            if prev_count_name != '' and (prev_count_name.strip('Count')[1:] in p.name or 'slotCount' == prev_count_name):
                sp_param_dict[pindex] = prev_count_name
            elif 'pDescriptorSets' == p.name and proto.params[-1].name == 'pCount':
                sp_param_dict[pindex] = '*pCount'
            elif 'Wsi' not in proto.name and xgl_helper.is_type(p.ty.strip('*').strip('const '), 'struct'):
                sp_param_dict[pindex] = 'index'
            pindex += 1
            if p.name.endswith('Count'):
                if '*' in p.ty:
                    prev_count_name = "*%s" % p.name
                else:
                    prev_count_name = p.name
            else:
                prev_count_name = ''
        log_func = log_func.strip(', ')
        if proto.ret != "void":
            log_func += ') = " << string_XGL_RESULT((XGL_RESULT)result) << endl'
            #print_vals += ', string_XGL_RESULT_CODE(result)'
        else:
            log_func += ')\\n"'
        log_func += ';'
        if len(sp_param_dict) > 0:
            i_decl = False
            log_func += '\n    string tmp_str;'
            for sp_index in sp_param_dict:
                if 'index' == sp_param_dict[sp_index]:
                    cis_print_func = 'xgl_print_%s' % (proto.params[sp_index].ty.strip('const ').strip('*').lower())
                    var_name = proto.params[sp_index].name
                    if proto.params[sp_index].name != 'color':
                        log_func += '\n    if (%s) {' % (proto.params[sp_index].name)
                    else:
                        var_name = '&%s' % (proto.params[sp_index].name)
                    log_func += '\n        tmp_str = %s(%s, "    ");' % (cis_print_func, var_name)
                    if "File" in self.layer_name:
                        if self.no_addr:
                            log_func += '\n        fprintf(pOutFile, "   %s (addr)\\n%%s\\n", pTmpStr);' % (proto.params[sp_index].name)
                        else:
                            log_func += '\n        fprintf(pOutFile, "   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, var_name)
                    else:
                        if self.no_addr:
                            #log_func += '\n        printf("   %s (addr)\\n%%s\\n", pTmpStr);' % (proto.params[sp_index].name)
                            log_func += '\n        cout << "   %s (addr)" << endl << tmp_str << endl;' % (proto.params[sp_index].name)
                        else:
                            #log_func += '\n        printf("   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                            log_func += '\n        cout << "   %s (" << %s << ")" << endl << tmp_str << endl;' % (proto.params[sp_index].name, var_name)
                        #log_func += '\n        fflush(stdout);'
                    if proto.params[sp_index].name != 'color':
                        log_func += '\n    }'
                else: # We have a count value stored to iterate over an array
                    print_cast = ''
                    print_func = ''
                    if xgl_helper.is_type(proto.params[sp_index].ty.strip('*').strip('const '), 'struct'):
                        print_cast = '&'
                        print_func = 'xgl_print_%s' % proto.params[sp_index].ty.strip('const ').strip('*').lower()
                        #cis_print_func = 'tmp_str = xgl_print_%s(&%s[i], "    ");' % (proto.params[sp_index].ty.strip('const ').strip('*').lower(), proto.params[sp_index].name)
# TODO : Need to display this address as a string
                    else:
                        print_cast = '(void*)'
                        print_func = 'string_convert_helper'
                        #cis_print_func = 'tmp_str = string_convert_helper((void*)%s[i], "    ");' % proto.params[sp_index].name
                    cis_print_func = 'tmp_str = %s(%s%s[i], "    ");' % (print_func, print_cast, proto.params[sp_index].name)
#                                else:
#                                    cis_print_func = ''
                    if not i_decl:
                        log_func += '\n    uint32_t i;'
                        i_decl = True
                    log_func += '\n    for (i = 0; i < %s; i++) {' % (sp_param_dict[sp_index])
                    log_func += '\n        %s' % (cis_print_func)
                    if "File" in self.layer_name:
                        if self.no_addr:
                            log_func += '\n        fprintf(pOutFile, "   %s (addr)\\n%%s\\n", pTmpStr);' % (proto.params[sp_index].name)
                        else:
                            log_func += '\n        fprintf(pOutFile, "   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                    else:
                        if self.no_addr:
                            #log_func += '\n        printf("   %s (addr)\\n%%s\\n", pTmpStr);' % (proto.params[sp_index].name)
                            log_func += '\n        cout << "   %s[" << (uint32_t)i << "] (addr)" << endl << tmp_str << endl;' % (proto.params[sp_index].name)
                        else:
                            #log_func += '\n        printf("   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                            #log_func += '\n        cout << "   %s[" << (uint32_t)i << "] (" << %s[i] << ")" << endl << tmp_str << endl;' % (proto.params[sp_index].name, proto.params[sp_index].name)
                            log_func += '\n        cout << "   %s[" << i << "] (" << %s%s[i] << ")" << endl << tmp_str << endl;' % (proto.params[sp_index].name, print_cast, proto.params[sp_index].name)
                        #log_func += '\n        fflush(stdout);'
                    log_func += '\n    }'
        if 'WsiX11AssociateConnection' == proto.name:
            funcs.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
        if proto.name == "EnumerateLayers":
            c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
            funcs.append('%s%s\n'
                     '{\n'
                     '    if (gpu != NULL) {\n'
                     '        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                     '        pCurObj = gpuw;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
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
                         '}' % (qual, decl, proto.params[0].name, self.layer_name, ret_val, c_call,f_open, log_func, f_close, stmt, self.layer_name))
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
                     '    loader_platform_thread_once(&tabOnce, init%s);\n'
                     '    %snextTable.%s;\n'
                     '    %s%s%s\n'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, self.layer_name, ret_val, c_call, f_open, log_func, f_close, stmt))
        if 'WsiX11QueuePresent' == proto.name:
            funcs.append("#endif")
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "APIDumpCpp"
        body = [self._generate_layer_initialization_with_lock(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

# subclass from ApiDumpSubcommand instead of Subcommand
class ApiDumpFileSubcommand(ApiDumpSubcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "xglLayer.h"\n#include "xgl_struct_string_helper.h"\n')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('static XGL_LAYER_DISPATCH_TABLE nextTable;')
        header_txt.append('static XGL_BASE_LAYER_OBJECT *pCurObj;\n')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;\n')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static loader_platform_thread_id tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t) maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}\n')
        header_txt.append('static FILE* pOutFile;\nstatic char* outFileName = "xgl_apidump.txt";')
        return "\n".join(header_txt)

    def generate_body(self):
        self.layer_name = "APIDumpFile"
        body = [self._generate_layer_initialization_with_lock(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

# subclass from ApiDumpSubcommand instead of Subcommand
class ApiDumpNoAddrSubcommand(ApiDumpSubcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "xglLayer.h"\n#include "xgl_struct_string_helper_no_addr.h"\n')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('static XGL_LAYER_DISPATCH_TABLE nextTable;')
        header_txt.append('static XGL_BASE_LAYER_OBJECT *pCurObj;\n')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;\n')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static loader_platform_thread_id tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t) maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_body(self):
        self.layer_name = "APIDumpNoAddr"
        self.no_addr = True
        body = [self._generate_layer_initialization_with_lock(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

# subclass from ApiDumpCppSubcommand instead of Subcommand
class ApiDumpNoAddrCppSubcommand(ApiDumpCppSubcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "xglLayer.h"\n#include "xgl_struct_string_helper_no_addr_cpp.h"\n')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('static XGL_LAYER_DISPATCH_TABLE nextTable;')
        header_txt.append('static XGL_BASE_LAYER_OBJECT *pCurObj;\n')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;\n')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static loader_platform_thread_id tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t) maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_body(self):
        self.layer_name = "APIDumpNoAddrCpp"
        self.no_addr = True
        body = [self._generate_layer_initialization_with_lock(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ObjectTrackerSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include "loader_platform.h"')
        header_txt.append('#include "object_track.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "layers_config.h"')
        header_txt.append('#include "layers_msg.h"')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static long long unsigned int object_track_index = 0;')
        header_txt.append('static int objLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex objLock;')
        header_txt.append('')
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
        header_txt.append('static uint32_t maxMemReferences = 0;')
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
        header_txt.append('static void ll_insert_obj(void* pObj, XGL_OBJECT_TYPE objType) {')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "OBJ[%llu] : CREATE %s object %p", object_track_index++, string_XGL_OBJECT_TYPE(objType), (void*)pObj);')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('    objNode* pNewObjNode = (objNode*)malloc(sizeof(objNode));')
        header_txt.append('    pNewObjNode->obj.pObj = pObj;')
        header_txt.append('    pNewObjNode->obj.objType = objType;')
        header_txt.append('    pNewObjNode->obj.status  = OBJSTATUS_NONE;')
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
        header_txt.append('static uint64_t ll_get_obj_uses(void* pObj, XGL_OBJECT_TYPE objType) {')
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
        header_txt.append('static void ll_increment_use_count(void* pObj, XGL_OBJECT_TYPE objType) {')
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
        header_txt.append('static void ll_remove_obj_type(void* pObj, XGL_OBJECT_TYPE objType) {')
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
        header_txt.append('static void ll_destroy_obj(void* pObj) {')
        header_txt.append('    objNode *pTrav = pGlobalHead;')
        header_txt.append('    objNode *pPrev = pGlobalHead;')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            ll_remove_obj_type(pObj, pTrav->obj.objType);')
        header_txt.append('            pPrev->pNextGlobal = pTrav->pNextGlobal;')
        header_txt.append('            // update HEAD of global list if needed')
        header_txt.append('            if (pGlobalHead == pTrav)')
        header_txt.append('                pGlobalHead = pTrav->pNextGlobal;')
        header_txt.append('            assert(numTotalObjs > 0);')
        header_txt.append('            numTotalObjs--;')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJ_STAT Removed %s obj %p that was used %lu times (%lu total objs remain & %lu %s objs).", string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, pTrav->obj.numUses, numTotalObjs, numObjs[pTrav->obj.objType], string_XGL_OBJECT_TYPE(pTrav->obj.objType));')
        header_txt.append('            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('            free(pTrav);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pPrev = pTrav;')
        header_txt.append('        pTrav = pTrav->pNextGlobal;')
        header_txt.append('    }')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to remove obj %p. Was it created? Has it already been destroyed?", pObj);')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_DESTROY_OBJECT_FAILED, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('// Set selected flag state for an object node')
        header_txt.append('static void set_status(void* pObj, XGL_OBJECT_TYPE objType, OBJECT_STATUS status_flag) {')
        header_txt.append('    if (pObj != NULL) {')
        header_txt.append('        objNode *pTrav = pObjectHead[objType];')
        header_txt.append('        while (pTrav) {')
        header_txt.append('            if (pTrav->obj.pObj == pObj) {')
        header_txt.append('                pTrav->obj.status |= status_flag;')
        header_txt.append('                return;')
        header_txt.append('            }')
        header_txt.append('            pTrav = pTrav->pNextObj;')
        header_txt.append('        }')
        header_txt.append('        // If we do not find it print an error')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Unable to set status for non-existent object %p of %s type", pObj, string_XGL_OBJECT_TYPE(objType));')
        header_txt.append('        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('    }');
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Track selected state for an object node')
        header_txt.append('static void track_object_status(void* pObj, XGL_STATE_BIND_POINT stateBindPoint) {')
        header_txt.append('    objNode *pTrav = pObjectHead[XGL_OBJECT_TYPE_CMD_BUFFER];')
        header_txt.append('')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            if (stateBindPoint == XGL_STATE_BIND_VIEWPORT) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_VIEWPORT_BOUND;')
        header_txt.append('            } else if (stateBindPoint == XGL_STATE_BIND_RASTER) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_RASTER_BOUND;')
        header_txt.append('            } else if (stateBindPoint == XGL_STATE_BIND_COLOR_BLEND) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_COLOR_BLEND_BOUND;')
        header_txt.append('            } else if (stateBindPoint == XGL_STATE_BIND_DEPTH_STENCIL) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_DEPTH_STENCIL_BOUND;')
        header_txt.append('            }')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    // If we do not find it print an error')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to track status for non-existent Command Buffer object %p", pObj);')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Reset selected flag state for an object node')
        header_txt.append('static void reset_status(void* pObj, XGL_OBJECT_TYPE objType, OBJECT_STATUS status_flag) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            pTrav->obj.status &= ~status_flag;')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    // If we do not find it print an error')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to reset status for non-existent object %p of %s type", pObj, string_XGL_OBJECT_TYPE(objType));')
        header_txt.append('    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Check object status for selected flag state')
        header_txt.append('static bool32_t validate_status(void* pObj, XGL_OBJECT_TYPE objType, OBJECT_STATUS status_mask, OBJECT_STATUS status_flag, XGL_DBG_MSG_TYPE error_level, OBJECT_TRACK_ERROR error_code, char* fail_msg) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.pObj == pObj) {')
        header_txt.append('            if ((pTrav->obj.status & status_mask) != status_flag) {')
        header_txt.append('                char str[1024];')
        header_txt.append('                sprintf(str, "OBJECT VALIDATION WARNING: %s object %p: %s", string_XGL_OBJECT_TYPE(objType), (void*)pObj, fail_msg);')
        header_txt.append('                layerCbMsg(error_level, XGL_VALIDATION_LEVEL_0, pObj, 0, error_code, "OBJTRACK", str);')
        header_txt.append('                return XGL_FALSE;')
        header_txt.append('            }')
        header_txt.append('            return XGL_TRUE;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    if (objType != XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY) {')
        header_txt.append('        // If we do not find it print an error')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Unable to obtain status for non-existent object %p of %s type", pObj, string_XGL_OBJECT_TYPE(objType));')
        header_txt.append('        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('    }')
        header_txt.append('    return XGL_FALSE;')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void validate_draw_state_flags(void* pObj) {')
        header_txt.append('    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_VIEWPORT_BOUND,      OBJSTATUS_VIEWPORT_BOUND,      XGL_DBG_MSG_ERROR,    OBJTRACK_VIEWPORT_NOT_BOUND,      "Viewport object not bound to this command buffer");')
        header_txt.append('    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_RASTER_BOUND,        OBJSTATUS_RASTER_BOUND,        XGL_DBG_MSG_ERROR,    OBJTRACK_RASTER_NOT_BOUND,        "Raster object not bound to this command buffer");')
        header_txt.append('    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_COLOR_BLEND_BOUND,   OBJSTATUS_COLOR_BLEND_BOUND,   XGL_DBG_MSG_UNKNOWN,  OBJTRACK_COLOR_BLEND_NOT_BOUND,   "Color-blend object not bound to this command buffer");')
        header_txt.append('    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_DEPTH_STENCIL_BOUND, OBJSTATUS_DEPTH_STENCIL_BOUND, XGL_DBG_MSG_UNKNOWN,  OBJTRACK_DEPTH_STENCIL_NOT_BOUND, "Depth-stencil object not bound to this command buffer");')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void validate_memory_mapping_status(const XGL_MEMORY_REF* pMemRefs, uint32_t numRefs) {')
        header_txt.append('    uint32_t i;')
        header_txt.append('    for (i = 0; i < numRefs; i++) {')
        header_txt.append('        if(pMemRefs[i].mem) {')
        header_txt.append('            // If mem reference is in presentable image memory list, skip the check of the GPU_MEMORY list')
        header_txt.append('            if (!validate_status((void *)pMemRefs[i].mem, XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY, OBJSTATUS_NONE, OBJSTATUS_NONE, XGL_DBG_MSG_UNKNOWN, OBJTRACK_NONE, NULL) == XGL_TRUE)')
        header_txt.append('            {')
        header_txt.append('                validate_status((void *)pMemRefs[i].mem, XGL_OBJECT_TYPE_GPU_MEMORY, OBJSTATUS_GPU_MEM_MAPPED, OBJSTATUS_NONE, XGL_DBG_MSG_ERROR, OBJTRACK_GPU_MEM_MAPPED, "A Mapped Memory Object was referenced in a command buffer");')
        header_txt.append('            }')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void validate_mem_ref_count(uint32_t numRefs) {')
        header_txt.append('    if (maxMemReferences == 0) {')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "xglQueueSubmit called before calling xglGetGpuInfo");')
        header_txt.append('        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, NULL, 0, OBJTRACK_GETGPUINFO_NOT_CALLED, "OBJTRACK", str);')
        header_txt.append('    } else {')
        header_txt.append('        if (numRefs > maxMemReferences) {')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "xglQueueSubmit Memory reference count (%d) exceeds allowable GPU limit (%d)", numRefs, maxMemReferences);')
        header_txt.append('            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, OBJTRACK_MEMREFCOUNT_MAX_EXCEEDED, "OBJTRACK", str);')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void setGpuQueueInfoState(void *pData) {')
        header_txt.append('    maxMemReferences = ((XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *)pData)->maxMemReferences;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback' ]:
            # use default version
            return None
        obj_type_mapping = {base_t : base_t.replace("XGL_", "XGL_OBJECT_TYPE_") for base_t in xgl.object_type_list}
        # For the various "super-types" we have to use function to distinguish sub type
        for obj_type in ["XGL_BASE_OBJECT", "XGL_OBJECT", "XGL_DYNAMIC_STATE_OBJECT"]:
            obj_type_mapping[obj_type] = "ll_get_obj_type(object)"

        decl = proto.c_func(prefix="xgl", attr="XGLAPI")
        param0_name = proto.params[0].name
        p0_type = proto.params[0].ty.strip('*').strip('const ')
        create_line = ''
        destroy_line = ''
        funcs = []
        # Special cases for API funcs that don't use an object as first arg
        if True in [no_use_proto in proto.name for no_use_proto in ['GlobalOption', 'CreateInstance', 'QueueSubmit', 'QueueAddMemReference', 'QueueRemoveMemReference', 'QueueWaitIdle', 'CreateDevice', 'GetGpuInfo', 'QueueSignalSemaphore', 'QueueWaitSemaphore', 'WsiX11QueuePresent']]:
            using_line = ''
        else:
            using_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    ll_increment_use_count((void*)%s, %s);\n' % (param0_name, obj_type_mapping[p0_type])
            using_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        if 'QueueSubmit' in proto.name:
            using_line += '    set_status((void*)fence, XGL_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED);\n'
            using_line += '    validate_memory_mapping_status(pMemRefs, memRefCount);\n'
            using_line += '    validate_mem_ref_count(memRefCount);\n'
        elif 'GetFenceStatus' in proto.name:
            using_line += '    // Warn if submitted_flag is not set\n'
            using_line += '    validate_status((void*)fence, XGL_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED, XGL_DBG_MSG_ERROR, OBJTRACK_INVALID_FENCE, "Status Requested for Unsubmitted Fence");\n'
        elif 'EndCommandBuffer' in proto.name:
            using_line += '    reset_status((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER, (OBJSTATUS_VIEWPORT_BOUND    |\n'
            using_line += '                                                                OBJSTATUS_RASTER_BOUND      |\n'
            using_line += '                                                                OBJSTATUS_COLOR_BLEND_BOUND |\n'
            using_line += '                                                                OBJSTATUS_DEPTH_STENCIL_BOUND));\n'
        elif 'CmdBindDynamicStateObject' in proto.name:
            using_line += '    track_object_status((void*)cmdBuffer, stateBindPoint);\n'
        elif 'CmdDraw' in proto.name:
            using_line += '    validate_draw_state_flags((void *)cmdBuffer);\n'
        elif 'MapMemory' in proto.name:
            using_line += '    set_status((void*)mem, XGL_OBJECT_TYPE_GPU_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);\n'
        elif 'UnmapMemory' in proto.name:
            using_line += '    reset_status((void*)mem, XGL_OBJECT_TYPE_GPU_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);\n'
        if 'AllocDescriptor' in proto.name: # Allocates array of DSs
            create_line =  '    for (uint32_t i = 0; i < *pCount; i++) {\n'
            create_line += '        loader_platform_thread_lock_mutex(&objLock);\n'
            create_line += '        ll_insert_obj((void*)pDescriptorSets[i], XGL_OBJECT_TYPE_DESCRIPTOR_SET);\n'
            create_line += '        loader_platform_thread_unlock_mutex(&objLock);\n'
            create_line += '    }\n'
        elif 'CreatePresentableImage' in proto.name:
            create_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
            create_line += '    ll_insert_obj((void*)*%s, %s);\n' % (proto.params[-2].name, obj_type_mapping[proto.params[-2].ty.strip('*').strip('const ')])
            create_line += '    ll_insert_obj((void*)*pMem, XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY);\n'
            # create_line += '    ll_insert_obj((void*)*%s, XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY);\n' % (obj_type_mapping[proto.params[-1].ty.strip('*').strip('const ')])
            create_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        elif 'Create' in proto.name or 'Alloc' in proto.name:
            create_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
            create_line += '    ll_insert_obj((void*)*%s, %s);\n' % (proto.params[-1].name, obj_type_mapping[proto.params[-1].ty.strip('*').strip('const ')])
            create_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        if 'DestroyObject' in proto.name:
            destroy_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
            destroy_line += '    ll_destroy_obj((void*)%s);\n' % (param0_name)
            destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
            using_line = ''
        else:
            if 'Destroy' in proto.name or 'Free' in proto.name:
                destroy_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
                destroy_line += '    ll_destroy_obj((void*)%s);\n' % (param0_name)
                destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
                using_line = ''
            if 'DestroyDevice' in proto.name:
                destroy_line += '    // Report any remaining objects in LL\n    objNode *pTrav = pGlobalHead;\n    while (pTrav) {\n'
                destroy_line += '        if (pTrav->obj.objType == XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY) {\n'
                destroy_line += '            objNode *pDel = pTrav;\n'
                destroy_line += '            pTrav = pTrav->pNextGlobal;\n'
                destroy_line += '            ll_destroy_obj((void*)(pDel->obj.pObj));\n'
                destroy_line += '        } else {\n'
                destroy_line += '            char str[1024];\n'
                destroy_line += '            sprintf(str, "OBJ ERROR : %s object %p has not been destroyed (was used %lu times).", string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, pTrav->obj.numUses);\n'
                destroy_line += '            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, device, 0, OBJTRACK_OBJECT_LEAK, "OBJTRACK", str);\n'
                destroy_line += '            pTrav = pTrav->pNextGlobal;\n'
                destroy_line += '        }\n'
                destroy_line += '    }\n'
        ret_val = ''
        stmt = ''
        if proto.ret != "void":
            ret_val = "XGL_RESULT result = "
            stmt = "    return result;\n"
        if 'WsiX11AssociateConnection' == proto.name:
            funcs.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
        if proto.name == "EnumerateLayers":
            c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
            funcs.append('%s%s\n'
                     '{\n'
                     '    if (gpu != NULL) {\n'
                     '        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                     '    %s'
                     '        pCurObj = gpuw;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
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
                         '}' % (qual, decl, proto.params[0].name, using_line, self.layer_name, ret_val, c_call, create_line, destroy_line, stmt, self.layer_name))
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
            gpu_state = ''
            if 'GetGpuInfo' in proto.name:
                gpu_state =  '    if (infoType == XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES) {\n'
                gpu_state += '        if (pData != NULL) {\n'
                gpu_state += '            setGpuQueueInfoState(pData);\n'
                gpu_state += '        }\n'
                gpu_state += '    }\n'
            funcs.append('%s%s\n'
                     '{\n'
                     '    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                     '%s'
                     '    pCurObj = gpuw;\n'
                     '    loader_platform_thread_once(&tabOnce, init%s);\n'
                     '    %snextTable.%s;\n'
                     '%s%s'
                     '%s'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, using_line, self.layer_name, ret_val, c_call, create_line, destroy_line, gpu_state, stmt))
        if 'WsiX11QueuePresent' == proto.name:
            funcs.append("#endif")
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "ObjectTracker"
        body = [self._generate_layer_initialization(True, lockname='obj'),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT"),
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
            "ApiDumpNoAddr" : ApiDumpNoAddrSubcommand,
            "ApiDumpCpp" : ApiDumpCppSubcommand,
            "ApiDumpNoAddrCpp" : ApiDumpNoAddrCppSubcommand,
            "ObjectTracker" : ObjectTrackerSubcommand,
    }

    if len(sys.argv) < 3 or sys.argv[1] not in subcommands or not os.path.exists(sys.argv[2]):
        print("Usage: %s <subcommand> <input_header> [options]" % sys.argv[0])
        print
        print("Available subcommands are: %s" % " ".join(subcommands))
        exit(1)

    hfp = xgl_helper.HeaderFileParser(sys.argv[2])
    hfp.parse()
    xgl_helper.enum_val_dict = hfp.get_enum_val_dict()
    xgl_helper.enum_type_dict = hfp.get_enum_type_dict()
    xgl_helper.struct_dict = hfp.get_struct_dict()
    xgl_helper.typedef_fwd_dict = hfp.get_typedef_fwd_dict()
    xgl_helper.typedef_rev_dict = hfp.get_typedef_rev_dict()
    xgl_helper.types_dict = hfp.get_types_dict()

    subcmd = subcommands[sys.argv[1]](sys.argv[2:])
    subcmd.run()

if __name__ == "__main__":
    main()
