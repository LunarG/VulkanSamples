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
# Set VERSION_BUILDNO to:
#    x.pre.z for prereleases
#    x for releases
#
!define PRODUCTNAME "VulkanRT"
!define VERSION_ABI_MAJOR "1"
!define VERSION_API_MAJOR "1"
!define VERSION_MINOR "0"
!define VERSION_PATCH "1"
!define VERSION_BUILDNO "0.pre.1"
#!define VERSION_BUILDNO "0"
!define PRODUCTVERSION "${VERSION_API_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILDNO}"

# Includes
!include LogicLib.nsh

# This number is determined by doing an install, and then from Windows Explorer,
# doing a "Properties" on the install directory. Add to this the size of the
# files installed to C:\Windows\System32. And then add a little bit more.
# The units are 1K bytes.
!define ESTIMATEDSIZE "500"

# Set the icon
!define ICOFILE "Vulkan.ico"
Icon ${ICOFILE}
UninstallIcon ${ICOFILE}
WindowIcon off

# Define name of installer
OutFile "VulkanRT-${PRODUCTVERSION}-Installer.exe"

# Define default installation directory
InstallDir "$PROGRAMFILES\${PRODUCTNAME}\${PRODUCTVERSION}"

# Variable that holds version string used in file names
Var FileVersion


#############################################
# StrRep - string replace

!define StrRep "!insertmacro StrRep"
!macro StrRep output string old new
    Push `${string}`
    Push `${old}`
    Push `${new}`
    !ifdef __UNINSTALL__
        Call un.StrRep
    !else
        Call StrRep
    !endif
    Pop ${output}
!macroend

!macro Func_StrRep un
    Function ${un}StrRep
        Exch $R2 ;new
        Exch 1
        Exch $R1 ;old
        Exch 2
        Exch $R0 ;string
        Push $R3
        Push $R4
        Push $R5
        Push $R6
        Push $R7
        Push $R8
        Push $R9

        StrCpy $R3 0
        StrLen $R4 $R1
        StrLen $R6 $R0
        StrLen $R9 $R2
        loop:
            StrCpy $R5 $R0 $R4 $R3
            StrCmp $R5 $R1 found
            StrCmp $R3 $R6 done
            IntOp $R3 $R3 + 1 ;move offset by 1 to check the next character
            Goto loop
        found:
            StrCpy $R5 $R0 $R3
            IntOp $R8 $R3 + $R4
            StrCpy $R7 $R0 "" $R8
            StrCpy $R0 $R5$R2$R7
            StrLen $R6 $R0
            IntOp $R3 $R3 + $R9 ;move offset by length of the replacement string
            Goto loop
        done:

        Pop $R9
        Pop $R8
        Pop $R7
        Pop $R6
        Pop $R5
        Pop $R4
        Pop $R3
        Push $R0
        Push $R1
        Pop $R0
        Pop $R1
        Pop $R0
        Pop $R2
        Exch $R1
    FunctionEnd
!macroend
!insertmacro Func_StrRep ""
!insertmacro Func_StrRep "un."

#############################################
# x64 macros

!define IsWow64 `"" IsWow64 ""`
!macro _IsWow64 _a _b _t _f
  !insertmacro _LOGICLIB_TEMP
  System::Call kernel32::GetCurrentProcess()p.s
  System::Call kernel32::IsWow64Process(ps,*i0s)
  Pop $_LOGICLIB_TEMP
  !insertmacro _!= $_LOGICLIB_TEMP 0 `${_t}` `${_f}`
!macroend

!define RunningX64 `"" RunningX64 ""`
!macro _RunningX64 _a _b _t _f
  !if ${NSIS_PTR_SIZE} > 4
    !insertmacro LogicLib_JumpToBranch `${_t}` `${_f}`
  !else
    !insertmacro _IsWow64 `${_a}` `${_b}` `${_t}` `${_f}`
  !endif
!macroend

!define DisableX64FSRedirection "!insertmacro DisableX64FSRedirection"
!macro DisableX64FSRedirection
  System::Call kernel32::Wow64EnableWow64FsRedirection(i0)
!macroend

!define EnableX64FSRedirection "!insertmacro EnableX64FSRedirection"
!macro EnableX64FSRedirection
  System::Call kernel32::Wow64EnableWow64FsRedirection(i1)
!macroend


