# Copyright (c) 2020 LunarG, Inc.
# To be run from build/API-Samples

import os, subprocess, platform

if platform.system() == 'Windows':
    for subdir in ["Debug", "Release"]:
        if os.path.isdir(subdir):
            directory = os.path.realpath(subdir)
            break
    else:
        sys.exit("Cannot find samples in Debug or Release, have they been built?")
    suffix = ".exe"
else:
    directory = os.getcwd()
    suffix = ""

samples = []
samplesdir = os.path.join("..", "..", "API-Samples")
for root, dir, files in os.walk(samplesdir):
    for file in files:
        if file.endswith(".cpp") and "utils" not in root and "android" not in root:
            samples.append(os.path.splitext(file)[0])

samples.sort()
samples_requiring_validation_layer = ["enable_validation_with_callback", "validation_cache"]
for sample in samples:
    executable = os.path.join(directory, sample)
    print('exe = ' + executable)
    if "VK_LAYER_PATH" not in os.environ and sample in samples_requiring_validation_layer:
        print('Skipping {} because it requires validation layers'.format(sample))
        continue
    if os.path.isfile(executable + suffix):
        print('Running: ' + sample)
        subprocess.check_call(executable)
    else:
        print("Skipping {} because the sample doesn't seem to be built".format(sample))
