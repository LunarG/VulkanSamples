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

    def _generate_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        for proto in self.protos:
            if not xgl.is_dispatchable(proto):
                continue
            decl = proto.c_func(prefix="xgl", attr="XGLAPI")
            stmt = "(*disp)->%s" % proto.c_call()
            if proto.name == "CreateDevice" and qual == "LOADER_EXPORT ":
                funcs.append("%s%s\n"
                         "{\n"
                         "    loader_activate_layers(%s, %s);\n"
                         "    XGL_BASE_LAYER_OBJECT* wrapped_obj = (XGL_BASE_LAYER_OBJECT*)%s;\n"
                         "    const XGL_LAYER_DISPATCH_TABLE **disp =\n"
                         "            (const XGL_LAYER_DISPATCH_TABLE **) wrapped_obj->baseObject;\n"
                         "    %s = wrapped_obj->nextObject;\n"
                         "    XGL_RESULT res = %s;\n"
                         "    *(const XGL_LAYER_DISPATCH_TABLE **) (*%s) = *disp;\n"
                         "    return res;\n"
                         "}" % (qual, decl, proto.params[0].name, proto.params[1].name,
                                proto.params[0].name, proto.params[0].name, stmt,
                                proto.params[-1].name))
            elif xgl.does_function_create_object(proto.name) and qual == "LOADER_EXPORT ":
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE **disp =\n"
                         "        (const XGL_LAYER_DISPATCH_TABLE **) %s;\n"
                         "    XGL_RESULT res = %s;\n"
                         "    *(const XGL_LAYER_DISPATCH_TABLE **) (*%s) = *disp;\n"
                         "    return res;\n"
                         "}" % (qual, decl, proto.params[0].name, stmt, proto.params[-1].name))
            elif proto.name == "GetMultiGpuCompatibility" and qual == "LOADER_EXPORT ":
                funcs.append("%s%s\n"
                         "{\n"
                         "    XGL_BASE_LAYER_OBJECT* wrapped_obj0 = (XGL_BASE_LAYER_OBJECT*)%s;\n"
                         "    XGL_BASE_LAYER_OBJECT* wrapped_obj1 = (XGL_BASE_LAYER_OBJECT*)%s;\n"
                         "    const XGL_LAYER_DISPATCH_TABLE * const *disp =\n"
                         "        (const XGL_LAYER_DISPATCH_TABLE * const *) wrapped_obj0->baseObject;\n"
                         "    %s = wrapped_obj0->nextObject;\n"
                         "    %s = wrapped_obj1->nextObject;\n"
                         "    return %s;\n"
                         "}" % (qual, decl, proto.params[0].name, proto.params[1].name,
                                proto.params[0].name, proto.params[1].name, stmt))
            elif proto.params[0].ty != "XGL_PHYSICAL_GPU" or qual != "LOADER_EXPORT ":
                if proto.ret != "XGL_VOID":
                    stmt = "return " + stmt
                funcs.append("%s%s\n"
                         "{\n"
                         "    const XGL_LAYER_DISPATCH_TABLE * const *disp =\n"
                         "        (const XGL_LAYER_DISPATCH_TABLE * const *) %s;\n"
                         "    %s;\n"
                         "}" % (qual, decl, proto.params[0].name, stmt))
            else:
                if proto.ret != "XGL_VOID":
                    stmt = "return " + stmt
                funcs.append("%s%s\n"
                         "{\n"
                         "    XGL_BASE_LAYER_OBJECT* wrapped_obj = (XGL_BASE_LAYER_OBJECT*)%s;\n"
                         "    const XGL_LAYER_DISPATCH_TABLE * const *disp =\n"
                         "        (const XGL_LAYER_DISPATCH_TABLE * const *) wrapped_obj->baseObject;\n"
                         "    %s = wrapped_obj->nextObject;\n"
                         "    %s;\n"
                             "}" % (qual, decl, proto.params[0].name, proto.params[0].name, stmt))

        return "\n\n".join(funcs)

class LoaderSubcommand(Subcommand):
    def generate_header(self):
        return "#include \"loader.h\""

    def generate_body(self):
        body = [self._generate_dispatch_entrypoints("LOADER_EXPORT")]

        return "\n\n".join(body)

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

        return """const XGL_LAYER_DISPATCH_TABLE %s_normal_dispatch_table = {
    %s,
};

const XGL_LAYER_DISPATCH_TABLE %s_debug_dispatch_table = {
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
