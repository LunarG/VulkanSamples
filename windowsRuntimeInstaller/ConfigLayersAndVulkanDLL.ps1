# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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

# Clear any pre-existing errors
$Error.Clear();

# Start logging
$log=$Env:Temp+"\VulkanRT"
New-Item -ItemType Directory -Force -Path $log | Out-Null
$logascii=$log+"\ConfigLayersAndVulkanDLL.log"

# Temp to be used for debugging failure to install issue
#$logascii2=$Env:Temp+"\ConfigLayersAndVulkanDLL_debug.log"

$log=$log+"\ConfigLayersAndVulkanDLL16.log"

start-transcript -path $log

Write-Host "ConfigLayersAndVulkanDLL.ps1 called with inputs of : $majorabi $ossize"
$startTime=Get-Date
Write-Host "Start time : $startTime"

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
#    <major>=<minor>=<patch>=<buildno>=<prebuildno>=<prerelease>=
#     filename
#    @<major>@<minor>@<patch>@<buildno>@<prerelease>@<prebuildno>@
# [Note that the above three lines are one element in the array.]
# The build identifiers separated by "=" are suitable for sorting, i.e.
# expanded to 10 digits with leading 0s. If <prerelease> or <prebuildno> are
# not specified, "zzzzzzzzzz" is substituted for them, so that they sort
# to a position after those that do specify them. Note that <prerelease>
# is "less significant" in the sort than <prebuildno>, and that <prerelease> is
# always treated as an alpha string, even though it may contain numeric characters.
# The build identifiers separated by "@" are the original values extracted
# from the file name. They are used later to find the path to the SDK
# install directory for the given filename.


function UpdateVulkanSysFolder([string]$dir, [int]$writeSdkName)
{
   Write-Host "UpdateVulkanSysFolder $dir $writeSdkName"

   # Push the current path on the stack and go to $dir
   Push-Location -Path $dir

   # Create a list for all the DLLs in the folder.
   # First Initialize the list to empty
   $script:VulkanDllList = @()

   # Find all vulkan dll files in this directory
   dir -name vulkan-$majorabi-*.dll |
   ForEach-Object {
       Write-Host  "File $_"
       if ($_ -match "=" -or
           $_ -match "@" -or
           $_ -match " " -or
           ($_.Split('-').count -lt 6) -or
           ($_.Split('-').count -gt 8) -or
           !$?)
       {
           # If a file name contains "=", "@", or " ", or it contains less then 5 dashes or more than
           # 7 dashes, it wasn't installed by the Vulkan Run Time.
           # Note that we need to use return inside of ForEach-Object is to continue with iteration.
           Write-Warning "Ignoring $_ - bad format"

           # Not a real error, so just clear it for now.
           $Error.Clear();

           # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
           return
       }

       # If the corresponding vulkaninfo is not present, it wasn't installed by the Vulkan Run Time
       $vulkaninfo=$_ -replace ".dll",".exe"
       $vulkaninfo=$vulkaninfo -replace "vulkan","vulkaninfo"
       if (-not (Test-Path $vulkaninfo) -or
          !$?) {
           Write-Warning "Rejected $_ - $vulkaninfo not present"

           # Not a real error, so just clear it for now.
           $Error.Clear();

           # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
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
       Write-Host "Version $majorOrig $minorOrig $patchOrig $buildnoOrig $prereleaseOrig $prebuildnoOrig"
       if (!$?) {
           Write-Warning "Ignoring version $majorOrig $minorOrig $patchOrig $buildnoOrig $prereleaseOrig $prebuildnoOrig"

           # Not a real error, so just clear it for now.
           $Error.Clear();

           # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
           return
       }

       # Make sure fields that are supposed to be numbers are numbers
       if (notNumeric($major)) {
           Write-Warning "Ignoring $_ - bad major"

           # Not a real d, so just clear it for now.
           $Error.Clear();

           # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
           return
       }
       if (notNumeric($minor)) {
           Write-Warning "Ignoring $_ - bad minor"

           # Not a real error, so just clear it for now.
           $Error.Clear();

           # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
           return
       }
       if (notNumeric($patch)) {
           Write-Warning "Ignoring $_ - bad patch"

           # Not a real error, so just clear it for now.
           $Error.Clear();

           # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
           return
       }
       if (notNumeric($buildno)) {
           Write-Warning "Ignoring $_ - bad buildno"

           # Not a real error, so just clear it for now.
           $Error.Clear();

           # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
           return
       }
       if (notNumeric($prebuildno)) {
           if ($prebuildno -ne "z"*10) {
               Write-Warning "Ignoring $_ - bad prebuildno"

               # Not a real error, so just clear it for now.
               $Error.Clear();

               # NOTE: Inside a ForEach-Object block, the 'return' call behaves like a 'continue' for a For loop
               return
           }
       }

       $major = $major.padleft(10,'0')
       $minor = $minor.padleft(10,'0')
       $patch = $patch.padleft(10,'0')
       $buildno = $buildno.padleft(10,'0')
       $prerelease = $prerelease.padright(10,'z')
       $prebuildno = $prebuildno.padleft(10,'0')

       # Add a new element to the $VulkanDllList array
       Write-Host "Adding $_ to Vulkan dll list "
       $script:VulkanDllList+="$major=$minor=$patch=$buildno=$prebuildno=$prerelease= $_ @$majorOrig@$minorOrig@$patchOrig@$buildnoOrig@$prereleaseOrig@$prebuildnoOrig@"
       if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder adding DLL $_ to list"
       }
   }

    # If $VulkanDllList contains at least one element, there's at least one vulkan*.dll file.
    # Copy the most recent vulkan*.dll (named in the last element of $VulkanDllList) to vulkan-$majorabi.dll.

    if ($script:VulkanDllList.Length -gt 0) {

        # Sort the list. The most recent vulkan-*.dll will be in the last element of the list.
        [array]::sort($script:VulkanDllList)
        if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder sorting DLL list" 
        }

        # Put the name of the most recent vulkan-*.dll in $mrVulkanDLL.
        # The most recent vulkanDLL is the second word in the last element of the
        # sorted $VulkanDllList. Copy it to $vulkandll.
        $mrVulkanDll=$script:VulkanDllList[-1].Split(' ')[1]
        Write-Host "Copying $mrVulkanDll $vulkandll"
        Copy-Item $mrVulkanDll $vulkandll -force
        if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder encountered error during copy $mrVulkanDll $vulkandll"
        }

        # Copy the most recent version of vulkaninfo-<abimajor>-*.exe to vulkaninfo.exe.
        # We create the source file name for the copy from $mrVulkanDll.
        $mrVulkaninfo=$mrVulkanDll -replace ".dll",".exe"
        $mrVulkaninfo=$mrVulkaninfo -replace "vulkan","vulkaninfo"
        Write-Host "Copying $mrVulkaninfo vulkaninfo.exe"
        Copy-Item $mrVulkaninfo vulkaninfo.exe -force
        if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder encountered error during copy $mrVulkaninfo vulkaninfo.exe"
        }

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

        Write-Host "sdkname = $sdktempname"
        if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder encountered error generating SDK name"
        }
    }

    # Return to our previous folder
    Pop-Location
    if (!$?) {
       Write-Error "Error: UpdateVulkanSysFolder popping location"
    }

    # Only update the overall script-scope SDK name if we're told to
    if ($writeSdkName -ne 0) {
        $script:sdkname = $sdktempname
    }

    return
}

