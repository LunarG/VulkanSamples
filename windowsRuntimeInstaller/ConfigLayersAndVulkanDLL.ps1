# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and/or associated documentation files (the "Materials"), to
# deal in the Materials without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Materials, and to permit persons to whom the Materials are
# furnished to do so, subject to the following conditions:
#
# The above copyright notice(s) and this permission notice shall be included in
# all copies or substantial portions of the Materials.
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
# USE OR OTHER DEALINGS IN THE MATERIALS.
#
# Author: David Pinedo <david@LunarG.com>
# Author: Mark Young <mark@LunarG.com>
#


# This Powershell script is used by the Vulkan Run Time Installer/Uninstaller to:
#   - Copy the most recent vulkan-<majorabi>-*.dll in C:\Windows\System32
#     to vulkan-<majorabi>.dll
#   - Copy the most recent version of vulkaninfo-<abimajor>-*.exe in
#     C:\Windows\System32 to vulkaninfo.exe
#   - The same thing is done for those files in C:\Windows\SysWOW64 on a 64-bit
#     target.
#   - Set the layer registry entries to point to the layer json files
#     in the Vulkan SDK associated with the most recent vulkan*dll.
#
# This script takes the following parameters:
#   $majorabi : a single string number specifying the major abi version.
#   $ossize     : an integer indicating if the target is a 64 (64) or 32 (32) bit OS.
#

Param(
 [string]$majorabi,
 [int]$ossize
)

$vulkandll = "vulkan-"+$majorabi+".dll"
$windrive  = $env:SYSTEMDRIVE
$winfolder = $env:SYSTEMROOT
$script:VulkanDllList=@()

function notNumeric ($x) {
    try {
        0 + $x | Out-Null
        return $false
    } catch {
        return $true
    }
}

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
# C:\Windows\System32 (and C:\Windows\SysWOW64 on 64-bit systems), with each element
# containing:
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


