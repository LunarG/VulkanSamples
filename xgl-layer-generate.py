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
    def _get_printf_params(self, xgl_type, name, last_create):
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
        if "BOOL" in xgl_type:
            return ("%u", name)
        if True in [t in xgl_type for t in ["INT", "SIZE", "FLAGS", "MASK"]]:
            if '[' in xgl_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                return ("[%i, %i, %i, %i]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name))
            if '*' in xgl_type:
                return ("%i", "*%s" % name)
            return ("%i", name)
        if last_create:
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

    def _generate_dispatch_entrypoints(self, qual="", layer="generic"):
        if qual:
            qual += " "

        funcs = []
        for proto in self.protos:
            if proto.name != "GetProcAddr" and proto.name != "InitAndEnumerateGpus":
                if "generic" == layer:
                    decl = proto.c_func(prefix="xgl", attr="XGLAPI")
                    param0_name = proto.params[0].name
                    ret_val = ''
                    stmt = ''
                    if proto.ret != "XGL_VOID":
                        ret_val = "XGL_RESULT result = "
                        stmt = "    return result;\n"
                    if proto.params[0].ty != "XGL_PHYSICAL_GPU":
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
                                 '%s'
                                 '}' % (qual, decl, proto.params[0].name, proto.name, ret_val, c_call, proto.name, stmt))
                elif "apidump" == layer:
                    decl = proto.c_func(prefix="xgl", attr="XGLAPI")
                    param0_name = proto.params[0].name
                    ret_val = ''
                    stmt = ''
                    cis_param_index = [] # Store list of indices when func has struct params
                    create_func = False
                    if 'Create' in proto.name:
                        create_func = True
                    if proto.ret != "XGL_VOID":
                        ret_val = "XGL_RESULT result = "
                        stmt = "    return result;\n"
                    log_func = 'printf("xgl%s(' % proto.name
                    print_vals = ''
                    pindex = 0
                    for p in proto.params:
                        if p.name == proto.params[-1].name and create_func: # last param of create func
                            (pft, pfi) = self._get_printf_params(p.ty, p.name, True)
                        else:
                            (pft, pfi) = self._get_printf_params(p.ty, p.name, False)
                        log_func += '%s = %s, ' % (p.name, pft)
                        print_vals += ', %s' % (pfi)
                        # TODO : Just want this to be simple check for params of STRUCT type
                        if "pCreateInfo" in p.name or ('const' in p.ty and '*' in p.ty and False not in [tmp_ty not in p.ty for tmp_ty in ['XGL_CHAR', 'XGL_VOID', 'XGL_CMD_BUFFER', 'XGL_QUEUE_SEMAPHORE', 'XGL_FENCE', 'XGL_SAMPLER', 'XGL_UINT32']]):
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
                            log_func += '\n        printf("   %s (%%p)\\n%%s\\n", (void*)%s, pTmpStr);' % (proto.params[sp_index].name, proto.params[sp_index].name)
                            log_func += '\n        free(pTmpStr);\n    }'
                    if proto.params[0].ty != "XGL_PHYSICAL_GPU":
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    %snextTable.%s;\n'
                                 '    %s\n'
                                 '%s'
                                 '}' % (qual, decl, ret_val, proto.c_call(), log_func, stmt))
                    else:
                        c_call = proto.c_call().replace("(" + proto.params[0].name, "((XGL_PHYSICAL_GPU)gpuw->nextObject", 1)
                        funcs.append('%s%s\n'
                                 '{\n'
                                 '    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) %s;\n'
                                 '    pCurObj = gpuw;\n'
                                 '    pthread_once(&tabOnce, initLayerTable);\n'
                                 '    %snextTable.%s;\n'
                                 '    %s\n'
                                 '%s'
                                 '}' % (qual, decl, proto.params[0].name, ret_val, c_call, log_func, stmt))
                elif "objecttracker" == layer:
                    decl = proto.c_func(prefix="xgl", attr="XGLAPI")
                    param0_name = proto.params[0].name
                    create_line = ''
                    destroy_line = ''
                    using_line = '    ll_increment_use_count((XGL_VOID*)%s);\n    printf("OBJ[%%llu] : USING %s object %%p (%%lu total uses)\\n", object_track_index++, (void*)%s, ll_get_obj_uses((XGL_VOID*)%s));\n' % (param0_name, param0_name, param0_name, param0_name)
                    if 'Create' in proto.name:
                        create_line = '    printf("OBJ[%%llu] : CREATE %s object %%p\\n", object_track_index++, (void*)*%s);\n    ll_insert_obj((XGL_VOID*)*%s, "%s");\n' % (proto.params[-1].ty.strip('*'), proto.params[-1].name, proto.params[-1].name, proto.params[-1].ty.strip('*'))
                    if 'Destroy' in proto.name:
                        destroy_line = '    printf("OBJ[%%llu] : DESTROY %s object %%p\\n", object_track_index++, (void*)%s);\n    ll_remove_obj((XGL_VOID*)%s);\n' % (param0_name, param0_name, param0_name)
                        using_line = ''
                    ret_val = ''
                    stmt = ''
                    if proto.ret != "XGL_VOID":
                        ret_val = "XGL_RESULT result = "
                        stmt = "    return result;\n"
                    if proto.params[0].ty != "XGL_PHYSICAL_GPU":
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

    def _generate_layer_gpa_function(self, prefix="xgl"):
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
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "generic"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ApiDumpSubcommand(Subcommand):
    def generate_header(self):
        return '#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n#include <pthread.h>\n#include "xglLayer.h"\n#include "xgl_string_helper.h"\n#include "xgl_struct_string_helper.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;\nstatic pthread_once_t tabOnce = PTHREAD_ONCE_INIT;\n'

    def generate_body(self):
        body = [self._generate_layer_dispatch_table(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "apidump"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class ObjectTrackerSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n#include <pthread.h>')
        header_txt.append('#include "xglLayer.h"\n\nstatic XGL_LAYER_DISPATCH_TABLE nextTable;\nstatic XGL_BASE_LAYER_OBJECT *pCurObj;')
        header_txt.append('static pthread_once_t tabOnce = PTHREAD_ONCE_INIT;\nstatic long long unsigned int object_track_index = 0;\n')
        header_txt.append('typedef struct _objNode {')
        header_txt.append('    XGL_VOID        *pObj;')
        header_txt.append('    const char      *objType;')
        header_txt.append('    uint64_t        numUses;')
        header_txt.append('    struct _objNode *pNext;')
        header_txt.append('} objNode;\n')
        header_txt.append('static objNode *pObjLLHead = NULL;\n')
        header_txt.append('static void ll_insert_obj(XGL_VOID* pObj, const char* type) {')
        header_txt.append('    objNode* pNewObjNode = (objNode*)malloc(sizeof(objNode));')
        header_txt.append('    pNewObjNode->pObj = pObj;')
        header_txt.append('    pNewObjNode->objType = type;')
        header_txt.append('    pNewObjNode->numUses = 0;')
        header_txt.append('    pNewObjNode->pNext = pObjLLHead;')
        header_txt.append('    pObjLLHead = pNewObjNode;')
        header_txt.append('}\n')
        header_txt.append('static void ll_increment_use_count(XGL_VOID* pObj) {')
        header_txt.append('    objNode *pTrav = pObjLLHead;')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->pObj == pObj) {')
        header_txt.append('            pTrav->numUses++;')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNext;')
        header_txt.append('    }')
        header_txt.append('    // If we do not find obj, insert it and then intrement count')
        header_txt.append('    printf("INFO : Unable to increment count for obj %p, will add to list as UNKNOWN type and increment count\\n", pObj);')
        header_txt.append('    ll_insert_obj(pObj, "UNKNOWN");')
        header_txt.append('    ll_increment_use_count(pObj);')
        header_txt.append('}')
        header_txt.append('static uint64_t ll_get_obj_uses(XGL_VOID* pObj) {')
        header_txt.append('    objNode *pTrav = pObjLLHead;')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->pObj == pObj) {')
        header_txt.append('            return pTrav->numUses;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNext;')
        header_txt.append('    }')
        header_txt.append('    return 0;')
        header_txt.append('}')
        header_txt.append('static void ll_remove_obj(XGL_VOID* pObj) {')
        header_txt.append('    objNode *pTrav = pObjLLHead;')
        header_txt.append('    objNode *pPrev = pObjLLHead;')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->pObj == pObj) {')
        header_txt.append('            pPrev->pNext = pTrav->pNext;')
        header_txt.append('            if (pObjLLHead == pTrav)')
        header_txt.append('                pObjLLHead = pTrav->pNext;')
        header_txt.append('            printf("OBJ_STAT Removed %s obj %p that was used %lu times.\\n", pTrav->objType, pTrav->pObj, pTrav->numUses);')
        header_txt.append('            free(pTrav);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pPrev = pTrav;')
        header_txt.append('        pTrav = pTrav->pNext;')
        header_txt.append('    }')
        header_txt.append('    printf("ERROR : Unable to remove obj %p\\n", pObj);')
        header_txt.append('}')

        return "\n".join(header_txt)

    def generate_body(self):
        body = [self._generate_layer_dispatch_table(),
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", "objecttracker"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)
    
def main():
    subcommands = {
            "layer-funcs" : LayerFuncsSubcommand,
            "layer-dispatch" : LayerDispatchSubcommand,
            "generic-layer" : GenericLayerSubcommand,
            "api-dump" : ApiDumpSubcommand,
            "object-tracker" : ObjectTrackerSubcommand,
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
