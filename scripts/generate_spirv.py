#!/usr/bin/env python3
#
# Copyright (C) 2016 Google, Inc.
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

"""Compile GLSL to SPIR-V.

Depends on glslangValidator and/or spirv-as.
"""

import os
import sys
import subprocess
import struct
import re

SPIRV_MAGIC = 0x07230203
COLUMNS = 4
INDENT = 4

in_filename = sys.argv[1]
out_filename = sys.argv[2] if len(sys.argv) > 2 else None
executable = sys.argv[3]
assemble = sys.argv[4]

def identifierize(s):
    # translate invalid chars
    s = re.sub("[^0-9a-zA-Z_]", "_", s)
    # translate leading digits
    return re.sub("^[^a-zA-Z_]+", "_", s)

def compile(filename, tmpfile):
    # invoke glslangValidator or spirv-as
    try:
        if (assemble == 'true'):
            args = [executable, "-o", tmpfile, "--target-env", "spv1.0", filename]
        else:
            args = [executable, "-V", "-H", "-o", tmpfile, filename]
        output = subprocess.check_output(args, universal_newlines=True)
    except subprocess.CalledProcessError as e:
        print(e.output, file=sys.stderr)
        exit(1)

    # read the temp file into a list of SPIR-V words
    words = []
    with open(tmpfile, "rb") as f:
        data = f.read()
        assert(len(data) and len(data) % 4 == 0)

        # determine endianness
        fmt = ("<" if data[0] == (SPIRV_MAGIC & 0xff) else ">") + "I"
        for i in range(0, len(data), 4):
            words.append(struct.unpack(fmt, data[i:(i + 4)])[0])

        assert(words[0] == SPIRV_MAGIC)


    # remove temp file
    os.remove(tmpfile)

    return (words, output.rstrip())

base = os.path.basename(in_filename)
words, comments = compile(in_filename, base + ".tmp")

literals = []
for i in range(0, len(words), COLUMNS):
    columns = ["0x%08x" % word for word in words[i:(i + COLUMNS)]]
    literals.append(" " * INDENT + ", ".join(columns) + ",")

header = """#include <stdint.h>

#if 0
%s
#endif

static const uint32_t %s[%d] = {
%s
};
""" % (comments, identifierize(base), len(words), "\n".join(literals))

if out_filename:
    with open(out_filename, "w") as f:
        print(header, end="", file=f)
else:
        print(header, end="")