function UpdateVulkanSysFolder([string]$dir, [int]$writeSdkName)
{
   # Push the current path on the stack and go to $dir
   Push-Location -Path $dir

   # Create a list for all the DLLs in the folder.
   # First Initialize the list to empty
   $script:VulkanDllList = @()

   # Find all DLL objects in this directory
   dir -name vulkan-$majorabi-*.dll |
   ForEach-Object {
       if ($_ -match "=" -or
           $_ -match "@" -or
           $_ -match " " -or
           ($_.Split('-').count -lt 6)  -or
           ($_.Split('-').count -gt 8))
       {
           # If a file name contains "=", "@", or " ", or it contains less then 5 dashes or more than
           # 7 dashes, it wasn't installed by the Vulkan Run Time.
           # Note that we need to use return inside of ForEach-Object is to continue with iteration.
           return
       }
       $major=$_.Split('-')[2]
       $majorOrig=$major
       $minor=$_.Split('-')[3]
       $minorOrig=$minor
       $patch=$_.Split('-')[4]
       $patchOrig=$patch
       $buildno=$_.Split('-')[5]

       if ($buildno -match ".dll") {
          # prerelease and prebuildno are not in the name
          # Extract buildno, and set prerelease and prebuildno to "z"s
          $buildno=$buildno -replace ".dll",""
          $buildnoOrig=$buildno
          $prerelease="z"*10
          $prereleaseOrig=""
          $prebuildno="z"*10
          $prebuildnoOrig=""
       } else {
          # Extract buildno, prerelease, and prebuildno
          $f=$_ -replace ".dll",""
          $buildno=$f.Split('-')[5]
          $buildnoOrig=$buildno
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

       # Make sure fields that are supposed to be numbers are numbers
       if (notNumeric($major)) {return}
       if (notNumeric($minor)) {return}
       if (notNumeric($patch)) {return}
       if (notNumeric($buildno)) {return}
       if (notNumeric($prebuildno)) {
           if ($prebuildno -ne "z"*10) {return}
       }

       $major = $major.padleft(10,'0')
       $minor = $minor.padleft(10,'0')
       $patch = $patch.padleft(10,'0')
       $buildno = $buildno.padleft(10,'0')
       $prerelease = $prerelease.padleft(10,'0')
       $prebuildno = $prebuildno.padleft(10,'0')

       # Add a new element to the $VulkanDllList array
       $script:VulkanDllList+="$major=$minor=$patch=$buildno=$prerelease=$prebuildno= $_ @$majorOrig@$minorOrig@$patchOrig@$buildnoOrig@$prereleaseOrig@$prebuildnoOrig@"
   }

    # If $VulkanDllList contains at least one element, there's at least one vulkan*.dll file.
    # Copy the most recent vulkan*.dll (named in the last element of $VulkanDllList) to vulkan-$majorabi.dll.

    if ($script:VulkanDllList.Length -gt 0) {

        # Sort the list. The most recent vulkan-*.dll will be in the last element of the list.
        [array]::sort($script:VulkanDllList)

        # Put the name of the most recent vulkan-*.dll in $mrVulkanDLL.
        # The most recent vulkanDLL is the second word in the last element of the
        # sorted $VulkanDllList. Copy it to $vulkandll.
        $mrVulkanDll=$script:VulkanDllList[-1].Split(' ')[1]
        copy $mrVulkanDll $vulkandll

        # Copy the most recent version of vulkaninfo-<abimajor>-*.exe to vulkaninfo.exe.
        # We create the source file name for the copy from $mrVulkanDll.
        $mrVulkaninfo=$mrVulkanDll -replace ".dll",".exe"
        $mrVulkaninfo=$mrVulkaninfo -replace "vulkan","vulkaninfo"
        copy $mrVulkaninfo vulkaninfo.exe

        # Create the name used in the registry for the SDK associated with $mrVulkanDll.
        $major=$script:VulkanDllList[-1].Split('@')[1]
        $minor=$script:VulkanDllList[-1].Split('@')[2]
        $patch=$script:VulkanDllList[-1].Split('@')[3]
        $buildno=$script:VulkanDllList[-1].Split('@')[4]
        $prerelease=$script:VulkanDllList[-1].Split('@')[5]
        $prebuildno=$script:VulkanDllList[-1].Split('@')[6]

        $sdktempname="VulkanSDK"+$major + "." + $minor + "." + $patch + "." + $buildno
        if ($prerelease -ne "") {
            $sdktempname=$sdktempname + "." + $prerelease
        }
        if ($prebuildno -ne "") {
            $sdktempname=$sdktempname + "." + $prebuildno
        }
    }

    # Return to our previous folder
    Pop-Location

    # Only update the overall script-scope SDK name if we're told to
    if ($writeSdkName -ne 0) {
        $script:sdkname = $sdktempname
    }

    return
}

# We only care about SYSWOW64 if we're targeting a 64-bit OS
if ($ossize -eq 64) {
    # Update the SYSWOW64 Vulkan DLLS/EXEs
    UpdateVulkanSysFolder $winfolder\SYSWOW64 0
}

# Update the SYSTEM32 Vulkan DLLS/EXEs
UpdateVulkanSysFolder $winfolder\SYSTEM32 1

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
           if ($regkey -eq $script:sdkname) {
               # Save away the sdk install dir for the the most recent vulkandll
               $mrVulkanDllInstallDir=$tmp
           }
       }
   }


