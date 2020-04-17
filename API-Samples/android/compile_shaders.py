# Copyright (c) 2020 LunarG, Inc.
# To be run from build/API-Samples

import os, subprocess, platform, sys
script = os.path.join("..", "..", "scripts", "generate_spirv.py")
validator = os.path.join("..", "..", "glslang", "bin", "glslangValidator")
assembler = os.path.join("..", "..", "spirv-tools", "bin", "spirv-as")

zip_name = {"Linux": "glslang-master-linux-Release.zip",
            "Darwin": "glslang-master-osx-Release.zip",
            "Windows": "glslang-master-windows-x64-Release.zip"}[platform.system()]
if not os.path.exists(validator):
    args = [ sys.executable, os.path.join("..", "..", "scripts", "fetch_glslangvalidator.py"), zip_name]
    subprocess.check_call(args)

zip_name = {"Linux" : "SPIRV-Tools-master-linux-RelWithDebInfo.zip",
            "Darwin": "SPIRV-Tools-master-osx-RelWithDebInfo.zip",
            "Windows":"SPIRV-Tools-master-windows-x64-Release.zip"}[platform.system()]
if not os.path.exists(assembler):
    args = [ sys.executable, os.path.join("..", "..", "scripts", "fetch_spirv_tools.py"), zip_name]
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
            

