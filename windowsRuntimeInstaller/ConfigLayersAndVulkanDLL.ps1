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

#Note that these two vars need to be prepended to this script.
#InstallerRT.nsi does this before calling this script.
#$majorabi=
#$ossize=

# Clear any pre-existing errors and set default return value
$Error.Clear();
$script:scriptReturnValue=0

# Start logging
$logascii=$Env:Temp+"\ConfigLayersAndVulkanDLL.log"
$log=$Env:Temp+"\ConfigLayersAndVulkanDLL16.log"
start-transcript -path $log

# Ignore errors related to log file
$Error.Clear();

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

function Get-CurrentLineNumber {
    $MyInvocation.ScriptLineNumber
}

function setScriptReturnValue($rvalue) {
    if ($script:scriptReturnValue -eq 0) {
        $script:scriptReturnValue = $rvalue
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

           # Not a real error, so just clear it for now.
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
           setScriptReturnValue(Get-CurrentLineNumber)
       }
   }

    # If $VulkanDllList contains at least one element, there's at least one vulkan*.dll file.
    # Copy the most recent vulkan*.dll (named in the last element of $VulkanDllList) to vulkan-$majorabi.dll.

    if ($script:VulkanDllList.Length -gt 0) {

        # Sort the list. The most recent vulkan-*.dll will be in the last element of the list.
        [array]::sort($script:VulkanDllList)
        if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder sorting DLL list" 
           setScriptReturnValue(Get-CurrentLineNumber)
        }

        # Put the name of the most recent vulkan-*.dll in $mrVulkanDLL.
        # The most recent vulkanDLL is the second word in the last element of the
        # sorted $VulkanDllList. Copy it to $vulkandll.
        $mrVulkanDll=$script:VulkanDllList[-1].Split(' ')[1]
        Write-Host "Copying $mrVulkanDll $vulkandll"
        Copy-Item $mrVulkanDll $vulkandll -force
        if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder encountered error during copy $mrVulkanDll $vulkandll"
           setScriptReturnValue(Get-CurrentLineNumber)
        }

        # Copy the most recent version of vulkaninfo-<abimajor>-*.exe to vulkaninfo.exe.
        # We create the source file name for the copy from $mrVulkanDll.
        $mrVulkaninfo=$mrVulkanDll -replace ".dll",".exe"
        $mrVulkaninfo=$mrVulkaninfo -replace "vulkan","vulkaninfo"
        Write-Host "Copying $mrVulkaninfo vulkaninfo.exe"
        Copy-Item $mrVulkaninfo vulkaninfo.exe -force
        if (!$?) {
           Write-Error "Error: UpdateVulkanSysFolder encountered error during copy $mrVulkaninfo vulkaninfo.exe"
           setScriptReturnValue(Get-CurrentLineNumber)
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
           setScriptReturnValue(Get-CurrentLineNumber)
        }
    }

    # Return to our previous folder
    Pop-Location
    if (!$?) {
       Write-Error "Error: UpdateVulkanSysFolder popping location"
       setScriptReturnValue(Get-CurrentLineNumber)
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
        setScriptReturnValue(Get-CurrentLineNumber)
    }
}

# Update the SYSTEM32 Vulkan DLLS/EXEs
Write-Host "Calling UpdateVulkanSysFolder $winfolder\SYSTEM32 1"
UpdateVulkanSysFolder $winfolder\SYSTEM32 1
if (!$?) {
    Write-Error "Error: Calling UpdateVulkanSysFolder for all OS"
    setScriptReturnValue(Get-CurrentLineNumber)
}

# Create an array of vulkan sdk install dirs

