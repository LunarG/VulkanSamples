#!/usr/bin/env python3
#
# VK
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

import vulkan

def generate_get_proc_addr_check(name):
    return "    if (!%s || %s[0] != 'v' || %s[1] != 'k')\n" \
           "        return NULL;" % ((name,) * 3)


class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.headers = vulkan.headers
        self.protos = vulkan.protos

    def run(self):
        print(self.generate())

    def is_dispatchable_object_first_param(self, proto):
        in_objs = proto.object_in_params()
        non_dispatch_objs = ["VkInstance"]
        param0 = proto.params[0]
        return (len(in_objs) > 0)  and (in_objs[0].ty == param0.ty) and (param0.ty not in non_dispatch_objs)

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

class LoaderEntrypointsSubcommand(Subcommand):
    def generate_header(self):
        return "#include \"loader.h\""

    def _is_loader_special_case(self, proto):
        if proto.name in ["GetProcAddr", "EnumerateGpus", "EnumerateLayers"]:
            return True

        return not self.is_dispatchable_object_first_param(proto)

    def _generate_object_setup(self, proto):
        method = "loader_init_data"
        cond = "res == VK_SUCCESS"

        if "Get" in proto.name:
            method = "loader_set_data"

        setup = []

        if proto.name == "AllocDescriptorSets":
            psets = proto.params[-2].name
            pcount = proto.params[-1].name
            setup.append("uint32_t i;")
            setup.append("for (i = 0; i < *%s; i++)" % pcount)
            setup.append("    %s(%s[i], disp);" % (method, psets))
        else:
            obj_params = proto.object_out_params()
            for param in obj_params:
                setup.append("%s(*%s, disp);" % (method, param.name))

        if setup:
            joined = "\n        ".join(setup)
            setup = []
            setup.append("    if (%s) {" % cond)
            setup.append("        " + joined)
            setup.append("    }")

        return "\n".join(setup)

    def _generate_loader_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        for proto in self.protos:
            if self._is_loader_special_case(proto):
                continue
            func = []

            obj_setup = self._generate_object_setup(proto)

            func.append(qual + proto.c_func(prefix="vk", attr="VKAPI"))
            func.append("{")

            # declare local variables
            func.append("    const VkLayerDispatchTable *disp;")
            if proto.ret != 'void' and obj_setup:
                func.append("    VkResult res;")
            func.append("")

            # get dispatch table
            func.append("    disp = loader_get_data(%s);" %
                    proto.params[0].name)
            func.append("")

            # dispatch!
            dispatch = "disp->%s;" % proto.c_call()
            if proto.ret == 'void':
                func.append("    " + dispatch)
            elif not obj_setup:
                func.append("    return " + dispatch)
            else:
                func.append("    res = " + dispatch)
                func.append(obj_setup)
                func.append("")
                func.append("    return res;")

            func.append("}")

            if 'WsiX11AssociateConnection' == proto.name:
                funcs.append("#if defined(__linux__) || defined(XCB_NVIDIA)")

            funcs.append("\n".join(func))

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
        return "\n".join(["#include <vulkan.h>",
                          "#include <vkLayer.h>",
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
            elif self.is_dispatchable_object_first_param(proto):
                stmts.append("table->%s = (PFN_vk%s) gpa(gpu, \"vk%s\");" %
                        (proto.name, proto.name, proto.name))
        stmts.append("#endif")

        func = []
        func.append("static inline void %s_initialize_dispatch_table(VkLayerDispatchTable *table,"
                % self.prefix)
        func.append("%s                                              PFN_vkGetProcAddr gpa,"
                % (" " * len(self.prefix)))
        func.append("%s                                              VkPhysicalGpu gpu)"
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
            if self.is_dispatchable_object_first_param(proto):
                lookups.append("if (!strcmp(name, \"%s\"))" % (proto.name))
                lookups.append("    return (void *) table->%s;"
                    % (proto.name))
        lookups.append("#endif")

        func = []
        func.append("static inline void *%s_lookup_dispatch_table(const VkLayerDispatchTable *table,"
                % self.prefix)
        func.append("%s                                           const char *name)"
                % (" " * len(self.prefix)))
        func.append("{")
        func.append(generate_get_proc_addr_check("name"))
        func.append("")
        func.append("    name += 2;")
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
            self.prefix = "vk"
            self.qual = "ICD_EXPORT"

        super().run()

    def generate_header(self):
        return "#include \"icd.h\""

    def _generate_stub_decl(self, proto):
        return proto.c_pretty_decl(self.prefix + proto.name, attr="VKAPI")

    def _generate_stubs(self):
        stubs = []
        for proto in self.protos:
            decl = self._generate_stub_decl(proto)
            if proto.ret != "void":
                stmt = "    return VK_ERROR_UNKNOWN;\n"
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
        body.append("    %s += 2;" % gpa_pname)
        body.append("    %s" % "\n    ".join(lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")

        return "\n".join(body)

class LayerInterceptProcSubcommand(Subcommand):
    def run(self):
        self.prefix = "vk"

        # we could get the list from argv if wanted
        self.intercepted = [proto.name for proto in self.protos
                if proto.name not in ["EnumerateGpus"]]

        for proto in self.protos:
            if proto.name == "GetProcAddr":
                self.gpa = proto

        super().run()

    def generate_header(self):
        return "\n".join(["#include <string.h>", "#include \"vkLayer.h\""])

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
        body.append("    name += 2;")
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
; Vulkan
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
            body.append("   vk" + proto.name)

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