# Search list of sdk install dirs for an sdk compatible with $script:sdkname.
# We go backwards through VulkanDllList to generate SDK names, because we want the most recent SDK.
if ($mrVulkanDllInstallDir -eq "") {
    ForEach ($idx in ($script:VulkanDllList.Length-1)..0) {
        $vulkanDllMajor=$script:VulkanDllList[$idx].Split('@')[1]
        $vulkanDllMinor=$script:VulkanDllList[$idx].Split('@')[2]
        $vulkanDllPatch=$script:VulkanDllList[$idx].Split('@')[3]
        $vulkanDllBuildno=$script:VulkanDllList[$idx].Split('@')[4]
        $vulkanDllPrerelease=$script:VulkanDllList[$idx].Split('@')[5]
        $vulkanDllPrebuildno=$script:VulkanDllList[$idx].Split('@')[6]
        $regEntry="VulkanSDK"+$vulkanDllMajor+"."+$vulkanDllMinor+"."+$vulkanDllPatch+"."+$vulkanDllBuildno
        if ($vulkanDllPrerelease) {
            $regEntry=$regEntry+"."+$vulkanDllPrerelease
        }
        if ($vulkanDllPrebuildno) {
            $regEntry=$regEntry+"."+$vulkanDllPrebuildno
        }
        $rval=Get-ItemProperty -Path HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$regEntry -ErrorAction SilentlyContinue
        $instDir=$rval
        $instDir=$instDir -replace "\\Uninstall.exe.*",""
        $instDir=$instDir -replace ".*=.",""
        if ($rval) {
            $rval=$rval -replace ".* DisplayVersion=",""
            $rval=$rval -replace ";.*",""
            $reMajor=$rval.Split('.')[0]
            $reMinor=$rval.Split('.')[1]
            $rePatch=$rval.Split('.')[2]
            if ($reMajor+$reMinor+$rePatch -eq $vulkanDllMajor+$vulkanDllMinor+$vulkanDllPatch) {
                $mrVulkanDllInstallDir=$instDir
                break
            }
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
$VulkanSdkDirs+="$windrive\VulkanSDK\0.9.3"

# Remove layer registry entries associated with all installed Vulkan SDKs.
# Note that we remove only those entries created by Vulkan SDKs. If other
# layers were installed that are not from an SDK, we don't mess with them.

Get-Item -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers | Select-Object -ExpandProperty Property |
   ForEach-Object {
       $regval=$_
       ForEach ($sdkdir in $VulkanSdkDirs) {
          if ($regval -like "$sdkdir\*.json") {
              Remove-ItemProperty -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -name $regval
          }
       }
   }
# Remove 32-bit layer registry entries if we're targeting a 64-bit OS
if ($ossize -eq 64) {
   Get-Item -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers | Select-Object -ExpandProperty Property |
      ForEach-Object {
          $regval=$_
          ForEach ($sdkdir in $VulkanSdkDirs) {
             if ($regval -like "$sdkdir\*.json") {
                 Remove-ItemProperty -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers -name $regval
             }
          }
      }
}


# Create layer registry entries associated with Vulkan SDK from which $mrVulkanDll is from

if ($mrVulkanDllInstallDir -ne "") {
    if ($ossize -eq 64) {
    
        # Create registry entires in normal registry location for 64-bit items on a 64-bit OS
        New-Item -Force -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers | out-null
        Get-ChildItem $mrVulkanDllInstallDir\Bin -Filter VkLayer*json |
           ForEach-Object {
               New-ItemProperty -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin\$_ -PropertyType DWord -Value 0 | out-null
           }

        # Create registry entires for the WOW6432Node registry location for 32-bit items on a 64-bit OS
        New-Item -Force -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers | out-null
        Get-ChildItem $mrVulkanDllInstallDir\Bin32 -Filter VkLayer*json |
           ForEach-Object {
               New-ItemProperty -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin32\$_ -PropertyType DWord -Value 0 | out-null
           }
           
    } else {
    
        # Create registry entires in normal registry location for 32-bit items on a 32-bit OS
        New-Item -Force -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers | out-null
        Get-ChildItem $mrVulkanDllInstallDir\Bin32 -Filter VkLayer*json |
           ForEach-Object {
               New-ItemProperty -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin32\$_ -PropertyType DWord -Value 0 | out-null
           }
    
    }
}
