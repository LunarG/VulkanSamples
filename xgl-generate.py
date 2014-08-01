#!/usr/bin/env python3

import sys

import xgl

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.protos = ()

    def run(self):
        self.protos = xgl.core
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
        pass

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

class PrettyDummySubcommand(Subcommand):
    def generate_header(self):
        return "\n".join([
            "#include <xgl.h>",
            "#include <xglDbg.h>"])

    def generate_body(self):
        funcs = []
        for proto in self.protos:
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

            if proto.ret != "XGL_VOID":
                stmt = "    return XGL_ERROR_UNAVAILABLE;\n"
            else:
                stmt = ""

            funcs.append("%s XGLAPI xgl%s(\n%s)\n{\n%s}" % (proto.ret,
                    proto.name, ",\n".join(plist), stmt))

        return "\n\n".join(funcs)

class LoaderSubcommand(Subcommand):
    def generate_header(self):
        return "\n".join([
            "#include <xgl.h>",
            "#include <xglDbg.h>"])

    def _generate_api(self):
        funcs = []
        for proto in self.protos:
            if not xgl.is_dispatchable(proto):
                continue

            decl = proto.c_func(prefix="xgl", attr="XGLAPI")
            stmt = "(*disp)->%s" % proto.c_call()
            if proto.ret != "XGL_VOID":
                stmt = "return " + stmt

            funcs.append("%s\n"
                         "{\n"
                         "    const struct icd_dispatch_table * const *disp =\n"
                         "        (const struct icd_dispatch_table * const *) %s;\n"
                         "    %s;\n"
                         "}" % (decl, proto.params[0].name, stmt))

        return "\n\n".join(funcs)

    def generate_body(self):
        body = [self._generate_icd_dispatch_table(),
                self._generate_api()]

        return "\n\n".join(body)

def main():
    subcommands = {
            "pretty-dummy": PrettyDummySubcommand,
            "loader": LoaderSubcommand,
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
