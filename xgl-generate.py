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

    def _generate_dispatch_entrypoints(self, qual="", unwrap=False, layer=False):
        if qual:
            qual += " "

        funcs = []
        for proto in self.protos:
            if not layer:
                if not xgl.is_dispatchable(proto):
                    continue
                decl = proto.c_func(prefix="xgl", attr="XGLAPI")
                stmt = "(*disp)->%s" % proto.c_call()
                if proto.ret != "XGL_VOID":
                    stmt = "return " + stmt
                if proto.name == "CreateDevice" and qual == "LOADER_EXPORT ":
                    stmt_cd = "XGL_RESULT res = " + "(*disp)->%s" % proto.c_call()
                    funcs.append("%s%s\n"
                             "{\n"
                             "    ActivateLayers(&%s);\n"
                             "    XGL_BASE_LAYER_OBJECT* wrapped_obj = (XGL_BASE_LAYER_OBJECT*)%s;\n"
                             "    const XGL_LAYER_DISPATCH_TABLE * const *disp =\n"
                             "            (const XGL_LAYER_DISPATCH_TABLE * const *) wrapped_obj->baseObject;\n"
                             "    %s = wrapped_obj->nextObject;\n"
                             "    %s;\n"
                             "    const XGL_LAYER_DISPATCH_TABLE * *disp_dev = (const XGL_LAYER_DISPATCH_TABLE *  *) *%s;\n"
                             "    *disp_dev = (const XGL_LAYER_DISPATCH_TABLE *) *disp;\n"
                             "    return res;\n"
                             "}" % (qual, decl, proto.params[0].name, proto.params[0].name, proto.params[0].name, stmt_cd, proto.params[2].name))
                elif proto.params[0].ty != "XGL_PHYSICAL_GPU":
                    funcs.append("%s%s\n"
                             "{\n"
                             "    const XGL_LAYER_DISPATCH_TABLE * const *disp =\n"
                             "        (const XGL_LAYER_DISPATCH_TABLE * const *) %s;\n"
                             "    %s;\n"
                             "}" % (qual, decl, proto.params[0].name, stmt))
                else:
                    funcs.append("%s%s\n"
                             "{\n"
                             "    XGL_BASE_LAYER_OBJECT* wrapped_obj = (XGL_BASE_LAYER_OBJECT*)%s;\n"
                             "    const XGL_LAYER_DISPATCH_TABLE * const *disp =\n"
                             "        (const XGL_LAYER_DISPATCH_TABLE * const *) wrapped_obj->baseObject;\n"
                             "    %s = wrapped_obj->nextObject;\n"
                             "    %s;\n"
                                 "}" % (qual, decl, proto.params[0].name, proto.params[0].name, stmt))
            elif proto.name != "GetProcAddr" and proto.name != "InitAndEnumerateGpus":
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

class LoaderSubcommand(Subcommand):
    def generate_header(self):
        return "#include \"loader.h\""

    def generate_body(self):
        body = [self._generate_dispatch_entrypoints("LOADER_EXPORT")]

        return "\n\n".join(body)

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
                self._generate_dispatch_entrypoints("XGL_LAYER_EXPORT", True, True),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class IcdDispatchTableSubcommand(Subcommand):
    def generate_body(self):
        return self._generate_icd_dispatch_table()

class IcdDispatchEntrypointsSubcommand(Subcommand):
    def generate_header(self):
        return "#include \"icd.h\""

    def generate_body(self):
        return self._generate_dispatch_entrypoints("ICD_EXPORT")

class IcdDispatchDummyImplSubcommand(Subcommand):
    def run(self):
        if len(self.argv) != 1:
            print("IcdDispatchDummyImplSubcommand: <prefix> unspecified")
            return

        self.prefix = self.argv[0]

        super().run()

    def generate_header(self):
        return "#include \"icd.h\""

    def _generate_stub_decl(self, proto):
        plist = []
        for param in proto.params:
            idx = param.ty.find("[")
            if idx < 0:
                idx = len(param.ty)

            pad = 44 - idx
            if pad <= 0:
                pad = 1

            plist.append("    %s%s%s%s" % (param.ty[:idx],
                " " * pad, param.name, param.ty[idx:]))

        return "%s XGLAPI %s%s(\n%s)" % (proto.ret, self.prefix,
                proto.name, ",\n".join(plist))

    def _generate_stubs(self):
        stubs = []
        for proto in self.protos:
            if not xgl.is_dispatchable(proto):
                continue

            decl = self._generate_stub_decl(proto)
            if proto.ret != "XGL_VOID":
                stmt = "    return XGL_ERROR_UNAVAILABLE;\n"
            else:
                stmt = ""

            stubs.append("static %s\n{\n%s}" % (decl, stmt))

        return "\n\n".join(stubs)


    def _generate_tables(self):
        initializer = []
        for proto in self.protos:
            prefix = self.prefix if xgl.is_dispatchable(proto) else "xgl"
            initializer.append(".%s = %s%s" %
                    (proto.name, prefix, proto.name))

        return """const struct icd_dispatch_table %s_normal_dispatch_table = {
    %s,
};

const struct icd_dispatch_table %s_debug_dispatch_table = {
    %s,
};""" % (self.prefix, ",\n    ".join(initializer),
         self.prefix, ",\n    ".join(initializer))

    def generate_body(self):
        body = [self._generate_stubs(),
                self._generate_tables()]

        return "\n\n".join(body)

def main():
    subcommands = {
            "loader": LoaderSubcommand,
            "layer-funcs" : LayerFuncsSubcommand,
            "layer-dispatch" : LayerDispatchSubcommand,
            "generic-layer" : GenericLayerSubcommand,
            "icd-dispatch-table": IcdDispatchTableSubcommand,
            "icd-dispatch-entrypoints": IcdDispatchEntrypointsSubcommand,
            "icd-dispatch-dummy-impl": IcdDispatchDummyImplSubcommand,
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
