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

def generate_get_proc_addr_check(name):
    return "    if (!%s || %s[0] != 'x' || %s[1] != 'g' || %s[2] != 'l')\n" \
           "        return NULL;" % ((name,) * 4)

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

class LoaderEntrypointsSubcommand(Subcommand):
    def generate_header(self):
        return "#include \"loader.h\""

    def _does_function_create_object(self, proto):
        out_objs = proto.object_out_params()
        return out_objs and out_objs[-1] == proto.params[-1]

    def _is_dispatchable(self, proto):
        if proto.name in ["GetProcAddr", "DestroyInstance", "EnumerateGpus", "EnumerateLayers"]:
            return False

        in_objs = proto.object_in_params()
        return in_objs and in_objs[0] == proto.params[0]

    def _generate_loader_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        for proto in self.protos:
            if not self._is_dispatchable(proto):
                continue
            if 'WsiX11AssociateConnection' == proto.name:
                funcs.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
            decl = proto.c_func(prefix="xgl", attr="XGLAPI")
            stmt = "disp->%s" % proto.c_call()
            if proto.name == "CreateDevice":
                funcs.append("%s%s\n"
                         "{\n"
                         "    loader_activate_layers(%s, %s);\n"
                         "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_unwrap_gpu(&%s);\n"
                         "    XGL_RESULT res = %s;\n"
                         "    if (res == XGL_SUCCESS) {\n"
                         "        loader_init_data(*%s, disp);\n"
                         "    }\n"
                         "    return res;\n"
                         "}" % (qual, decl, proto.params[0].name, proto.params[1].name,
                                proto.params[0].name, stmt, proto.params[-1].name))
            elif proto.name == "WsiX11CreatePresentableImage":
                funcs.append("%s%s\n"
                        "{\n"
                        "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_get_data(%s);\n"
                        "    XGL_RESULT res = %s;\n"
                        "    if (res == XGL_SUCCESS) {\n"
                        "        loader_init_data(*%s, disp);\n"
                        "        loader_init_data(*%s, disp);\n"
                        "    }\n"
                        "    return res;\n"
                        "}"
                        % (qual, decl, proto.params[0].name, stmt, proto.params[-1].name, proto.params[-2].name))
            elif proto.name == "GetDeviceQueue":
                # GetDeviceQueue returns an existing Queue object so cannot check for magic header as
                # it may have already been replaced with a function table pointer
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_get_data(%s);\n"
                         "    XGL_RESULT res = %s;\n"
                         "    if (res == XGL_SUCCESS) {\n"
                         "        loader_set_data(*%s, disp);\n"
                         "    }\n"
                         "    return res;\n"
                         "}"
                         % (qual, decl, proto.params[0].name, stmt, proto.params[-1].name))
            elif self._does_function_create_object(proto):
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_get_data(%s);\n"
                         "    XGL_RESULT res = %s;\n"
                         "    if (res == XGL_SUCCESS) {\n"
                         "        loader_init_data(*%s, disp);\n"
                         "    }\n"
                         "    return res;\n"
                         "}"
                         % (qual, decl, proto.params[0].name, stmt, proto.params[-1].name))
            elif proto.name == "AllocDescriptorSets":
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_get_data(%s);\n"
                         "    uint32_t i;\n"
                         "    XGL_RESULT res = %s;\n"
                         "    for (i = 0; i < *%s; i++) {\n"
                         "        loader_init_data(%s[i], disp);\n"
                         "    }\n"
                         "    return res;\n"
                         "}" % (qual, decl, proto.params[0].name, stmt, proto.params[-1].name, proto.params[-2].name))
            elif proto.name == "GetMultiGpuCompatibility":
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_unwrap_gpu(&%s);\n"
                         "    loader_unwrap_gpu(&%s);\n"
                         "    return %s;\n"
                         "}" % (qual, decl, proto.params[0].name, proto.params[1].name,
                                stmt))
            elif proto.params[0].ty != "XGL_PHYSICAL_GPU":
                if proto.ret != "void":
                    stmt = "return " + stmt
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_get_data(%s);\n"
                         "    %s;\n"
                         "}" % (qual, decl, proto.params[0].name, stmt))
            else:
                if proto.ret != "void":
                    stmt = "return " + stmt
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE *disp = loader_unwrap_gpu(&%s);\n"
                         "    %s;\n"
                             "}" % (qual, decl, proto.params[0].name, stmt))

        funcs.append("#endif")
        return "\n\n".join(funcs)

    def generate_body(self):
        body = [self._generate_loader_dispatch_entrypoints("LOADER_EXPORT")]

        return "\n\n".join(body)

