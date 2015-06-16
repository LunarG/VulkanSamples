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
                          "#include <string.h>"])

    def _generate_init_dispatch(self, type):
        stmts = []
        func = []
        if type == "device":
            # GPA has to be first one and uses wrapped object
            stmts.append("VkDevice device = (VkDevice) devw->nextObject;")
            stmts.append("PFN_vkGetDeviceProcAddr gpa = (PFN_vkGetDeviceProcAddr) devw->pGPA;")
            stmts.append("VkDevice baseDevice = (VkDevice) devw->baseObject;")
            stmts.append("// GPA has to be first entry inited and uses wrapped object since it triggers init")
            stmts.append("table->GetDeviceProcAddr =(PFN_vkGetDeviceProcAddr)  gpa(device,\"vkGetDeviceProcAddr\");")
            for proto in self.protos:
                if proto.name == "CreateInstance" or proto.name == "GetGlobalExtensionInfo" or proto.params[0].ty == "VkInstance" or proto.params[0].ty == "VkPhysicalDevice":
                    continue
                if proto.name != "GetDeviceProcAddr":
                    stmts.append("table->%s = (PFN_vk%s) gpa(baseDevice, \"vk%s\");" %
                        (proto.name, proto.name, proto.name))
            func.append("static inline void %s_initialize_dispatch_table(VkLayerDispatchTable *table,"
                % self.prefix)
            func.append("%s                                              const VkBaseLayerObject *devw)"
                % (" " * len(self.prefix)))
        else:
            # GPA has to be first one and uses wrapped object
            stmts.append("VkInstance instance = (VkInstance) instw->nextObject;")
            stmts.append("PFN_vkGetInstanceProcAddr gpa = (PFN_vkGetInstanceProcAddr) instw->pGPA;")
            stmts.append("VkInstance baseInstance = (VkInstance) instw->baseObject;")
            stmts.append("// GPA has to be first entry inited and uses wrapped object since it triggers init")
            stmts.append("table->GetInstanceProcAddr =(PFN_vkGetInstanceProcAddr)  gpa(instance,\"vkGetInstanceProcAddr\");")
            for proto in self.protos:
                if proto.name != "CreateInstance"  and proto.params[0].ty != "VkInstance" and proto.params[0].ty != "VkPhysicalDevice":
                    continue
                if proto.name != "GetInstanceProcAddr":
                    stmts.append("table->%s = (PFN_vk%s) gpa(baseInstance, \"vk%s\");" %
                          (proto.name, proto.name, proto.name))
            func.append("static inline void %s_init_instance_dispatch_table(VkLayerInstanceDispatchTable *table,"
                % self.prefix)
            func.append("%s                                              const VkBaseLayerObject *instw)"
                % (" " * len(self.prefix)))
        func.append("{")
        func.append("    %s" % "\n    ".join(stmts))
        func.append("}")

        return "\n".join(func)

    def generate_body(self):
        body = [self._generate_init_dispatch("device"),
                self._generate_init_dispatch("instance")]

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
            if proto.name == "GetDeviceProcAddr":
                gpa_proto = proto
            if proto.name == "GetInstanceProcAddr":
                gpa_instance_proto = proto

        gpa_instance_decl = self._generate_stub_decl(gpa_instance_proto)
        gpa_decl = self._generate_stub_decl(gpa_proto)
        gpa_pname = gpa_proto.params[-1].name

        lookups = []
        for proto in self.protos:
            lookups.append("if (!strcmp(%s, \"%s\"))" %
                    (gpa_pname, proto.name))
            lookups.append("    return (%s) %s%s;" %
                    (gpa_proto.ret, self.prefix, proto.name))

        body = []
        body.append("%s %s" % (self.qual, gpa_instance_decl))
        body.append("{")
        body.append("    return NULL;")
        body.append("}")
        body.append("")

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

class WinDefFileSubcommand(Subcommand):
    def run(self):
        library_exports = {
                "all": [],
                "icd": [
                    "EnumeratePhysicalDevices",
                    "CreateInstance",
                    "DestroyInstance",
                    "GetDeviceProcAddr",
                    "GetInstanceProcAddr",
                ],
                "layer": [
                    "GetInstanceProcAddr",
                    "GetDeviceProcAddr",
                    "EnumerateLayers",
                    "GetGlobalExtensionInfo",
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
            "dispatch-table-ops": DispatchTableOpsSubcommand,
            "icd-dummy-entrypoints": IcdDummyEntrypointsSubcommand,
            "icd-get-proc-addr": IcdGetProcAddrSubcommand,
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
