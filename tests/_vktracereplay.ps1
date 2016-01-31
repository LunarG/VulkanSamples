# Powershell script for running the vktrace trace/replay auto test
# To run this test:
#    cd <this-dir>
#    powershell C:\src\LoaderAndValidationLayers\vktracereplay.ps1 [-Debug]
$exitstatus = 0

if ($args[0] -eq "-Debug") {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

write-host -background black -foreground green "[  RUN     ] " -nonewline
write-host "vktracereplay.ps1: Vktrace trace/replay"

# Create a temp directory to run the test in
rm -recurse -force vktracereplay_tmp  > $null 2> $null
new-item vktracereplay_tmp -itemtype directory > $null 2> $null

# Copy everything we need into the temp directory, so we
# can make sure we are using the correct dll and exe files
cd vktracereplay_tmp
cp ..\..\vktrace\$dPath\vkreplay.exe .
cp ..\..\vktrace\$dPath\vktrace.exe .
cp ..\..\demos\$dPath\cube.exe .
cp ..\..\demos\*.ppm .
cp ..\..\demos\*.spv .
cp ..\..\loader\$dPath\vulkan-1.dll .
cp ..\..\layers\$dPath\VkLayer_screenshot.dll .
cp ..\..\layers\$dPath\VkLayer_screenshot.json .
cp ..\..\layers\$dPath\VkLayer_vktrace_layer.dll .
cp ..\..\layers\$dPath\VkLayer_vktrace_layer.json .

# Change PATH to the temp directory
$oldpath = $Env:PATH
$Env:PATH = $pwd

# Set up some modified env vars
$Env:VK_LAYER_PATH = $pwd

# Do a trace and replay
& vktrace -o c01.vktrace -s 1 -p cube -a "--c 10" > trace.sout 2> trace.serr
rename-item -path 1.ppm -newname 1-trace.ppm
& vkreplay  -s 1 -t  c01.vktrace > replay.sout 2> replay.serr
rename-item -path 1.ppm -newname 1-replay.ppm

# Force a failure - for testing this script
#cp vulkan.dll 1-replay.ppm
#rm 1-trace.ppm
#rm 1-replay.ppm

# Restore PATH
$Env:PATH = $oldpath

if ($exitstatus -eq 0) {
   # Check that two screenshots were created
   if (!(Test-Path 1-trace.ppm) -or !(Test-Path 1-replay.ppm)) {
           echo 'Trace file does not exist'
           write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
           $exitstatus = 1
   }
}

if ($exitstatus -eq 0) {
    # ensure the trace and replay snapshots are identical
    fc.exe /b 1-trace.ppm 1-replay.ppm > $null
    if (!(Test-Path 1-trace.ppm) -or !(Test-Path 1-replay.ppm) -or $LastExitCode -eq 1) {
         echo 'Trace files do not match'
         write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
         $exitstatus = 1
    }
}

# check the average pixel value of each screenshot to ensure something plausible was written
if ($exitstatus -eq 0) {
    $trace_mean = (convert 1-trace.ppm -format "%[mean]" info:)
    $replay_mean = (convert 1-replay.ppm -format "%[mean]" info:)
    $version = (identify -version)

    # normalize the values so we can support Q8 and Q16 imagemagick installations
    if ($version -match "Q8") {
        $trace_mean = $trace_mean   / 255 # 2^8-1
        $replay_mean = $replay_mean / 255 # 2^8-1
    } else {
        $trace_mean = $trace_mean   / 65535 # 2^16-1
        $replay_mean = $replay_mean / 65535 # 2^16-1
    }

    # if either screenshot is too bright or too dark, it either failed, or is a bad test
    if (($trace_mean -lt 0.10) -or ($trace_mean -gt 0.90)){
        echo ''
        echo 'Trace screenshot failed mean check, must be in range [0.1, 0.9]'
        write-host 'Detected mean:' $trace_mean
        echo ''
        write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
        $exitstatus = 1
    }
    if (($replay_mean -lt 0.10) -or ($replay_mean -gt 0.90)){
        echo ''
        echo 'Replay screenshot failed mean check, must be in range [0.1, 0.9]'
        write-host 'Detected mean:' $replay_mean
        echo ''
        write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
        $exitstatus = 1
    }
}

# if we passed all the checks, the test is good
if ($exitstatus -eq 0) {
   write-host -background black -foreground green "[  PASSED  ] " -nonewline;
}

write-host "vktracereplay.ps1: Vktrace trace/replay"
write-host
if ($exitstatus) {
    echo '1 FAILED TEST'
}

# cleanup
cd ..
rm -recurse -force vktracereplay_tmp  > $null 2> $null
Remove-Item Env:\VK_LAYER_PATH
exit $exitstatus