class DispatchTableOpsSubcommand(Subcommand):
    def run(self):
        if len(self.argv) != 1:
            print("DispatchTableOpsSubcommand: <prefix> unspecified")
            return

        self.prefix = self.argv[0]
        super().run()

    def generate_header(self):
        return "\n".join(["#include <xgl.h>",
                          "#include <xglLayer.h>",
                          "#include <string.h>",
                          "#include \"loader_platform.h\""])

    def _generate_init(self):
        stmts = []
        for proto in self.protos:
            if 'WsiX11AssociateConnection' == proto.name:
                stmts.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
            if proto.name == "GetProcAddr":
                stmts.append("table->%s = gpa; /* direct assignment */" %
                        proto.name)
            else:
                stmts.append("table->%s = (xgl%sType) gpa(gpu, \"xgl%s\");" %
                        (proto.name, proto.name, proto.name))
        stmts.append("#endif")

        func = []
        func.append("static inline void %s_initialize_dispatch_table(XGL_LAYER_DISPATCH_TABLE *table,"
                % self.prefix)
        func.append("%s                                              xglGetProcAddrType gpa,"
                % (" " * len(self.prefix)))
        func.append("%s                                              XGL_PHYSICAL_GPU gpu)"
                % (" " * len(self.prefix)))
        func.append("{")
        func.append("    %s" % "\n    ".join(stmts))
        func.append("}")

        return "\n".join(func)

    def _generate_lookup(self):
        lookups = []
        for proto in self.protos:
            if 'WsiX11AssociateConnection' == proto.name:
                lookups.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
            lookups.append("if (!strcmp(name, \"%s\"))" % (proto.name))
            lookups.append("    return (void *) table->%s;"
                    % (proto.name))
        lookups.append("#endif")

        func = []
        func.append("static inline void *%s_lookup_dispatch_table(const XGL_LAYER_DISPATCH_TABLE *table,"
                % self.prefix)
        func.append("%s                                           const char *name)"
                % (" " * len(self.prefix)))
        func.append("{")
        func.append(generate_get_proc_addr_check("name"))
        func.append("")
        func.append("    name += 3;")
        func.append("    %s" % "\n    ".join(lookups))
        func.append("")
        func.append("    return NULL;")
        func.append("}")

        return "\n".join(func)

    def generate_body(self):
        body = [self._generate_init(),
                self._generate_lookup()]

        return "\n\n".join(body)

class IcdDummyEntrypointsSubcommand(Subcommand):
    def run(self):
        if len(self.argv) == 1:
            self.prefix = self.argv[0]
            self.qual = "static"
        else:
            self.prefix = "xgl"
            self.qual = "ICD_EXPORT"

        super().run()

    def generate_header(self):
        return "#include \"icd.h\""

    def _generate_stub_decl(self, proto):
        return proto.c_pretty_decl(self.prefix + proto.name, attr="XGLAPI")

    def _generate_stubs(self):
        stubs = []
        for proto in self.protos:
            decl = self._generate_stub_decl(proto)
            if proto.ret != "void":
                stmt = "    return XGL_ERROR_UNKNOWN;\n"
            else:
                stmt = ""

            stubs.append("%s %s\n{\n%s}" % (self.qual, decl, stmt))

        return "\n\n".join(stubs)

    def generate_body(self):
        return self._generate_stubs()

