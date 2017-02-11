# Powershell script for running validating the layer database file
# To run this test:
#    From a Windows powershell:
#    cd C:\src\Vulkan-LoaderAndValidationLayers\build\tests
#    .\vkvalidatelayerdoc.ps1 [-Debug]

if ($args[0] -eq "-Debug") {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

write-host -background black -foreground green "[  RUN     ] " -nonewline
write-host "vkvalidatelayerdoc.ps1: Validate layer documentation"

# Run doc validation from project layers dir
push-location ..\..\layers

# Validate that layer documentation matches source contents
python vk_validation_stats.py

# Report result based on exit code
if (!$LASTEXITCODE) {
    write-host -background black -foreground green "[  PASSED  ] " -nonewline;
    $exitstatus = 0
} else {
    echo 'Validation of vk_validation_error_database.txt failed'
    write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
    echo '1 FAILED TEST'
    $exitstatus = 1
}

pop-location
exit $exitstatus
