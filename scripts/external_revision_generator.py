#!/usr/bin/env python3
#
# Copyright (c) 2015-2017 The Khronos Group Inc.
# Copyright (c) 2015-2017 Valve Corporation
# Copyright (c) 2015-2017 LunarG, Inc.
# Copyright (c) 2015-2017 Google Inc.
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
# Author: Cort Stratton <cort@google.com>

import os
import subprocess
import sys

if __name__ == '__main__':
    if (len(sys.argv) != 4):
        print("Usage: %s <SOURCE_DIR> <SYMBOL_NAME> <OUTPUT_HEADER_FILE>" % sys.argv[0])
        sys.exit(os.EX_USAGE)
    
    source_dir = sys.argv[1]
    symbol_name = sys.argv[2]
    output_header_file = sys.argv[3]
    
    # Extract commit ID from the specified source directory
    # Call git.bat on Windows for compatiblity.
    git_binary = "git.bat" if os == "nt" else "git"
    commit_id = subprocess.check_output([git_binary, "rev-parse", "HEAD"], cwd=source_dir).decode('utf-8').strip()
    
    # Write commit ID to output header file
    with open(output_header_file, "w") as header_file:
         # File Comment
        file_comment = '// *** THIS FILE IS GENERATED - DO NOT EDIT ***\n'
        file_comment += '// See external_revision_generator.py for modifications\n'
        header_file.write(file_comment)
        # Copyright Notice
        copyright = ''
        copyright += '\n'
        copyright += '/***************************************************************************\n'
        copyright += ' *\n'
        copyright += ' * Copyright (c) 2015-2017 The Khronos Group Inc.\n'
        copyright += ' * Copyright (c) 2015-2017 Valve Corporation\n'
        copyright += ' * Copyright (c) 2015-2017 LunarG, Inc.\n'
        copyright += ' * Copyright (c) 2015-2017 Google Inc.\n'
        copyright += ' *\n'
        copyright += ' * Licensed under the Apache License, Version 2.0 (the "License");\n'
        copyright += ' * you may not use this file except in compliance with the License.\n'
        copyright += ' * You may obtain a copy of the License at\n'
        copyright += ' *\n'
        copyright += ' *     http://www.apache.org/licenses/LICENSE-2.0\n'
        copyright += ' *\n'
        copyright += ' * Unless required by applicable law or agreed to in writing, software\n'
        copyright += ' * distributed under the License is distributed on an "AS IS" BASIS,\n'
        copyright += ' * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n'
        copyright += ' * See the License for the specific language governing permissions and\n'
        copyright += ' * limitations under the License.\n'
        copyright += ' *\n'
        copyright += ' * Author: Chris Forbes <chrisforbes@google.com>\n'
        copyright += ' * Author: Cort Stratton <cort@google.com>\n'
        copyright += ' *\n'
        copyright += ' ****************************************************************************/\n'
        header_file.write(copyright)
        # Contents
        contents = '#pragma once\n\n'
        contents += '#define %s "%s"\n' % (symbol_name, commit_id)
        header_file.write(contents)
    