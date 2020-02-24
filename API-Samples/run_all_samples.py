# Copyright (c) 2020 LunarG, Inc.
# To be run from build/API-Samples

import os, subprocess, platform

if platform.system() == 'Windows':
    if os.path.isdir("Debug"):
        directory = "Debug"
    elif ospath.isdir("Release"):
        directory = "Release"
    else:
        sys.exit("Cannot find samples in Debug or Release, have they been built?")
else:
    directory = os.cwd()
    
for file in os.listdir(directory):
    if file.endswith(".exe"):
        sample = os.path.join(directory, file)
        if "VK_LAYER_PATH" not in os.environ:
            if file == "enable_validation_with_callback.exe" or file == "validation_cache.exe":
                continue
        print("Running: ", sample)
        subprocess.check_call(sample)
