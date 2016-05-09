#!/usr/bin/env python3
#
# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Jon Ashburn <jon@lunarg.com>
#

import os, sys

# add main repo directory so vulkan.py can be imported. This needs to be a complete path.
ld_path = os.path.dirname(os.path.abspath(__file__))
main_path = os.path.abspath(ld_path + "/../")
sys.path.append(main_path)

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

    def _requires_special_trampoline_code(self, name):
        # Don't be cute trying to use a general rule to programmatically populate this list
        # it just obsfucates what is going on!
        wsi_creates_dispatchable_object = ["CreateSwapchainKHR"]
        creates_dispatchable_object = ["CreateDevice", "GetDeviceQueue", "AllocateCommandBuffers"] + wsi_creates_dispatchable_object
        if name in creates_dispatchable_object:
            return True
        else:
           return False

    def _is_loader_non_trampoline_entrypoint(self, proto):
        if proto.name in ["GetDeviceProcAddr", "EnumeratePhysicalDevices", "EnumerateLayers", "DbgRegisterMsgCallback", "DbgUnregisterMsgCallback", "DbgSetGlobalOption", "DestroyInstance"]:
            return True
        return not self.is_dispatchable_object_first_param(proto)


    def is_dispatchable_object_first_param(self, proto):
        in_objs = proto.object_in_params()
        non_dispatch_objs = []
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
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@lunarg.com>
 */"""

    def generate_header(self):
        return "\n".join(["#include <" + h + ">" for h in self.headers])

    def generate_body(self):
        pass

    def generate_footer(self):
        pass

class DevExtTrampolineSubcommand(Subcommand):
    def generate_header(self):
        lines = []
        lines.append("#include \"vk_loader_platform.h\"")
        lines.append("#include \"loader.h\"")
        lines.append("#if defined(__linux__)")
        lines.append("#pragma GCC optimize(3)  // force gcc to use tail-calls")
        lines.append("#endif")
        return "\n".join(lines)

    def generate_body(self):
        lines = []
        for i in range(250):
            lines.append('\nVKAPI_ATTR void VKAPI_CALL vkDevExt%s(VkDevice device)' % i)
            lines.append('{')
            lines.append('    const struct loader_dev_dispatch_table *disp;')
            lines.append('    disp = loader_get_dev_dispatch(device);')
            lines.append('    disp->ext_dispatch.DevExt[%s](device);' % i)
            lines.append('}')
        lines.append('')
        lines.append('void *loader_get_dev_ext_trampoline(uint32_t index)')
        lines.append('{')
        lines.append('    switch (index) {')
        for i in range(250):
            lines.append('        case %s:' % i)
            lines.append('            return vkDevExt%s;' % i)
        lines.append('    }')
        lines.append('    return NULL;')
        lines.append('}')
        return "\n".join(lines)

class LoaderEntrypointsSubcommand(Subcommand):
    def generate_header(self):
        return "#include \"loader.h\""

    def _generate_object_setup(self, proto):
        method = "loader_init_dispatch"
        cond = "res == VK_SUCCESS"
        setup = []

        if not self._requires_special_trampoline_code(proto.name):
           return setup

        if "Get" in proto.name:
            method = "loader_set_dispatch"

        if proto.name == "GetSwapchainInfoKHR":
            ptype = proto.params[-3].name
            psize = proto.params[-2].name
            pdata = proto.params[-1].name
            cond = ("%s == VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_KHR && "
                    "%s && %s" % (ptype, pdata, cond))
            setup.append("VkSwapchainImageInfoKHR *info = %s;" % pdata)
            setup.append("size_t count = *%s / sizeof(*info), i;" % psize)
            setup.append("for (i = 0; i < count; i++) {")
            setup.append("    %s(info[i].image, disp);" % method)
            setup.append("    %s(info[i].memory, disp);" % method)
            setup.append("}")
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
            if self._is_loader_non_trampoline_entrypoint(proto):
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
            func.append("    disp = loader_get_dispatch(%s);" %
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

            funcs.append("\n".join(func))

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
        return "\n".join(["#include <vulkan/vulkan.h>",
                          "#include <vkLayer.h>",
                          "#include <string.h>",
                          "#include \"loader_platform.h\""])

    def _generate_init(self, type):
        stmts = []
        func = []
        if type == "device":
            for proto in self.protos:
                if self.is_dispatchable_object_first_param(proto) or proto.name == "CreateInstance":
                    stmts.append("table->%s = (PFN_vk%s) gpa(gpu, \"vk%s\");" %
                        (proto.name, proto.name, proto.name))
                else:
                    stmts.append("table->%s = vk%s; /* non-dispatchable */" %
                             (proto.name, proto.name))
            func.append("static inline void %s_init_device_dispatch_table(VkLayerDispatchTable *table,"
                % self.prefix)
            func.append("%s                                              PFN_vkGetDeviceProcAddr gpa,"
                % (" " * len(self.prefix)))
            func.append("%s                                              VkPhysicalDevice gpu)"
                % (" " * len(self.prefix)))
        else:
            for proto in self.protos:
                if proto.params[0].ty != "VkInstance" and proto.params[0].ty != "VkPhysicalDevice":
                    continue
                stmts.append("table->%s = vk%s;" % (proto.name, proto.name))
            func.append("static inline void %s_init_instance_dispatch_table(VkLayerInstanceDispatchTable *table)"
                % self.prefix)
        func.append("{")
        func.append("    %s" % "\n    ".join(stmts))
        func.append("}")

        return "\n".join(func)

    def _generate_lookup(self):
        lookups = []
        for proto in self.protos:
            if self.is_dispatchable_object_first_param(proto):
                lookups.append("if (!strcmp(name, \"%s\"))" % (proto.name))
                lookups.append("    return (void *) table->%s;"
                    % (proto.name))

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
        body = [self._generate_init("device"),
                self._generate_lookup(),
                self._generate_init("instance")]

        return "\n\n".join(body)

class WinDefFileSubcommand(Subcommand):
    def run(self):
        library_exports = {
                "all": [],
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
; Copyright (c) 2015-2016 The Khronos Group Inc.
; Copyright (c) 2015-2016 Valve Corporation
; Copyright (c) 2015-2016 LunarG, Inc.
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;
;
;  Author: Jon Ashburn <jon@lunarg.com>
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
#           This was intended to reject WSI calls, but actually rejects ALL extensions
#           TODO:  Make this WSI-extension specific
#           if proto.name.endswith("KHR"):
#               continue
            body.append("   vk" + proto.name)

        return "\n".join(body)

class LoaderGetProcAddrSubcommand(Subcommand):
    def run(self):
        self.prefix = "vk"

        # we could get the list from argv if wanted
        self.intercepted = [proto.name for proto in self.protos]

        for proto in self.protos:
            if proto.name == "GetDeviceProcAddr":
                self.gpa = proto

        super().run()

    def generate_header(self):
        return "\n".join(["#include <string.h>"])

    def generate_body(self):
        lookups = []
        for proto in self.protos:
            if proto.name not in self.intercepted:
                lookups.append("/* no %s%s */" % (self.prefix, proto.name))
                continue

            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (%s) %s%s;" %
                    (self.gpa.ret, self.prefix, proto.name))

        special_lookups = []
        for proto in self.protos:
            if self._is_loader_non_trampoline_entrypoint(proto) or self._requires_special_trampoline_code(proto.name):
                special_lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
                special_lookups.append("    return (%s) %s%s;" %
                        (self.gpa.ret, self.prefix, proto.name))
            else:
                continue
        body = []
        body.append("static inline %s globalGetProcAddr(const char *name)" %
                self.gpa.ret)
        body.append("{")
        body.append(generate_get_proc_addr_check("name"))
        body.append("")
        body.append("    name += 2;")
        body.append("    %s" % "\n    ".join(lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")
        body.append("")
        body.append("static inline void *loader_non_passthrough_gpa(const char *name)")
        body.append("{")
        body.append(generate_get_proc_addr_check("name"))
        body.append("")
        body.append("    name += 2;")
        body.append("    %s" % "\n    ".join(special_lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")

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
            "dev-ext-trampoline": DevExtTrampolineSubcommand,
            "loader-entrypoints": LoaderEntrypointsSubcommand,
            "dispatch-table-ops": DispatchTableOpsSubcommand,
            "win-def-file": WinDefFileSubcommand,
            "loader-get-proc-addr": LoaderGetProcAddrSubcommand,
    }

    if len(sys.argv) < 3 or sys.argv[1] not in wsi or sys.argv[2] not in subcommands:
        print("Usage: %s <wsi> <subcommand> [options]" % sys.argv[0])
        print
        print("Available wsi (displayservers) are: %s" % " ".join(wsi))
        print("Available subcommands are: %s" % " ".join(subcommands))
        exit(1)

    subcmd = subcommands[sys.argv[2]](sys.argv[3:])
    subcmd.run()

if __name__ == "__main__":
    main()