# We only care about SYSWOW64 if we're targeting a 64-bit OS
if ($ossize -eq 64) {
    # Update the SYSWOW64 Vulkan DLLS/EXEs
    Write-Host "Calling UpdateVulkanSysFolder $winfolder\SYSWOW64 0"
    UpdateVulkanSysFolder $winfolder\SYSWOW64 0
    if (!$?) {
        Write-Error "Error: Calling UpdateVulkanSysFolder for 64-bit OS" 
    }
}

# Update the SYSTEM32 Vulkan DLLS/EXEs
Write-Host "Calling UpdateVulkanSysFolder $winfolder\SYSTEM32 1"
UpdateVulkanSysFolder $winfolder\SYSTEM32 1
if (!$?) {
    Write-Error "Error: Calling UpdateVulkanSysFolder for all OS"
}

# Create an array of vulkan sdk install dirs

Write-Host "Creating array of of Vulkan SDK Install dirs"
$mrVulkanDllInstallDir=""
$VulkanSdkDirs=@()
Get-ChildItem -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall |
   ForEach-Object {
       $regkey=$_ -replace ".*\\",""
       if ($_ -match "\\VulkanSDK") {
           # Get the install path from UninstallString
           $tmp=Get-ItemProperty -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$regkey -Name UninstallString
           if (!$?) {
               Write-Error "Error: Get-ItemProperty failed for Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$regkey"
           }
           $tmp=$tmp -replace "\\Uninstall.exe.*",""
           $tmp=$tmp -replace ".*=.",""
           Write-Host "Adding $tmp to VulkanSDKDirs"
           $VulkanSdkDirs+=$tmp
           if ($regkey -eq $script:sdkname) {
               # Save away the sdk install dir for the the most recent vulkandll
               Write-Host "Setting mrVulkanDllInstallDir to $tmp"
               $mrVulkanDllInstallDir=$tmp
           }
       }
   }
