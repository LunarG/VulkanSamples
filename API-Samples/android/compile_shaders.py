# Copyright (c) 2020 LunarG, Inc.
# To be run from build/API-Samples

import os, subprocess, platform, sys
script = os.path.join("..", "..", "scripts", "generate_spirv.py")
validator = os.path.join("..", "..", "glslang", "bin", "glslangValidator")
assembler = os.path.join("..", "..", "spirv-tools", "bin", "spirv-as")

if not os.path.exists(validator):
    args = [ sys.executable, os.path.join("..", "..", "scripts", "fetch_glslangvalidator.py"), "glslang-master-linux-Release.zip" ]
    subprocess.check_call(args)

if not os.path.exists(assembler):
    args = [ sys.executable, os.path.join("..", "..", "scripts", "fetch_spirv_tools.py"), "SPIRV-Tools-master-linux-RelWithDebInfo.zip" ]
    subprocess.check_call(args)

samplesdir = os.path.join(os.getcwd(), "..")
headersdir = os.path.join(os.getcwd(), "ShaderHeaders")
if not os.path.exists(headersdir):
    os.mkdir(headersdir)
assembly_files = ["spirv_assembly.vert", "spirv_assembly.frag", "specialized.frag"]
for root, dir, files in os.walk(samplesdir):
    for file in files:
        if file.endswith(".vert") or file.endswith(".frag"):
            samplepath = os.path.join(root, file)
            if file in assembly_files:
                args = [ sys.executable, script, samplepath, os.path.join(headersdir, file + ".h"), assembler, "true"]
            else:
                args = [ sys.executable, script, samplepath, os.path.join(headersdir, file + ".h"), validator, "false"]
            subprocess.check_call(args)
            #print(args)
            

