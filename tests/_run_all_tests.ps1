# Be sure to run "Set-ExecutionPolicy RemoteSigned" before running powershell scripts

# Use TestExceptions to filter out tests with known problems, separated by a colon
# i.e. run_all_tests.ps1 -TestExceptions VkLayerTest.RequiredParameter:VkLayerTest.UnrecognizedValue

# To trigger Debug tests, specify the parameter with a hyphen
# i.e  run_all_tests.ps1 -Debug

Param(
    [switch]$Debug,
    [string]$LoaderTestExceptions,
    [string]$TestExceptions
)

if ($Debug) {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

$AboveDir = (Get-Item -Path ".." -Verbose).FullName
Write-Host "Using Vulkan run-time=$AboveDir\loader\$dPath"
Set-Item -path env:Path -value ("$AboveDir\loader\$dPath;$AboveDir\tests\gtest-1.7.0\$dPath;" + $env:Path)
Write-Host "Using VK_LAYER_PATH=$AboveDir\layers\$dPath"
$env:VK_LAYER_PATH = "$AboveDir\layers\$dPath"

& $dPath\vk_loader_validation_tests --gtest_filter=-$LoaderTestExceptions
if ($lastexitcode -ne 0) {
   exit 1
}

& $dPath\vk_layer_validation_tests --gtest_filter=-$TestExceptions

exit $lastexitcode
