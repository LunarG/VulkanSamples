# NSIS Script for creating the Windows Vulkan RT installer.
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
# Author: David Pinedo <david@lunarg.com>
#


# Version information
# Set VERSION_PATCH to:
#    x.pre.y for prereleases
#    a single number for releases
!define PRODUCTNAME "VulkanRT"
!define VERSION_ABI_MAJOR "0"
!define VERSION_API_MAJOR "0"
!define VERSION_MINOR "10"
!define VERSION_PATCH "3.pre.1"
#!define VERSION_PATCH "1"
!define PRODUCTVERSION "${VERSION_API_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"

# This number is determined by doing an install, and then from Windows Explorer,
# doing a "Properties" on the install directory. Add to this the size of the
# files installed to C:\Windows\System32. And then add a little bit more.
.  The units are 1K bytes.
!define ESTIMATEDSIZE "340"

# Set the icon
!define ICOFILE "Vulkan.ico"
Icon ${ICOFILE}
UninstallIcon ${ICOFILE}
WindowIcon off

# Define name of installer
OutFile "VulkanRT-${PRODUCTVERSION}-Installer.exe"

# Define default installation directory
InstallDir "C:\Program Files (x86)\${PRODUCTNAME}\${PRODUCTVERSION}"

# Includes
!include StrRep.nsh
!include x64.nsh
!include LogicLib.nsh

# Declar var that holds version string used in file names
Var FV

Function .onInit

  # Make sure we are on Win64.
  ${IfNot} ${RunningX64}
     MessageBox MB_OK "The Vulkan Run Time Libraries must be installed on a Windows 64-bit system."
     abort
  ${Endif}

FunctionEnd

# Need admin to write to C:\Windows\System32 and install dir
RequestExecutionLevel admin

AddBrandingImage left 150
Caption "${PRODUCTNAME} ${PRODUCTVERSION} Setup"
Name "${PRODUCTNAME} ${PRODUCTVERSION}"
Page custom brandimage "" ": Brand Image"
Page directory
Page instfiles
UninstallCaption "\${PRODUCTNAME} ${PRODUCTVERSION} Uninstall"
UninstallText "This wizard will uninstall ${PRODUCTNAME} ${PRODUCTVERSION} from your computer. Click Uninstall to start the uninstallation."
UninstPage custom un.brandimage "" ": Brand Image"
UninstPage uninstConfirm
UninstPage instFiles

# Start default section
Section

    ${DisableX64FSRedirection}
    SetRegView 64

    # Set up version number for file names
    ${StrRep} $0 ${VERSION_PATCH} "." "-"
    StrCpy $FV ${VERSION_ABI_MAJOR}-${VERSION_API_MAJOR}-${VERSION_MINOR}-$0

    # Libraries
    SetOutPath $WINDIR\System32
    File /oname=vulkan-$FV.dll ..\build\loader\Release\vulkan-${VERSION_ABI_MAJOR}.dll

    # vulkaninfo.exe
    SetOutPath $WINDIR\System32
    File /oname=vulkaninfo-$FV.exe ..\build\demos\Release\vulkaninfo.exe
    SetOutPath "$INSTDIR"
    File ..\build\demos\Release\vulkaninfo.exe
    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}"
    CreateDirectory "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos"
    CreateShortCut "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos\vulkaninfo.lnk" "$INSTDIR\vulkaninfo.exe"

    SetOutPath "$INSTDIR"
    File ${ICOFILE}
    File LICENSE.rtf
    File ConfigLayersAndVulkanDLL.ps1

    # Run the ConfigLayersAndVulkanDLL.ps1 script to copy the most recent version of
    # vulkan-<abimajor>-*.dll to vulkan-<abimajor>.dll, and to set up layer registry
    # entries to use layers from the corresponding SDK
    nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File ConfigLayersAndVulkanDLL.ps1 ${VERSION_ABI_MAJOR}'

    # We are done using ConfigLayersAndVulkanDLL.ps1, delete it. It will be re-installed
    # by the uninstaller when it needs to be run again during uninstall.
    Delete ConfigLayersAndVulkanDLL.ps1

    # Reference count the number of times we have been installed.
    # The reference count is stored in the regisry value IC
    ReadRegDword $1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "IC"
    IntOp $1 $1 + 1
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "IC" $1

    # Create the uninstaller
    WriteUninstaller "$INSTDIR\Uninstall${PRODUCTNAME}.exe"

    # Modify registry for Programs and Features
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "DisplayName" "Vulkan Run Time Libraries ${PRODUCTVERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "UninstallString" "$INSTDIR\Uninstall${PRODUCTNAME}.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "Publisher" "LunarG, Inc."
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "DisplayVersion" "${PRODUCTVERSION}"
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "VersionABIMajor" ${VERSION_ABI_MAJOR}
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "VersionAPIMajor" ${VERSION_API_MAJOR}
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "VersionMinor" ${VERSION_MINOR}
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "VersionMinor" ${VERSION_PATCH}
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "EstimatedSize" ${ESTIMATEDSIZE}
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "DisplayIcon" "$\"$INSTDIR\${ICOFILE}$\""

    # Possibly install MSVC 2013 redistributables
    ReadRegDword $1 HKLM "SOFTWARE\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum" "Install"
    IntCmp $1 1 RedistributablesInstalled InstallRedistributables InstallRedistributables
    InstallRedistributables:
       SetOutPath "$TEMP"
       File vcredist_x64.exe
       ExecWait '"$TEMP\vcredist_x64.exe"  /passive /norestart'
     RedistributablesInstalled:

SectionEnd

# Uninstaller section start
Section "uninstall"

    ${DisableX64FSRedirection}
    SetRegView 64

    SetOutPath "$INSTDIR"

    # Set up version number for file names
    ${StrRep} $0 ${VERSION_PATCH} "." "-"
    StrCpy $FV ${VERSION_ABI_MAJOR}-${VERSION_API_MAJOR}-${VERSION_MINOR}-$0

    # Decrement the number of times we have been installed.
    ReadRegDword $1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "IC"
    IntOp $1 $1 - 1
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "IC" $1

    # Ref count is in $1. If it is zero, uninstall.
    ${If} $1 <= 0

        # Install the ConfigLayersAndVulkanDLL.ps1 so we can run it.
        # It will be deleted later when we remove the install directory.
        File ConfigLayersAndVulkanDLL.ps1

        # Delete vulkaninfo.exe in C:\Windows\System32
        Delete /REBOOTOK $WINDIR\System32\vulkaninfo.exe
        Delete /REBOOTOK "$WINDIR\System32\vulkaninfo-$FV.exe"

        # Delete vullkan dll files: vulkan-<majorabi>.dll and vulkan-<majorabi>-<major>-<minor>-<patch>.dll
        Delete /REBOOTOK $WINDIR\System32\vulkan-${VERSION_ABI_MAJOR}.dll
        ${StrRep} $0 ${VERSION_PATCH} "." "-"
        Delete /REBOOTOK $WINDIR\System32\vulkan-$FV.dll

        # Run the ConfigLayersAndVulkanDLL.ps1 script to:
        #   Copy the most recent version of vulkan-<abimajor>-*.dll to vulkan-<abimajor>.dll
        #   Copy the most recent version of vulkaninfo-<abimajor>-*.exe to vulkaninfo.exe
        #   Set up layer registry entries to use layers from the corresponding SDK
        nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File "$INSTDIR\ConfigLayersAndVulkanDLL.ps1" ${VERSION_ABI_MAJOR}'

        # Delete vulkaninfo from start menu.
        # Delete vulkan start menu if the vulkan start menu is empty
        SetShellVarContext all
        Delete "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos\vulkaninfo.lnk"
        StrCpy $0 "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos"
        Call un.DeleteDirIfEmpty
        StrCpy $0 "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}"
        Call un.DeleteDirIfEmpty

        # Modify registry for Programs and Features
        DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}"

        # Remove files in install dir
        Delete /REBOOTOK "$INSTDIR\LICENSE.rtf"
        Delete /REBOOTOK "$INSTDIR\UninstallVulkanRT.exe"
        Delete /REBOOTOK "$INSTDIR\Vulkan.ico"
        Delete /REBOOTOK "$INSTDIR\ConfigLayersAndVulkanDLL.ps1"
        Delete /REBOOTOK "$INSTDIR\vulkaninfo.exe"

        # Need to do a SetOutPath to something outside of INSTDIR,
        # or the uninstall will think INSTDIR is busy
        SetOutPath "$TEMP"

        # Remove install directories
        Rmdir /REBOOTOK "$INSTDIR"
        StrCpy $0 "C:\Program Files (x86)\${PRODUCTNAME}"
        Call un.DeleteDirIfEmpty

        # If any of the remove commands failed, request a reboot
        IfRebootFlag 0 noreboot
             MessageBox MB_YESNO "A reboot is required to finish the uninstall. Do you wish to reboot now?" IDNO noreboot
        Reboot
        noreboot:
    ${EndIf}

SectionEnd

Function brandimage
  SetOutPath "$TEMP"
  SetFileAttributes VulkanLogo.bmp temporary
  File VulkanLogo.bmp
  SetBrandingImage "$TEMP/VulkanLogo.bmp"
Functionend


Function un.brandimage
  SetOutPath "$TEMP"
  SetFileAttributes VulkanLogo.bmp temporary
  File VulkanLogo.bmp
  SetBrandingImage "$TEMP/VulkanLogo.bmp"
Functionend

Function un.DeleteDirIfEmpty
  FindFirst $R0 $R1 "$0\*.*"
  strcmp $R1 "." 0 NoDelete
   FindNext $R0 $R1
   strcmp $R1 ".." 0 NoDelete
    ClearErrors
    FindNext $R0 $R1
    IfErrors 0 NoDelete
     FindClose $R0
     Sleep 1000
     RMDir "$0"
  NoDelete:
   FindClose $R0
FunctionEnd