Function .onInit

    #
    # Nothing to do now, we used to fail on a 32-bit system here.
    # Remove if not needed in the future.
    #
    
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

    # If running on a 64-bit OS machine, disable registry re-direct since we're running as a 32-bit executable.
    ${If} ${RunningX64}
            
        ${DisableX64FSRedirection}
        SetRegView 64

    ${Endif}

    # Set up version number for file names
    ${StrRep} $0 ${VERSION_BUILDNO} "." "-"
    StrCpy $FileVersion ${VERSION_ABI_MAJOR}-${VERSION_API_MAJOR}-${VERSION_MINOR}-${VERSION_PATCH}-$0


    # If running on a 64-bit OS machine
    ${If} ${RunningX64}
            
        # 32-bit DLLs/EXEs destined for SysWOW64
        ##########################################
        SetOutPath $WINDIR\SysWow64
        File /oname=vulkan-$FileVersion.dll ..\build32\loader\Release\vulkan-${VERSION_ABI_MAJOR}.dll
        File /oname=vulkaninfo-$FileVersion.exe ..\build32\demos\Release\vulkaninfo.exe
        
        # 64-bit DLLs/EXEs
        ##########################################
        SetOutPath $WINDIR\System32
        File /oname=vulkan-$FileVersion.dll ..\build\loader\Release\vulkan-${VERSION_ABI_MAJOR}.dll

        # vulkaninfo.exe
        File /oname=vulkaninfo-$FileVersion.exe ..\build\demos\Release\vulkaninfo.exe
        SetOutPath "$INSTDIR"
        File ..\build\demos\Release\vulkaninfo.exe
        File /oname=vulkaninfo32.exe ..\build32\demos\Release\vulkaninfo.exe
        SetShellVarContext all
        CreateDirectory "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}"
        CreateDirectory "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos"
        CreateShortCut "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos\vulkaninfo.lnk" "$INSTDIR\vulkaninfo.exe"
        CreateShortCut "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos\vulkaninfo32.lnk" "$INSTDIR\vulkaninfo32.exe"

        SetOutPath "$INSTDIR"
        File ${ICOFILE}
        File LICENSE.rtf
        File ConfigLayersAndVulkanDLL.ps1

        # Run the ConfigLayersAndVulkanDLL.ps1 script to copy the most recent version of
        # vulkan-<abimajor>-*.dll to vulkan-<abimajor>.dll, and to set up layer registry
        # entries to use layers from the corresponding SDK
        nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File ConfigLayersAndVulkanDLL.ps1 ${VERSION_ABI_MAJOR} 64'

        # We are done using ConfigLayersAndVulkanDLL.ps1, delete it. It will be re-installed
        # by the uninstaller when it needs to be run again during uninstall.
        Delete ConfigLayersAndVulkanDLL.ps1

    # Else, running on a 32-bit OS machine
    ${Else}
 
        # 32-bit DLLs/EXEs destined for SysWOW64
        ##########################################
        SetOutPath $WINDIR\System32
        File /oname=vulkan-$FileVersion.dll ..\build32\loader\Release\vulkan-${VERSION_ABI_MAJOR}.dll

        # vulkaninfo.exe
        File /oname=vulkaninfo-$FileVersion.exe ..\build32\demos\Release\vulkaninfo.exe
        SetOutPath "$INSTDIR"
        File ..\build32\demos\Release\vulkaninfo.exe
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
        nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File ConfigLayersAndVulkanDLL.ps1 ${VERSION_ABI_MAJOR} 32'

        # We are done using ConfigLayersAndVulkanDLL.ps1, delete it. It will be re-installed
        # by the uninstaller when it needs to be run again during uninstall.
        Delete ConfigLayersAndVulkanDLL.ps1

    ${Endif}

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
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "VersionMinor" ${VERSION_PATCH}.${VERSION_BUILDNO}
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "EstimatedSize" ${ESTIMATEDSIZE}
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "DisplayIcon" "$\"$INSTDIR\${ICOFILE}$\""

    # Possibly install MSVC 2013 redistributables
    ReadRegDword $1 HKLM "SOFTWARE\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum" "Install"
    IntCmp $1 1 RedistributablesInstalled InstallRedistributables InstallRedistributables
    InstallRedistributables:
       SetOutPath "$TEMP"
        # If running on a 64-bit OS machine, we need to install the 64-bit Visual Studio re-distributable
        ${If} ${RunningX64}
        
           File vcredist_x64.exe
           ExecWait '"$TEMP\vcredist_x64.exe"  /passive /norestart'

        # Otherwise, we're running on a 32-bit OS machine, we need to install the 32-bit Visual Studio re-distributable
        ${Else}
           File vcredist_x86.exe
           ExecWait '"$TEMP\vcredist_x86.exe"  /passive /norestart'
        ${Endif}   
     RedistributablesInstalled:

SectionEnd

