#
# Copyright (C) 2016 Valve Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
# Author: David Pinedo <david@LunarG.com>
#

# This Powershell script is used by the Vulkan Run Time Installer/Uninstaller to:
#   - Copy the most recent vulkan-<majorabi>-*.dll in C:\Windows\System32
#     to vulkan-<majorabi>.dll
#   - Copy the most recent version of vulkaninfo-<abimajor>-*.exe in
#     C:\Windows\System32 to vulkaninfo.exe
#   - Set the layer registry entries to point to the layer json files
#     in the Vulkan SDK associated with the most recent vulkan*dll.
#
# This script takes one parameter - a single number specifying the major abi version.
#

Param([string]$majorabi)

$vulkandll = "vulkan-"+$majorabi+".dll"

# The name of the versioned vulkan dll file is one of the following:
#
#   vulkan-<majorabi>-<major>-<minor>-<patch>-<buildno>-<prerelease>-<prebuildno>
#   vulkan-<majorabi>-<major>-<minor>-<patch>-<buildno>-<prerelease>
#   vulkan-<majorabi>-<major>-<minor>-<patch>-<buildno>-<prebuildno>
#   vulkan-<majorabi>-<major>-<minor>-<patch>-<buildno>.dll
#
# <major>, <minor>, <patch>, <buildno> and <prebuildno> are 1 to 10 numeric digits.
# <prerelease> is any combination of alpha and numeric characters.
# If <prerelease> and/or <prebuildno> are present, this identifies a prerelease,
# and the vulkan dll file will be considered less recent than one with the same
# <major>, <minor>, <patch>, <buildno> numbers without the <prerelease> and/or
# <prebuildno>.


# We first create an array, with one array element for each vulkan-*dll in
# C:\Windows\System32, with each element containing:
#    <major>=<minor>=<patch>=<buildno>=<prerelease>=<prebuildno>=
#     filename
#    @<major>@<minor>@<patch>@<buildno>@<prerelease>@<prebuildno>@
# [Note that the above three lines are one element in the array.]
# The build identifiers separated by "=" are suitable for sorting, i.e.
# expanded to 10 digits with leading 0s. If <prerelease> or <prebuildno> are
# not specified, "zzzzzzzzzz" is substituted for them, so that they sort
# to a position after those that do specify them.
# The build identifiers separated by "@" are the original values extracted
# from the file name. They are used later to find the path to the SDK
# install directory for the given filename.

$VulkanDllList=@()
cd c:\WINDOWS\SYSTEM32
dir -name vulkan-$majorabi-*.dll |
   ForEach-Object {
       $major=$_.Split('-')[2]
       $majorOrig=$major
       $minor=$_.Split('-')[3]
       $minorOrig=$minor
       $patch=$_.Split('-')[4]
       $patchOrig=$patch
       $buildno=$_.Split('-')[5]

       if ($buildno -match ".dll") {
           # <prerelease> and <prebuildno> are not in the name
           $buildno=$buildno -replace ".dll",""
           $buildnoOrig=$buildno
           $prerelease="z"*10
           $prereleaseOrig=""
           $prebuildno="z"*10
           $prebuildnoOrig=""
       } else {

          # We assume we don't have more than 5 dashes

          $f=$_ -replace ".dll",""
          $buildno=$f.Split('-')[5]
          $prerelease=$f.Split('-')[6]
          $prebuildno=$f.Split('-')[7]
          if ($prebuildno.Length -eq 0) {
              if ($prerelease -match "^[0-9]") {
                  # prerelease starts with a digit, it must be the prebuildno
                  $prebuildno=$prerelease
                  $prerelease=""
              }
          }
          $prereleaseOrig=$prerelease
          $prebuildnoOrig=$prebuildno

          if ($prerelease.Length -eq 0) {
              $prerelease="z"*10
          }
          if ($prebuildno.Length -eq 0) {
              $prebuildno="z"*10
          }
       }

       $major = $major.padleft(10,'0')
       $minor = $minor.padleft(10,'0')
       $patch = $patch.padleft(10,'0')
       $buildno = $buildno.padleft(10,'0')
       $prerelease = $prerelease.padleft(10,'0')
       $prebuildno = $prebuildno.padleft(10,'0')

       # Add a new element to the $VulkanDllList array
       $VulkanDllList+="$major=$minor=$patch=$buildno=$prerelease=$prebuildno= $_ @$majorOrig@$minorOrig@$patch@$buildnoOrig@$prereleaseOrig@$prebuildnoOrig@"
   }


