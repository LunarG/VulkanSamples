#!/usr/bin/env python3
#
# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
# Copyright (c) 2015-2016 Google Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and/or associated documentation files (the "Materials"), to
# deal in the Materials without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Materials, and to permit persons to whom the Materials
# are furnished to do so, subject to the following conditions:
#
# The above copyright notice(s) and this permission notice shall be included
# in all copies or substantial portions of the Materials.
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
# USE OR OTHER DEALINGS IN THE MATERIALS
#
# Author: Chia-I Wu <olv@lunarg.com>
# Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
# Author: Jon Ashburn <jon@lunarg.com>
# Author: Gwan-gyeong Mun <kk.moon@samsung.com>

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
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and/or associated documentation files (the "Materials"), to
 * deal in the Materials without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Materials, and to permit persons to whom the Materials are
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included in
 * all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
 * USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
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
        super(DispatchTableOpsSubcommand, self).run()

    def generate_header(self):
        return "\n".join(["#include <vulkan/vulkan.h>",
                          "#include <vulkan/vk_layer.h>",
                          "#include <string.h>"])

    def _generate_init_dispatch(self, type):
        stmts = []
        func = []
        if type == "device":
            # GPA has to be first one and uses wrapped object
            stmts.append("memset(table, 0, sizeof(*table));")
            stmts.append("table->GetDeviceProcAddr =(PFN_vkGetDeviceProcAddr)  gpa(device,\"vkGetDeviceProcAddr\");")
            for proto in self.protos:
                if proto.name == "CreateInstance" or proto.name == "EnumerateInstanceExtensionProperties" or proto.name == "EnumerateInstanceLayerProperties" or proto.params[0].ty == "VkInstance" or proto.params[0].ty == "VkPhysicalDevice":
                    continue
                if proto.name != "GetDeviceProcAddr" and 'KHR' not in proto.name:
                    stmts.append("table->%s = (PFN_vk%s) gpa(device, \"vk%s\");" %
                        (proto.name, proto.name, proto.name))
            func.append("static inline void %s_init_device_dispatch_table(VkDevice device,"
                % self.prefix)
            func.append("%s                                               VkLayerDispatchTable *table,"
                % (" " * len(self.prefix)))
            func.append("%s                                               PFN_vkGetDeviceProcAddr gpa)"
                % (" " * len(self.prefix)))
        else:
            stmts.append("table->GetInstanceProcAddr =(PFN_vkGetInstanceProcAddr)  gpa(instance,\"vkGetInstanceProcAddr\");")
            for proto in self.protos:
                if proto.params[0].ty != "VkInstance" and proto.params[0].ty != "VkPhysicalDevice":
                    continue
                if proto.name == "CreateDevice":
                    continue
                if proto.name != "GetInstanceProcAddr" and 'KHR' not in proto.name:
                    stmts.append("table->%s = (PFN_vk%s) gpa(instance, \"vk%s\");" %
                          (proto.name, proto.name, proto.name))
            func.append("static inline void %s_init_instance_dispatch_table(" % self.prefix)
            func.append("%s        VkInstance instance," % (" " * len(self.prefix)))
            func.append("%s        VkLayerInstanceDispatchTable *table," % (" " * len(self.prefix)))
            func.append("%s        PFN_vkGetInstanceProcAddr gpa)" % (" " * len(self.prefix)))
        func.append("{")
        func.append("    %s" % "\n    ".join(stmts))
        func.append("}")

        return "\n".join(func)

    def generate_body(self):
        body = [self._generate_init_dispatch("device"),
                self._generate_init_dispatch("instance")]

        return "\n\n".join(body)

class WinDefFileSubcommand(Subcommand):
    def run(self):
        library_exports = {
                "all": [],
                "icd": [
                    "vk_icdGetInstanceProcAddr",
                ],
                "layer": [
                    "vkGetInstanceProcAddr",
                    "vkGetDeviceProcAddr",
                    "vkEnumerateInstanceLayerProperties",
                    "vkEnumerateInstanceExtensionProperties"
                ],
                "layer_multi": [
                    "multi2GetInstanceProcAddr",
                    "multi1GetDeviceProcAddr"
                ]
        }

        if len(self.argv) != 2 or self.argv[1] not in library_exports:
            print("WinDefFileSubcommand: <library-name> {%s}" %
                    "|".join(library_exports.keys()))
            return

        self.library = self.argv[0]
        if self.library == "VkLayer_multi":
            self.exports = library_exports["layer_multi"]
        else:
            self.exports = library_exports[self.argv[1]]

        super().run()

    def generate_copyright(self):
        return """; THIS FILE IS GENERATED.  DO NOT EDIT.

;;;; Begin Copyright Notice ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Vulkan
;
; Copyright (c) 2015-2016 The Khronos Group Inc.
; Copyright (c) 2015-2016 Valve Corporation
; Copyright (c) 2015-2016 LunarG, Inc.
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and/or associated documentation files (the "Materials"), to
; deal in the Materials without restriction, including without limitation the
; rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
; sell copies of the Materials, and to permit persons to whom the Materials are
; furnished to do so, subject to the following conditions:
;
; The above copyright notice(s) and this permission notice shall be included in
; all copies or substantial portions of the Materials.
;
; THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;
; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
; DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
; OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
; USE OR OTHER DEALINGS IN THE MATERIALS.
;
;  Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
;;;;  End Copyright Notice ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;"""

    def generate_header(self):
        return "; The following is required on Windows, for exporting symbols from the DLL"

    def generate_body(self):
        body = []

        body.append("LIBRARY " + self.library)
        body.append("EXPORTS")

        for proto in self.exports:
            if self.library != "VkLayerSwapchain" or proto != "vkEnumerateInstanceExtensionProperties" and proto != "vkEnumerateInstanceLayerProperties":
                body.append( proto)

        return "\n".join(body)

def main():
    wsi = {
            "Win32",
            "Android",
            "Xcb",
            "Xlib",
            "Wayland",
            "Mir"
    }
    subcommands = {
            "dispatch-table-ops": DispatchTableOpsSubcommand,
            "win-def-file": WinDefFileSubcommand,
    }

    if len(sys.argv) < 3 or sys.argv[1] not in wsi or sys.argv[2] not in subcommands:
        print("Usage: %s <wsi> <subcommand> [options]" % sys.argv[0])
        print
        print("Available sucommands are: %s" % " ".join(subcommands))
        exit(1)

    subcmd = subcommands[sys.argv[2]](sys.argv[3:])
    subcmd.run()

if __name__ == "__main__":
    main()