# Uninstaller section start
Section "uninstall"

    # If running on a 64-bit OS machine, disable registry re-direct since we're running as a 32-bit executable.
    ${If} ${RunningX64}

        ${DisableX64FSRedirection}
        SetRegView 64
        
    ${Endif}

    SetOutPath "$INSTDIR"

    # Set up version number for file names
    ${StrRep} $0 ${VERSION_BUILDNO} "." "-"
    StrCpy $FileVersion ${VERSION_ABI_MAJOR}-${VERSION_API_MAJOR}-${VERSION_MINOR}-${VERSION_PATCH}-$0

    # Decrement the number of times we have been installed.
    ReadRegDword $1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "IC"
    IntOp $1 $1 - 1
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "IC" $1

    # Ref count is in $1. If it is zero, uninstall.
    ${If} $1 <= 0

        # Install the ConfigLayersAndVulkanDLL.ps1 so we can run it.
        # It will be deleted later when we remove the install directory.
        File ConfigLayersAndVulkanDLL.ps1

        # If running on a 64-bit OS machine
        ${If} ${RunningX64}
        
            # Delete vulkaninfo.exe in C:\Windows\System32 and C:\Windows\SysWOW64
            Delete /REBOOTOK $WINDIR\SysWow64\vulkaninfo.exe
            Delete /REBOOTOK "$WINDIR\SysWow64\vulkaninfo-$FileVersion.exe"
            Delete /REBOOTOK $WINDIR\System32\vulkaninfo.exe
            Delete /REBOOTOK "$WINDIR\System32\vulkaninfo-$FileVersion.exe"

            # Delete vullkan dll files: vulkan-<majorabi>.dll and vulkan-<majorabi>-<major>-<minor>-<patch>-<buildno>.dll
            Delete /REBOOTOK $WINDIR\SysWow64\vulkan-${VERSION_ABI_MAJOR}.dll
            Delete /REBOOTOK $WINDIR\SysWow64\vulkan-$FileVersion.dll
            Delete /REBOOTOK $WINDIR\System32\vulkan-${VERSION_ABI_MAJOR}.dll
            Delete /REBOOTOK $WINDIR\System32\vulkan-$FileVersion.dll

            # Run the ConfigLayersAndVulkanDLL.ps1 script to:
            #   Copy the most recent version of vulkan-<abimajor>-*.dll to vulkan-<abimajor>.dll
            #   Copy the most recent version of vulkaninfo-<abimajor>-*.exe to vulkaninfo.exe
            #   Set up layer registry entries to use layers from the corresponding SDK
            nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File "$INSTDIR\ConfigLayersAndVulkanDLL.ps1" ${VERSION_ABI_MAJOR} 64'

        # Else, running on a 32-bit OS machine
        ${Else}
  
            # Delete vulkaninfo.exe in C:\Windows\System32
            Delete /REBOOTOK $WINDIR\System32\vulkaninfo.exe
            Delete /REBOOTOK "$WINDIR\System32\vulkaninfo-$FileVersion.exe"

            # Delete vullkan dll files: vulkan-<majorabi>.dll and vulkan-<majorabi>-<major>-<minor>-<patch>-<buildno>.dll
            Delete /REBOOTOK $WINDIR\System32\vulkan-${VERSION_ABI_MAJOR}.dll
            Delete /REBOOTOK $WINDIR\System32\vulkan-$FileVersion.dll

            # Run the ConfigLayersAndVulkanDLL.ps1 script to:
            #   Copy the most recent version of vulkan-<abimajor>-*.dll to vulkan-<abimajor>.dll
            #   Copy the most recent version of vulkaninfo-<abimajor>-*.exe to vulkaninfo.exe
            #   Set up layer registry entries to use layers from the corresponding SDK
            nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File "$INSTDIR\ConfigLayersAndVulkanDLL.ps1" ${VERSION_ABI_MAJOR} 32'

        ${EndIf}
  
        # Delete vulkaninfo from start menu.
        # Delete vulkan start menu if the vulkan start menu is empty
        SetShellVarContext all
        Delete "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos\vulkaninfo.lnk"

        # If running on a 64-bit OS machine
        ${If} ${RunningX64}
            Delete "$SMPROGRAMS\Vulkan ${PRODUCTVERSION}\Demos\vulkaninfo32.lnk"
        ${EndIf}
         
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
        StrCpy $0 "$PROGRAMFILES\${PRODUCTNAME}"
        Call un.DeleteDirIfEmpty

        # If any of the remove commands failed, request a reboot
        IfRebootFlag 0 noreboot
             MessageBox MB_YESNO "A reboot is required to finish the uninstall. Do you wish to reboot now?" IDNO noreboot
        Reboot
        noreboot:
        
    ${Endif}

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