if (!$?) {
    Write-Error "Error: Failed creating array of of Vulkan SDK Install dirs"
}


# Search list of sdk install dirs for an sdk compatible with $script:sdkname.
# We go backwards through VulkanDllList to generate SDK names, because we want the most recent SDK.
if ($mrVulkanDllInstallDir -eq "") {
    Write-Host "Searching VulkanDllList"
    ForEach ($idx in ($script:VulkanDllList.Length-1)..0) {
        $tmp=$script:VulkanDllList[$idx]
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
        Write-Host "Comparing $regEntry"
        $rval=Get-ItemProperty -Path HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$regEntry -ErrorAction SilentlyContinue
        if (!$?) {
            Write-Warning "Ignoring $regEntry because Get-ItemProperty failed to find corresponding SDK registry entry"
            $Error.Clear();
            continue
        }
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
                Write-Host "Setting mrVulkanDllInstallDir to $instDir"
                $mrVulkanDllInstallDir=$instDir
                break
            }
        }
    }
    if (!$?) {
        Write-Error "Error: Failed searching VulkanDLLList"
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

# Remove layer registry values associated with all installed Vulkan SDKs.
# Note that we remove only those entries created by Vulkan SDKs. If other
# layers were installed that are not from an SDK, we don't mess with them.

Write-Host "Removing old layer registry values from HKLM\SOFTWARE\Khronos\Vulkan\ExplicitLayers"
Get-Item -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers | Select-Object -ExpandProperty Property |
   ForEach-Object {
       $regval=$_
       ForEach ($sdkdir in $VulkanSdkDirs) {
          if ($regval -like "$sdkdir\*.json") {
              Remove-ItemProperty -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -name $regval
              if (!$?) {
                 Write-Error "Error: Remove-ItemProperty failed for -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -name $regval"
              } else {
                 Write-Host "Removed registry value $regval"
              }
          }
       }
   }

if (!$?) {
    Write-Error "Error: Failed Removing old layer registry values from HKLM\SOFTWARE\Khronos\Vulkan\ExplicitLayers"
}

# Remove 32-bit layer registry value if we're targeting a 64-bit OS
if ($ossize -eq 64) {
   Get-Item -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers | Select-Object -ExpandProperty Property |
      ForEach-Object {
          $regval=$_
          ForEach ($sdkdir in $VulkanSdkDirs) {
             if ($regval -like "$sdkdir\*.json") {
                 Remove-ItemProperty -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers -name $regval
                 if (!$?) {
                    Write-Error "Error: Remove-ItemProperty failed for -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers -name $regval"
                 } else {
                    Write-Host "Removed WOW6432Node registry value $regval"
                 }
             }
          }
      }

    if (!$?) {
        Write-Error "Error: Failed Removing old layer registry values from HKLM\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers"
    }
}


# Create layer registry values associated with Vulkan SDK from which $mrVulkanDll is from

Write-Host "Creating new layer registry values"
if ($mrVulkanDllInstallDir -ne "") {

    # Create registry keys if they don't exist
    if (-not (Test-Path -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers)) {
        Write-Host "Creating new registry key HKLM\SOFTWARE\Khronos\Vulkan\ExplicitLayers"
        New-Item -Force -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers | out-null
        if (!$?) {
            Write-Error "Error: Failed creating HKLM\SOFTWARE\Khronos\Vulkan\ExplicitLayers"
        }
    }
    if ($ossize -eq 64) {
        if (-not (Test-Path -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers)) {
            Write-Host "Creating new registry key HKLM\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers"
            New-Item -Force -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers | out-null
            if (!$?) {
                Write-Error "Error: Failed creating HKLM\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers"
            }
        }
    }


    if ($ossize -eq 64) {
        # Create registry values in normal registry location for 64-bit items on a 64-bit OS
        Get-ChildItem $mrVulkanDllInstallDir\Bin -Filter VkLayer*json |
           ForEach-Object {
               Write-Host "Creating registry value $mrVulkanDllInstallDir\Bin\$_"
               New-ItemProperty -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin\$_ -PropertyType DWord -Value 0 | out-null
               if (!$?) {
                   Write-Error "Error: Failed creating $mrVulkanDllInstallDir\Bin\$_"
               }
           }
           if (!$?) {
               Write-Error "Error: Failed Get-ChildItem $mrVulkanDllInstallDir\Bin | ForEach-Object "
           }

        # Create registry values for the WOW6432Node registry location for 32-bit items on a 64-bit OS
        Get-ChildItem $mrVulkanDllInstallDir\Bin32 -Filter VkLayer*json |
           ForEach-Object {
               Write-Host "Creating WOW6432Node registry value $mrVulkanDllInstallDir\Bin32\$_"
               New-ItemProperty -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin32\$_ -PropertyType DWord -Value 0 | out-null
               if (!$?) {
                   Write-Error "Error: Failed creating $mrVulkanDllInstallDir\Bin32\$_"
               }
           }
           if (!$?) {
               Write-Error "Error: Failed Get-ChildItem $mrVulkanDllInstallDir\Bin32 | ForEach-Object "
           }
    } else {
        # Create registry values in normal registry location for 32-bit items on a 32-bit OS
        Get-ChildItem $mrVulkanDllInstallDir\Bin32 -Filter VkLayer*json |
           ForEach-Object {
               Write-Host "Creating registry value $mrVulkanDllInstallDir\Bin\$_"
               New-ItemProperty -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin32\$_ -PropertyType DWord -Value 0 | out-null
               if (!$?) {
                   Write-Error "Error: Failed creating $mrVulkanDllInstallDir\Bin\$_"
               }
            }
            if (!$?) {
                Write-Error "Error: Failed Get-ChildItem $mrVulkanDllInstallDir\Bin32 | ForEach-Object "
            }
    }
}

# Final log output
Write-Host "ConfigLayersAndVulkanDLL.ps1 completed"
$endTime=Get-Date
Write-Host "End time: $endTime"

Stop-Transcript

# Convert logfile to ascii
Get-Content $log | Out-File -encoding ascii -filepath $logascii

# Temp to be used for debugging failure to install issue (as VulkanRT folder might get removed)
#Get-Content $log | Out-File -encoding ascii -filepath $logascii2


# Remove the unicode log as we no longer need it.
Remove-Item $log


# SIG # Begin signature block
# MIIcZgYJKoZIhvcNAQcCoIIcVzCCHFMCAQExCzAJBgUrDgMCGgUAMGkGCisGAQQB
# gjcCAQSgWzBZMDQGCisGAQQBgjcCAR4wJgIDAQAABBAfzDtgWUsITrck0sYpfvNR
# AgEAAgEAAgEAAgEAAgEAMCEwCQYFKw4DAhoFAAQUQ4I+TKoMwtXHArekRd5/bX04
# sreggheVMIIFHjCCBAagAwIBAgIQDmYEpPtQ2iBY4vC2AGq6uzANBgkqhkiG9w0B
# AQsFADByMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYD
# VQQLExB3d3cuZGlnaWNlcnQuY29tMTEwLwYDVQQDEyhEaWdpQ2VydCBTSEEyIEFz
# c3VyZWQgSUQgQ29kZSBTaWduaW5nIENBMB4XDTE1MDQzMDAwMDAwMFoXDTE2MDcw
# NjEyMDAwMFowZTELMAkGA1UEBhMCVVMxETAPBgNVBAgTCENvbG9yYWRvMRUwEwYD
# VQQHEwxGb3J0IENvbGxpbnMxFTATBgNVBAoTDEx1bmFyRywgSW5jLjEVMBMGA1UE
# AxMMTHVuYXJHLCBJbmMuMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
# jaLJm/Joxsn/IeopiGY3XPeSZeIhjSjSlQUkDyIyDGcBG7CvfiSsXIw3EgdGkjQg
# yBcW5YsPz9bPGPUjo5go7CwZaRkhW7/LmSkAlx0UAv8EMLuJrAZ3jBNZvpPPqfWd
# zgi/Rkm2gWQ6eSKouy7IjcLk+EwkeBbB+UBnYfMp0BfCPzR3mPgGAJH6efAmEaqQ
# FBCrX97joYgDqp3v8u42jALLl/Ict/GNMHLxP+QWagIHIICCRgS6s02OsildLF6R
# nqJOOG/43f2qUD4Cab65kzlI+0+uQyOl1UlxNxp0XareghGTqECsYA03j64Esxyo
# 2xrNbV2LJm9crTX6QthxywIDAQABo4IBuzCCAbcwHwYDVR0jBBgwFoAUWsS5eyoK
# o6XqcQPAYPkt9mV1DlgwHQYDVR0OBBYEFOSdVsqodGWApfCjHtAcn8sAzLBGMA4G
# A1UdDwEB/wQEAwIHgDATBgNVHSUEDDAKBggrBgEFBQcDAzB3BgNVHR8EcDBuMDWg
# M6Axhi9odHRwOi8vY3JsMy5kaWdpY2VydC5jb20vc2hhMi1hc3N1cmVkLWNzLWcx
# LmNybDA1oDOgMYYvaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL3NoYTItYXNzdXJl
# ZC1jcy1nMS5jcmwwQgYDVR0gBDswOTA3BglghkgBhv1sAwEwKjAoBggrBgEFBQcC
# ARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQuY29tL0NQUzCBhAYIKwYBBQUHAQEEeDB2
# MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wTgYIKwYBBQUH
# MAKGQmh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydFNIQTJBc3N1
# cmVkSURDb2RlU2lnbmluZ0NBLmNydDAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEB
# CwUAA4IBAQCIt1S8zfvzMQEVmdAssjrwqBaq78xhtGPLjkNF06EvtWoV6VMLI/A6
# 45KoULsaXeYuszLxNI+OT/b4HfD0e2LxImaTDZRmCLeIs+2pMLSlWDSV4okm8Vk2
# rObLBlgiI1x0PiMa1le9D832COWM4EJcH7pxM+9JfiHYMLlZbcfNEVgv6Dhhl4MG
# mOTMTl7vQNNQaJ1coNVf9m5Bez1DV9Iu2Cgd8BHp1oLVCQCHjVv0Ifj48RIPi4SQ
# khzalrnrf+L/BWRDhpLnxYasazdV5WfrMHurPuBvYUiLQNkU9SqKgRk9XrzDAfMe
# gPbGybMr0kqtbE/A/cDcTVnvRuTZnhXSMIIFMDCCBBigAwIBAgIQBAkYG1/Vu2Z1
# U0O1b5VQCDANBgkqhkiG9w0BAQsFADBlMQswCQYDVQQGEwJVUzEVMBMGA1UEChMM
# RGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMSQwIgYDVQQD
# ExtEaWdpQ2VydCBBc3N1cmVkIElEIFJvb3QgQ0EwHhcNMTMxMDIyMTIwMDAwWhcN
# MjgxMDIyMTIwMDAwWjByMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQg
# SW5jMRkwFwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMTEwLwYDVQQDEyhEaWdpQ2Vy
# dCBTSEEyIEFzc3VyZWQgSUQgQ29kZSBTaWduaW5nIENBMIIBIjANBgkqhkiG9w0B
# AQEFAAOCAQ8AMIIBCgKCAQEA+NOzHH8OEa9ndwfTCzFJGc/Q+0WZsTrbRPV/5aid
# 2zLXcep2nQUut4/6kkPApfmJ1DcZ17aq8JyGpdglrA55KDp+6dFn08b7KSfH03sj
# lOSRI5aQd4L5oYQjZhJUM1B0sSgmuyRpwsJS8hRniolF1C2ho+mILCCVrhxKhwjf
# DPXiTWAYvqrEsq5wMWYzcT6scKKrzn/pfMuSoeU7MRzP6vIK5Fe7SrXpdOYr/mzL
# fnQ5Ng2Q7+S1TqSp6moKq4TzrGdOtcT3jNEgJSPrCGQ+UpbB8g8S9MWOD8Gi6CxR
# 93O8vYWxYoNzQYIH5DiLanMg0A9kczyen6Yzqf0Z3yWT0QIDAQABo4IBzTCCAckw
# EgYDVR0TAQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwEwYDVR0lBAwwCgYI
# KwYBBQUHAwMweQYIKwYBBQUHAQEEbTBrMCQGCCsGAQUFBzABhhhodHRwOi8vb2Nz
# cC5kaWdpY2VydC5jb20wQwYIKwYBBQUHMAKGN2h0dHA6Ly9jYWNlcnRzLmRpZ2lj
# ZXJ0LmNvbS9EaWdpQ2VydEFzc3VyZWRJRFJvb3RDQS5jcnQwgYEGA1UdHwR6MHgw
# OqA4oDaGNGh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEFzc3VyZWRJ
# RFJvb3RDQS5jcmwwOqA4oDaGNGh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdp
# Q2VydEFzc3VyZWRJRFJvb3RDQS5jcmwwTwYDVR0gBEgwRjA4BgpghkgBhv1sAAIE
# MCowKAYIKwYBBQUHAgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwCgYI
# YIZIAYb9bAMwHQYDVR0OBBYEFFrEuXsqCqOl6nEDwGD5LfZldQ5YMB8GA1UdIwQY
# MBaAFEXroq/0ksuCMS1Ri6enIZ3zbcgPMA0GCSqGSIb3DQEBCwUAA4IBAQA+7A1a
# JLPzItEVyCx8JSl2qB1dHC06GsTvMGHXfgtg/cM9D8Svi/3vKt8gVTew4fbRknUP
# UbRupY5a4l4kgU4QpO4/cY5jDhNLrddfRHnzNhQGivecRk5c/5CxGwcOkRX7uq+1
# UcKNJK4kxscnKqEpKBo6cSgCPC6Ro8AlEeKcFEehemhor5unXCBc2XGxDI+7qPjF
# Emifz0DLQESlE/DmZAwlCEIysjaKJAL+L3J+HNdJRZboWR3p+nRka7LrZkPas7CM
# 1ekN3fYBIM6ZMWM9CBoYs4GbT8aTEAb8B4H6i9r5gkn3Ym6hU/oSlBiFLpKR6mhs
# RDKyZqHnGKSaZFHvMIIGajCCBVKgAwIBAgIQAwGaAjr/WLFr1tXq5hfwZjANBgkq
# hkiG9w0BAQUFADBiMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5j
# MRkwFwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMSEwHwYDVQQDExhEaWdpQ2VydCBB
# c3N1cmVkIElEIENBLTEwHhcNMTQxMDIyMDAwMDAwWhcNMjQxMDIyMDAwMDAwWjBH
# MQswCQYDVQQGEwJVUzERMA8GA1UEChMIRGlnaUNlcnQxJTAjBgNVBAMTHERpZ2lD
# ZXJ0IFRpbWVzdGFtcCBSZXNwb25kZXIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAw
# ggEKAoIBAQCjZF38fLPggjXg4PbGKuZJdTvMbuBTqZ8fZFnmfGt/a4ydVfiS457V
# WmNbAklQ2YPOb2bu3cuF6V+l+dSHdIhEOxnJ5fWRn8YUOawk6qhLLJGJzF4o9GS2
# ULf1ErNzlgpno75hn67z/RJ4dQ6mWxT9RSOOhkRVfRiGBYxVh3lIRvfKDo2n3k5f
# 4qi2LVkCYYhhchhoubh87ubnNC8xd4EwH7s2AY3vJ+P3mvBMMWSN4+v6GYeofs/s
# jAw2W3rBerh4x8kGLkYQyI3oBGDbvHN0+k7Y/qpA8bLOcEaD6dpAoVk62RUJV5lW
# MJPzyWHM0AjMa+xiQpGsAsDvpPCJEY93AgMBAAGjggM1MIIDMTAOBgNVHQ8BAf8E
# BAMCB4AwDAYDVR0TAQH/BAIwADAWBgNVHSUBAf8EDDAKBggrBgEFBQcDCDCCAb8G
# A1UdIASCAbYwggGyMIIBoQYJYIZIAYb9bAcBMIIBkjAoBggrBgEFBQcCARYcaHR0
# cHM6Ly93d3cuZGlnaWNlcnQuY29tL0NQUzCCAWQGCCsGAQUFBwICMIIBVh6CAVIA
# QQBuAHkAIAB1AHMAZQAgAG8AZgAgAHQAaABpAHMAIABDAGUAcgB0AGkAZgBpAGMA
# YQB0AGUAIABjAG8AbgBzAHQAaQB0AHUAdABlAHMAIABhAGMAYwBlAHAAdABhAG4A
# YwBlACAAbwBmACAAdABoAGUAIABEAGkAZwBpAEMAZQByAHQAIABDAFAALwBDAFAA
# UwAgAGEAbgBkACAAdABoAGUAIABSAGUAbAB5AGkAbgBnACAAUABhAHIAdAB5ACAA
# QQBnAHIAZQBlAG0AZQBuAHQAIAB3AGgAaQBjAGgAIABsAGkAbQBpAHQAIABsAGkA
# YQBiAGkAbABpAHQAeQAgAGEAbgBkACAAYQByAGUAIABpAG4AYwBvAHIAcABvAHIA
# YQB0AGUAZAAgAGgAZQByAGUAaQBuACAAYgB5ACAAcgBlAGYAZQByAGUAbgBjAGUA
# LjALBglghkgBhv1sAxUwHwYDVR0jBBgwFoAUFQASKxOYspkH7R7for5XDStnAs0w
# HQYDVR0OBBYEFGFaTSS2STKdSip5GoNL9B6Jwcp9MH0GA1UdHwR2MHQwOKA2oDSG
# Mmh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEFzc3VyZWRJRENBLTEu
# Y3JsMDigNqA0hjJodHRwOi8vY3JsNC5kaWdpY2VydC5jb20vRGlnaUNlcnRBc3N1
# cmVkSURDQS0xLmNybDB3BggrBgEFBQcBAQRrMGkwJAYIKwYBBQUHMAGGGGh0dHA6
# Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBBBggrBgEFBQcwAoY1aHR0cDovL2NhY2VydHMu
# ZGlnaWNlcnQuY29tL0RpZ2lDZXJ0QXNzdXJlZElEQ0EtMS5jcnQwDQYJKoZIhvcN
# AQEFBQADggEBAJ0lfhszTbImgVybhs4jIA+Ah+WI//+x1GosMe06FxlxF82pG7xa
# FjkAneNshORaQPveBgGMN/qbsZ0kfv4gpFetW7easGAm6mlXIV00Lx9xsIOUGQVr
# NZAQoHuXx/Y/5+IRQaa9YtnwJz04HShvOlIJ8OxwYtNiS7Dgc6aSwNOOMdgv420X
# Ewbu5AO2FKvzj0OncZ0h3RTKFV2SQdr5D4HRmXQNJsQOfxu19aDxxncGKBXp2JPl
# VRbwuwqrHNtcSCdmyKOLChzlldquxC5ZoGHd2vNtomHpigtt7BIYvfdVVEADkitr
# wlHCCkivsNRu4PQUCjob4489yq9qjXvc2EQwggbNMIIFtaADAgECAhAG/fkDlgOt
# 6gAK6z8nu7obMA0GCSqGSIb3DQEBBQUAMGUxCzAJBgNVBAYTAlVTMRUwEwYDVQQK
# EwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20xJDAiBgNV
# BAMTG0RpZ2lDZXJ0IEFzc3VyZWQgSUQgUm9vdCBDQTAeFw0wNjExMTAwMDAwMDBa
# Fw0yMTExMTAwMDAwMDBaMGIxCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2Vy
# dCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20xITAfBgNVBAMTGERpZ2lD
# ZXJ0IEFzc3VyZWQgSUQgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC
# ggEBAOiCLZn5ysJClaWAc0Bw0p5WVFypxNJBBo/JM/xNRZFcgZ/tLJz4FlnfnrUk
# FcKYubR3SdyJxArar8tea+2tsHEx6886QAxGTZPsi3o2CAOrDDT+GEmC/sfHMUiA
# fB6iD5IOUMnGh+s2P9gww/+m9/uizW9zI/6sVgWQ8DIhFonGcIj5BZd9o8dD3QLo
# Oz3tsUGj7T++25VIxO4es/K8DCuZ0MZdEkKB4YNugnM/JksUkK5ZZgrEjb7Szgau
# rYRvSISbT0C58Uzyr5j79s5AXVz2qPEvr+yJIvJrGGWxwXOt1/HYzx4KdFxCuGh+
# t9V3CidWfA9ipD8yFGCV/QcEogkCAwEAAaOCA3owggN2MA4GA1UdDwEB/wQEAwIB
# hjA7BgNVHSUENDAyBggrBgEFBQcDAQYIKwYBBQUHAwIGCCsGAQUFBwMDBggrBgEF
# BQcDBAYIKwYBBQUHAwgwggHSBgNVHSAEggHJMIIBxTCCAbQGCmCGSAGG/WwAAQQw
# ggGkMDoGCCsGAQUFBwIBFi5odHRwOi8vd3d3LmRpZ2ljZXJ0LmNvbS9zc2wtY3Bz
# LXJlcG9zaXRvcnkuaHRtMIIBZAYIKwYBBQUHAgIwggFWHoIBUgBBAG4AeQAgAHUA
# cwBlACAAbwBmACAAdABoAGkAcwAgAEMAZQByAHQAaQBmAGkAYwBhAHQAZQAgAGMA
# bwBuAHMAdABpAHQAdQB0AGUAcwAgAGEAYwBjAGUAcAB0AGEAbgBjAGUAIABvAGYA
# IAB0AGgAZQAgAEQAaQBnAGkAQwBlAHIAdAAgAEMAUAAvAEMAUABTACAAYQBuAGQA
# IAB0AGgAZQAgAFIAZQBsAHkAaQBuAGcAIABQAGEAcgB0AHkAIABBAGcAcgBlAGUA
# bQBlAG4AdAAgAHcAaABpAGMAaAAgAGwAaQBtAGkAdAAgAGwAaQBhAGIAaQBsAGkA
# dAB5ACAAYQBuAGQAIABhAHIAZQAgAGkAbgBjAG8AcgBwAG8AcgBhAHQAZQBkACAA
# aABlAHIAZQBpAG4AIABiAHkAIAByAGUAZgBlAHIAZQBuAGMAZQAuMAsGCWCGSAGG
# /WwDFTASBgNVHRMBAf8ECDAGAQH/AgEAMHkGCCsGAQUFBwEBBG0wazAkBggrBgEF
# BQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEMGCCsGAQUFBzAChjdodHRw
# Oi8vY2FjZXJ0cy5kaWdpY2VydC5jb20vRGlnaUNlcnRBc3N1cmVkSURSb290Q0Eu
# Y3J0MIGBBgNVHR8EejB4MDqgOKA2hjRodHRwOi8vY3JsMy5kaWdpY2VydC5jb20v
# RGlnaUNlcnRBc3N1cmVkSURSb290Q0EuY3JsMDqgOKA2hjRodHRwOi8vY3JsNC5k
# aWdpY2VydC5jb20vRGlnaUNlcnRBc3N1cmVkSURSb290Q0EuY3JsMB0GA1UdDgQW
# BBQVABIrE5iymQftHt+ivlcNK2cCzTAfBgNVHSMEGDAWgBRF66Kv9JLLgjEtUYun
# pyGd823IDzANBgkqhkiG9w0BAQUFAAOCAQEARlA+ybcoJKc4HbZbKa9Sz1LpMUer
# Vlx71Q0LQbPv7HUfdDjyslxhopyVw1Dkgrkj0bo6hnKtOHisdV0XFzRyR4WUVtHr
# uzaEd8wkpfMEGVWp5+Pnq2LN+4stkMLA0rWUvV5PsQXSDj0aqRRbpoYxYqioM+Sb
# OafE9c4deHaUJXPkKqvPnHZL7V/CSxbkS3BMAIke/MV5vEwSV/5f4R68Al2o/vsH
# OE8Nxl2RuQ9nRc3Wg+3nkg2NsWmMT/tZ4CMP0qquAHzunEIOz5HXJ7cW7g/DvXwK
# oO4sCFWFIrjrGBpN/CohrUkxg0eVd3HcsRtLSxwQnHcUwZ1PL1qVCCkQJjGCBDsw
# ggQ3AgEBMIGGMHIxCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMx
# GTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20xMTAvBgNVBAMTKERpZ2lDZXJ0IFNI
# QTIgQXNzdXJlZCBJRCBDb2RlIFNpZ25pbmcgQ0ECEA5mBKT7UNogWOLwtgBqursw
# CQYFKw4DAhoFAKB4MBgGCisGAQQBgjcCAQwxCjAIoAKAAKECgAAwGQYJKoZIhvcN
# AQkDMQwGCisGAQQBgjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcCARUw
# IwYJKoZIhvcNAQkEMRYEFAoOC46C6ArmxtlmLsUTidSbkN3rMA0GCSqGSIb3DQEB
# AQUABIIBADXG8YUKEPQHyMUpBvGWwb5VeZ8oWPyiSSE649GXu5tHDn+N2lhDPngR
# Cksh4FpF56hP4RgTzH/Nmxf2D4kZUzPCrs2Il1S+U0ZoFpoAwrN8dbnvw2Wvf7ns
# LZHXKG9eIaMYx6r/nn+VV8qvL/25fZ8oNyIFCYy4FYRLmla5g1+Vmtg6anHj89c+
# EMSIwR8BR5UlAagfhfKJQHYMz4xkdqMrR6ZDsMHvYjbOg3MILrrdgomH2R5JKAHK
# IaD3EqM+Tgu8LH1MMt/hIf4RS/lgqT12qM88J64dyhquaM1BUzw5dnwb+h3aAF2O
# ZJ4IhONECJbJf8wVT8rTUA+6uUm/Y6ihggIPMIICCwYJKoZIhvcNAQkGMYIB/DCC
# AfgCAQEwdjBiMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkw
# FwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMSEwHwYDVQQDExhEaWdpQ2VydCBBc3N1
# cmVkIElEIENBLTECEAMBmgI6/1ixa9bV6uYX8GYwCQYFKw4DAhoFAKBdMBgGCSqG
# SIb3DQEJAzELBgkqhkiG9w0BBwEwHAYJKoZIhvcNAQkFMQ8XDTE2MDQwNzIxNTM0
# OFowIwYJKoZIhvcNAQkEMRYEFERKj5qmhGjIIeKV/myZFhJ/EUO7MA0GCSqGSIb3
# DQEBAQUABIIBAJYQytkxzpn/UVwVJ0tsompGzVKSEgjuqAiI2jA0LiQwWM9iBHpG
# 8ijDH6Dh+Fqa1JSsynyFixF26SuHeDoY/LX14HhRPDEkBa70qt9h9gc/73f9AzUy
# eSbxwRhlF/UAyk0E3fbK1of431HxfvcdhnvCIDW8orfiG5v7gS0Mub4C70TlMXTp
# b6XT1orYqnih9j4EVCYWZwv+EsOADRHW7o1RvIC2gI2dzmAkMSEjehk3we6u8KXI
# xkggPOXy5O8TFgFdjvKU6XaoTTCklKWFIQIRG9r5m//Qj3jwzwN/03gLPphi6zze
# 8fAJmClDyH+kHivSSfFnFUB7elvajTvasQE=
# SIG # End signature block
