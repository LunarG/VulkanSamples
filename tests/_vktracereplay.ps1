# Powershell script for running the vktrace trace/replay auto test
# To run this test:
#    cd <this-dir>
#    powershell C:\src\LoaderAndTools\vktracereplay.ps1 [-Debug]

if ($args[0] -eq "-Debug") {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

write-host -background black -foreground green "[  RUN     ] " -nonewline
write-host "vktracereplay.ps1: Vktrace trace/replay"

$gdir = "..\..\..\Vktrace"

# Create a temp directory to run the test in
rm -recurse -force vktracereplay_tmp  > $null 2> $null
new-item vktracereplay_tmp -itemtype directory > $null 2> $null

# Copy everything we need into the temp directory, so we
# can make sure we are using the correct dll and exe files
cd vktracereplay_tmp
cp ..\$gdir\$dPath\vkreplay.exe .
cp ..\$gdir\$dPath\vktrace.exe .
cp ..\$gdir\$dPath\vkreplay.dll .
cp ..\$gdir\$dPath\vktrace.dll .
cp ..\..\demos\$dPath\cube.exe .
cp ..\..\demos\*.png .
cp ..\..\loader\$dPath\vulkan.0.dll .
cp ..\..\layers\$dPath\VKLayerScreenShot.dll .

# Change PATH to the temp directory
$oldpath = $Env:PATH
$Env:PATH = $pwd
$Env:VK_LAYER_PATH = $pwd

# Do a trace and replay
& vktrace -o c01.vktrace -s 1 -p cube -a "--c 10" -l0 vktrace.dll > trace.sout 2> trace.serr
rename-item -path 1.ppm -newname 1-trace.ppm
& vkreplay  -s 1 -t  c01.vktrace  > replay.sout 2> replay.serr
rename-item -path 1.ppm -newname 1-replay.ppm

# Force a failure - for testing this script
#cp vulkan.dll 1-replay.ppm
#rm 1-trace.ppm
#rm 1-replay.ppm

# Restore PATH
$Env:PATH = $oldpath

# Compare the trace file, print a message
if (!(Test-Path 1-trace.ppm) -or !(Test-Path 1-replay.ppm)) {
        echo 'Trace file does not exist'
        write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
        $exitstatus = 1
} else {
    fc.exe /b 1-trace.ppm 1-replay.ppm > $null
    if (!(Test-Path 1-trace.ppm) -or !(Test-Path 1-replay.ppm) -or $LastExitCode -eq 1) {
        echo 'Trace files do not match'
        write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
        $exitstatus = 1
    } else {
        write-host -background black -foreground green "[  PASSED  ] " -nonewline;
        $exitstatus = 0
    }
}
write-host "vktracereplay.ps1: Vktrace trace/replay"
write-host
if ($exitstatus) {
    echo '1 FAILED TEST'
}

cd ..
rm -recurse -force vktracereplay_tmp  > $null 2> $null
exit $exitstatus
