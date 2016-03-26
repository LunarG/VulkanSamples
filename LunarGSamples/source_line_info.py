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
# Author: Tobin Ehlis <tobin@lunarg.com>

from inspect import currentframe, getframeinfo

# This is a wrapper class for inspect module that returns a formatted line
#  with details of the source file and line number of python code who called
#  into this class. The result can them be added to codegen to simplify
#  debug as it shows where code was generated from.
class sourcelineinfo():
    def __init__(self):
        self.general_prefix = "// CODEGEN : "
        self.file_prefix = "file "
        self.line_prefix = "line #"
        self.enabled = True

    def get(self):
        if self.enabled:
            frameinfo = getframeinfo(currentframe().f_back)
            return "%s%s%s %s%s" % (self.general_prefix, self.file_prefix, frameinfo.filename, self.line_prefix, frameinfo.lineno)
        return ""