Write-Host "Creating array of of Vulkan SDK Install dirs"
$mrVulkanDllInstallDir=""
$VulkanSdkDirs=@()
$installSDKRegs = @(Get-ChildItem -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall)
if ($installSDKRegs -ne $null) {
   ForEach ($curSDKReg in $installSDKRegs) {
      if ($curSDKReg -ne $null) {
         $regkey=$curSDKReg -replace ".*\\",""
         if ($regkey -match "VulkanSDK") {
             # Get the install path from UninstallString
             $tmp=Get-ItemProperty -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$regkey -Name UninstallString
             if (!$? -or $tmp -eq $null) {
                 Write-Warning "Error: Get-ItemProperty failed for Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$regkey"
                 $Error.Clear();
                 continue;
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
   }
}
if (!$?) {
    Write-Error "Error: Failed creating array of of Vulkan SDK Install dirs"
    setScriptReturnValue(Get-CurrentLineNumber)
}


# Search list of sdk install dirs for an sdk compatible with $script:sdkname.
# We go backwards through VulkanDllList to generate SDK names, because we want the most recent SDK.

if ($mrVulkanDllInstallDir -eq "" -and $script:VulkanDllList.Length -gt 0) {
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
        if (!$? -or $rval -eq $null) {
            Write-Warning "Ignoring $regEntry - corresponding SDK registry entry does not exist"
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
        Write-Warning "Failed searching VulkanDLLList"
        $Error.Clear();
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
$regkeys = @(Get-Item -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers | Select-Object -ExpandProperty Property)
if ($regkeys -ne $null) {
   ForEach ($regval in $regkeys) {
       if ($regval -ne $null) {
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
    }
}

if (!$?) {
    Write-Error "Error: Failed Removing old layer registry values from HKLM\SOFTWARE\Khronos\Vulkan\ExplicitLayers"
    setScriptReturnValue(Get-CurrentLineNumber)
}

# Remove 32-bit layer registry value if we're targeting a 64-bit OS
if ($ossize -eq 64) {
   $regkeys = @(Get-Item -Path Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers | Select-Object -ExpandProperty Property)
   if ($regkeys -ne $null) {
      ForEach ($regval in $regkeys) {
          if ($regval -ne $null) {
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
      }
   }

   if (!$?) {
      Write-Error "Error: Failed Removing old layer registry values from HKLM\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers"
      setScriptReturnValue(Get-CurrentLineNumber)
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
            setScriptReturnValue(Get-CurrentLineNumber)
        }
    }
    if ($ossize -eq 64) {
        if (-not (Test-Path -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers)) {
            Write-Host "Creating new registry key HKLM\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers"
            New-Item -Force -ErrorAction SilentlyContinue -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers | out-null
            if (!$?) {
                Write-Error "Error: Failed creating HKLM\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers"
                setScriptReturnValue(Get-CurrentLineNumber)
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
                   setScriptReturnValue(Get-CurrentLineNumber)
               }
           }
           if (!$?) {
               Write-Error "Error: Failed Get-ChildItem $mrVulkanDllInstallDir\Bin | ForEach-Object "
               setScriptReturnValue(Get-CurrentLineNumber)
           }

        # Create registry values for the WOW6432Node registry location for 32-bit items on a 64-bit OS
        Get-ChildItem $mrVulkanDllInstallDir\Bin32 -Filter VkLayer*json |
           ForEach-Object {
               Write-Host "Creating WOW6432Node registry value $mrVulkanDllInstallDir\Bin32\$_"
               New-ItemProperty -Path HKLM:\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin32\$_ -PropertyType DWord -Value 0 | out-null
               if (!$?) {
                   Write-Error "Error: Failed creating $mrVulkanDllInstallDir\Bin32\$_"
                   setScriptReturnValue(Get-CurrentLineNumber)
               }
           }
           if (!$?) {
               Write-Error "Error: Failed Get-ChildItem $mrVulkanDllInstallDir\Bin32 | ForEach-Object "
               setScriptReturnValue(Get-CurrentLineNumber)
           }
    } else {
        # Create registry values in normal registry location for 32-bit items on a 32-bit OS
        Get-ChildItem $mrVulkanDllInstallDir\Bin32 -Filter VkLayer*json |
           ForEach-Object {
               Write-Host "Creating registry value $mrVulkanDllInstallDir\Bin\$_"
               New-ItemProperty -Path HKLM:\SOFTWARE\Khronos\Vulkan\ExplicitLayers -Name $mrVulkanDllInstallDir\Bin32\$_ -PropertyType DWord -Value 0 | out-null
               if (!$?) {
                   Write-Error "Error: Failed creating $mrVulkanDllInstallDir\Bin\$_"
                   setScriptReturnValue(Get-CurrentLineNumber)
               }
            }
            if (!$?) {
                Write-Error "Error: Failed Get-ChildItem $mrVulkanDllInstallDir\Bin32 | ForEach-Object "
                setScriptReturnValue(Get-CurrentLineNumber)
            }
    }
}

# Debug - for testing handling of script failure in installer
#setScriptReturnValue(Get-CurrentLineNumber)

# Final log output
Write-Host "ConfigLayersAndVulkanDLL.ps1 completed"
$endTime=Get-Date
Write-Host "End time: $endTime"

Stop-Transcript

# Convert logfile to ascii
Get-Content $log | Out-File -encoding ascii -filepath $logascii

# Remove the unicode log as we no longer need it.
Remove-Item $log

exit $script:scriptReturnValue