# If $VulkanDllList contains at least one element, there's at least one vulkan*.dll file.
# Copy the most recent vulkan*.dll (named in the last element of $VulkanDllList) to vulkan-0.dll.
# Also copy the corresponding vulkaninfo-*.exe to vulkaninfo.exe.

if ($VulkanDllList.Length -gt 0) {

    # Sort the list. The most recent vulkan-*.dll will be in the last element of the list.
    [array]::sort($VulkanDllList)

    # Put the name of the most recent vulkan-*.dll in $mrVulkanDLL.
    # The most recent vulkanDLL is the second word in the last element of the
    # sorted $VulkanDllList. Copy it to $vulkandll.
    $mrVulkanDll=$VulkanDLLList[-1].Split(' ')[1]
    copy $mrVulkanDll $vulkandll

    # Copy the most recent version of vulkaninfo-<abimajor>-*.exe to vulkaninfo.exe.
    # We create the source file name for the copy from $mrVulkanDll.
    $mrVulkaninfo=$mrVulkanDll -replace ".dll",".exe"
    $mrVulkaninfo=$mrVulkaninfo -replace "vulkan","vulkaninfo"
    copy $mrVulkaninfo vulkaninfo.exe

    # Create the name used in the registry for the SDK associated with $mrVulkanDll.
    $major=$VulkanDLLList[-1].Split('@')[1]
    $minor=$VulkanDLLList[-1].Split('@')[2]
    $patch=$VulkanDLLList[-1].Split('@')[3]
    $buildno=$VulkanDLLList[-1].Split('@')[4]
    $prerelease=$VulkanDLLList[-1].Split('@')[5]
    $prebuildno=$VulkanDLLList[-1].Split('@')[6]
    $sdkname="VulkanSDK"+$major + "." + $minor + "." + $patch
    if ($prerelease -ne "") {
        $sdkname=$sdkname + "." + $prerelease
    }
    if ($prebuildno -ne "") {
        $sdkname=$sdkname + "." + $prebuildno
    }
}


# Create an array of vulkan sdk install dirs

$mrVulkanDllInstallDir=""
$VulkanSdkDirs=@()
Get-ChildItem -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall | 
   ForEach-Object {
       $regkey=$_ -replace ".*\\",""
       if ($_ -match "\\VulkanSDK") {
           # Get the install path from UninstallString
           $tmp=Get-ItemProperty -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$regkey -Name UninstallString
           $tmp=$tmp -replace "\\Uninstall.exe.*",""
           $tmp=$tmp -replace ".*=.",""
           $VulkanSdkDirs+=$tmp
           if ($regkey -eq $sdkname) {
               # Save away the sdk install dir for the the most recent vulkandll
               $mrVulkanDllInstallDir=$tmp
           }
       }
   }

# Add C:\Vulkan\SDK\0.9.3 to list of SDK install dirs.
# We do this because there is in a bug in SDK 0.9.3 in which layer
# reg entries were not removed on uninstall. So we'll try to clean up
# and remove them now.
# This works only if 0.9.3 was installed to the default location.
# If it was not installed to the default location, those entries will
# need to be cleaned up manually.

$VulkanSdkDirs+="C:\VulkanSDK\0.9.3"


# Remove layer registry entries associated with all installed Vulkan SDKs.
# Note that we remove only those entries created by Vulkan SDKs. If other
# layers were installed that are not from an SDK, we don't mess with them.

Get-Item -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers | Select-Object -ExpandProperty Property |
   ForEach-Object {
       $regval=$_
       ForEach ($sdkdir in $VulkanSdkDirs) {
          if ($regval -like "$sdkdir\*.json") {
              Remove-ItemProperty -ErrorAction Ignore -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -name $regval
          }
       }
   }


# Create layer registry entries associated with Vulkan SDK from which $mrVulkanDll is from

if ($mrVulkanDllInstallDir -ne "") {
    New-Item -ErrorAction Ignore -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers | out-null
    Get-ChildItem $mrVulkanDllInstallDir\Bin -Filter *json |
       ForEach-Object {
           New-ItemProperty -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin\$_ -PropertyType DWord -Value 0 | out-null
       }
}