class IcdGetProcAddrSubcommand(IcdDummyEntrypointsSubcommand):
    def generate_header(self):
        return "\n".join(["#include <string.h>", "#include \"icd.h\""])

    def generate_body(self):
        for proto in self.protos:
            if proto.name == "GetProcAddr":
                gpa_proto = proto

        gpa_decl = self._generate_stub_decl(gpa_proto)
        gpa_pname = gpa_proto.params[-1].name

        lookups = []
        for proto in self.protos:
            if 'WsiX11AssociateConnection' == proto.name:
                lookups.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
            lookups.append("if (!strcmp(%s, \"%s\"))" %
                    (gpa_pname, proto.name))
            lookups.append("    return (%s) %s%s;" %
                    (gpa_proto.ret, self.prefix, proto.name))
        lookups.append("#endif")

        body = []
        body.append("%s %s" % (self.qual, gpa_decl))
        body.append("{")
        body.append(generate_get_proc_addr_check(gpa_pname))
        body.append("")
        body.append("    %s += 3;" % gpa_pname)
        body.append("    %s" % "\n    ".join(lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")

        return "\n".join(body)

class LayerInterceptProcSubcommand(Subcommand):
    def run(self):
        self.prefix = "xgl"

        # we could get the list from argv if wanted
        self.intercepted = [proto.name for proto in self.protos
                if proto.name not in ["EnumerateGpus"]]

        for proto in self.protos:
            if proto.name == "GetProcAddr":
                self.gpa = proto

        super().run()

    def generate_header(self):
        return "\n".join(["#include <string.h>", "#include \"xglLayer.h\""])

    def generate_body(self):
        lookups = []
        for proto in self.protos:
            if proto.name not in self.intercepted:
                lookups.append("/* no %s%s */" % (self.prefix, proto.name))
                continue

            if 'WsiX11AssociateConnection' == proto.name:
                lookups.append("#if defined(__linux__) || defined(XCB_NVIDIA)")
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (%s) %s%s;" %
                    (self.gpa.ret, self.prefix, proto.name))
        lookups.append("#endif")

        body = []
        body.append("static inline %s layer_intercept_proc(const char *name)" %
                self.gpa.ret)
        body.append("{")
        body.append(generate_get_proc_addr_check("name"))
        body.append("")
        body.append("    name += 3;")
        body.append("    %s" % "\n    ".join(lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")

        return "\n".join(body)

class WinDefFileSubcommand(Subcommand):
    def run(self):
        library_exports = {
                "all": [],
                "icd": [
                    "EnumerateGpus",
                    "CreateInstance",
                    "DestroyInstance",
                    "GetProcAddr",
                ],
                "layer": [
                    "GetProcAddr",
                    "EnumerateLayers",
                ],
        }

        if len(self.argv) != 2 or self.argv[1] not in library_exports:
            print("WinDefFileSubcommand: <library-name> {%s}" %
                    "|".join(library_exports.keys()))
            return

        self.library = self.argv[0]
        self.exports = library_exports[self.argv[1]]

        super().run()

    def generate_copyright(self):
        return """; THIS FILE IS GENERATED.  DO NOT EDIT.

;;;; Begin Copyright Notice ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XGL
;
; Copyright (C) 2015 LunarG, Inc.
;
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the "Software"),
; to deal in the Software without restriction, including without limitation
; the rights to use, copy, modify, merge, publish, distribute, sublicense,
; and/or sell copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
; THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
; DEALINGS IN THE SOFTWARE.
;;;;  End Copyright Notice ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;"""

    def generate_header(self):
        return "; The following is required on Windows, for exporting symbols from the DLL"

    def generate_body(self):
        body = []

        body.append("LIBRARY " + self.library)
        body.append("EXPORTS")

        for proto in self.protos:
            if self.exports and proto.name not in self.exports:
                continue
            body.append("   xgl" + proto.name)

        return "\n".join(body)

def main():
    subcommands = {
            "loader-entrypoints": LoaderEntrypointsSubcommand,
            "dispatch-table-ops": DispatchTableOpsSubcommand,
            "icd-dummy-entrypoints": IcdDummyEntrypointsSubcommand,
            "icd-get-proc-addr": IcdGetProcAddrSubcommand,
            "layer-intercept-proc": LayerInterceptProcSubcommand,
            "win-def-file": WinDefFileSubcommand,
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
