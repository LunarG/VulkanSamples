# NSIS Script for creating the Windows Vulkan RT installer.
#
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
!define PUBLISHER "YourCompany, Inc."
#!define VERSION_BUILDNO "0"
!define PRODUCTVERSION "${VERSION_API_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILDNO}"

# Includes
!include LogicLib.nsh

# This number is determined by doing an install, and then from Windows Explorer,
# doing a "Properties" on the install directory. Add to this the size of the
# files installed to C:\Windows\System32. And then add a little bit more.
# The units are 1K bytes.
!define ESTIMATEDSIZE "1700"

# This is used for the error message if a problem occurs during install.
!define errorMessage1 "Installation of ${PRODUCTNAME} failed!$\r$\n"
!define errorMessage1un "Uninstall of ${PRODUCTNAME} failed!$\r$\n"
!define errorMessage2 "Uninstalling any installed items and exiting.$\r$\n"

# Set the icon
!define ICOFILE "V.ico"
Icon ${ICOFILE}
UninstallIcon ${ICOFILE}
WindowIcon off

# If /DUNINSTALLER was specified, Create the uinstaller
!ifdef UNINSTALLER
  !echo "Creating RT uninstaller...."
  OutFile "$%TEMP%\tempinstaller.exe"
  SetCompress off
!else
  !echo "Creating RT installer...."

  # Define name of installer
  OutFile "VulkanRT-${PRODUCTVERSION}-Installer.exe"
  SetCompressor /SOLID lzma

!endif

# Define default installation directory
InstallDir "$PROGRAMFILES\${PRODUCTNAME}\${PRODUCTVERSION}"

# Version string used in file names
Var FileVersion

# Directory RT was installed to.
# The uninstaller can't just use $INSTDIR because it is set to the
# directory the uninstaller exe file is located in.
!ifdef UNINSTALLER
Var IDir
!endif

# Install count
Var IC

# Error code from powershell script
Var PsErr


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


# Need admin to write to C:\Windows\System32 and install dir
RequestExecutionLevel admin

Function .onInit

!ifdef UNINSTALLER
   ; Write out the uinstaller and quit
   WriteUninstaller "$%TEMP%\Uninstall${PRODUCTNAME}.exe"
   Quit
!endif

FunctionEnd

AddBrandingImage left 150
Caption "${PRODUCTNAME} ${PRODUCTVERSION} Setup"
Name "${PRODUCTNAME} ${PRODUCTVERSION}"
LIcenseData "VULKANRT_LICENSE.rtf"
Page custom brandimage "" ": Brand Image"
Page license
Page directory
Page instfiles
UninstallCaption "\${PRODUCTNAME} ${PRODUCTVERSION} Uninstall"
UninstallText "This wizard will uninstall ${PRODUCTNAME} ${PRODUCTVERSION} from your computer. Click Uninstall to start the uninstallation."
UninstPage custom un.brandimage "" ": Brand Image"
UninstPage uninstConfirm
UninstPage instFiles

# File Properties
VIProductVersion "${PRODUCTVERSION}"
VIAddVersionKey  "ProductName" "Vulkan Runtime"
VIAddVersionKey  "FileVersion" "${PRODUCTVERSION}"
VIAddVersionKey  "ProductVersion" "${PRODUCTVERSION}"
VIAddVersionKey  "FileDescription" "Vulkan Runtime Installer"
VIAddVersionKey  "LegalCopyright" ""

# Start default section
Section

    # If running on a 64-bit OS machine, disable registry re-direct since we're running as a 32-bit executable.
    ${If} ${RunningX64}

        ${DisableX64FSRedirection}
        SetRegView 64

    ${Endif}

    # Create our temp directory, with minimal permissions
    SetOutPath "$TEMP\VulkanRT"
    AccessControl::DisableFileInheritance $TEMP\VulkanRT
    AccessControl::SetFileOwner $TEMP\VulkanRT "Administrators"
    AccessControl::ClearOnFile  $TEMP\VulkanRT "Administrators" "FullAccess"
    AccessControl::SetOnFile    $TEMP\VulkanRT "SYSTEM" "FullAccess"
    AccessControl::GrantOnFile  $TEMP\VulkanRT "Everyone" "ListDirectory"
    AccessControl::GrantOnFile  $TEMP\VulkanRT "Everyone" "GenericExecute"
    AccessControl::GrantOnFile  $TEMP\VulkanRT "Everyone" "GenericRead"
    AccessControl::GrantOnFile  $TEMP\VulkanRT "Everyone" "ReadAttributes"
    StrCpy $1 10
    Call CheckForError

    # Check the registry to see if we are already installed
    ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "InstallDir"

    # If the registry entry isn't there, it will throw an error as well as return a blank value.  So, clear the errors.
    ${If} ${Errors}

        # Nothing else to do since there is no previous install
        ClearErrors

    ${Else}

        # Use the previous install directory, so we don't have to keep tracking every possible runtime install.
        strcmp $INSTDIR $0 notinstalled

        ${If} $0 != ""
            MessageBox MB_OK "The Windows Vulkan Runtime is already installed to $0. It will be re-installed to the same folder." /SD IDOK
            Strcpy $INSTDIR $0
        ${Endif}

        notinstalled:

    ${EndIf}

    SetOutPath "$INSTDIR"
    AccessControl::DisableFileInheritance $INSTDIR
    AccessControl::SetFileOwner $INSTDIR "Administrators"
    AccessControl::ClearOnFile  $INSTDIR "Administrators" "FullAccess"
    AccessControl::SetOnFile    $INSTDIR "SYSTEM" "FullAccess"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "ListDirectory"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "GenericExecute"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "GenericRead"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "ReadAttributes"
    File ${ICOFILE}
    File VULKANRT_LICENSE.RTF
    File LICENSE.txt
    File ConfigLayersAndVulkanDLL.ps1
    StrCpy $1 15
    Call CheckForError

    # Add the signed uninstaller
    !ifndef UNINSTALLER
        SetOutPath $INSTDIR
        File "Uninstall${PRODUCTNAME}.exe"
    !endif

    StrCpy $1 20
    Call CheckForError

    # Reference count the number of times we have been installed.
    # The reference count is stored in the registry value InstallCount
    ReadRegDword $1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "InstallCount"
    IntOp $1 $1 + 1
    StrCpy $IC $1

    # We need to create a new folder for each install. Since we are using counted installs,
    # an uninstall when the count is greater than one would result in the install
    # count being decremented and nothing being removed. But Windows Add/Remove Programs
    # generates a warning Window if the install dir for a package that is removed is not
    # deleted. So we create a unique folder for each counted install.
    # We fudge it a little and only create one folder, and rename it after each
    # install/uninstall.

    # Create the install instance folder. We rename the install instance folder if it already exists.
    # Then copy the uninstaller to it.
    ${If} $IC > 2
        IntOp $1 $IC - 1
        Rename "$INSTDIR\Instance_$1" "$INSTDIR\Instance_$IC"
        CopyFiles /SILENT "$INSTDIR\Uninstall${PRODUCTNAME}.exe" "$INSTDIR\Instance_$IC"
    ${ElseIf} $IC = 2
        CreateDirectory "$INSTDIR\Instance_$IC"
        CopyFiles /SILENT "$INSTDIR\Uninstall${PRODUCTNAME}.exe" "$INSTDIR\Instance_$IC"
    ${EndIf}


    # If the registry entry isn't there, it will throw an error as well as return a blank value.  So, clear the errors.
    ${If} ${Errors}
        ClearErrors
    ${EndIf}

    # Modify registry for Programs and Features
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "DisplayName" "Vulkan Run Time Libraries ${PRODUCTVERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "UninstallString" "$INSTDIR\Uninstall${PRODUCTNAME}.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "Publisher" "${PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "DisplayVersion" "${PRODUCTVERSION}"
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "EstimatedSize" ${ESTIMATEDSIZE}
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "DisplayIcon" "$\"$INSTDIR\${ICOFILE}$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "InstallDir" "$INSTDIR"
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "InstallCount" $IC

    ${If} $IC > 1
        WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "DisplayName" "Vulkan Run Time Libraries ${PRODUCTVERSION}"
        WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "UninstallString" "$INSTDIR\Instance_$IC\Uninstall${PRODUCTNAME}.exe"
        WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "Publisher" "${PUBLISHER}"
        WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "DisplayVersion" "${PRODUCTVERSION}"
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "EstimatedSize" ${ESTIMATEDSIZE}
        WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "DisplayIcon" "$\"$INSTDIR\${ICOFILE}$\""
        WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "InstallDir" "$INSTDIR\Instance_$IC"
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "InstallCount" $IC
    ${EndIf}

    # Set SystemComponent to 1 for those instances that are not to be visible to Add/Remove Programs.
    # Set SystemComponent to 0 for the instance that is to be visible to Add/Remove Programs.
    ${If} $IC > 2
        IntOp $1 $IC - 1
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "SystemComponent" 0
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$1" "SystemComponent" 1
    ${ElseIf} $IC = 2
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "SystemComponent" 0
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "SystemComponent" 1
    ${Else}
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "SystemComponent" 0
    ${EndIf}

    StrCpy $1 25
    Call CheckForError

    # Set up version number for file names
    ${StrRep} $0 ${VERSION_BUILDNO} "." "-"
    StrCpy $FileVersion ${VERSION_ABI_MAJOR}-${VERSION_API_MAJOR}-${VERSION_MINOR}-${VERSION_PATCH}-$0

    # Remove vulkaninfo from Start Menu
    SetShellVarContext all
    Delete "$SMPROGRAMS\Vulkan\vulkaninfo32.lnk"
    Delete "$SMPROGRAMS\Vulkan\vulkaninfo.lnk"
    ClearErrors

    # Create Vulkan in the Start Menu
    CreateDirectory "$SMPROGRAMS\Vulkan"
    ClearErrors

    # If running on a 64-bit OS machine
    ${If} ${RunningX64}

        # 32-bit DLLs/EXEs destined for SysWOW64
        ##########################################
        SetOutPath $WINDIR\SysWow64
        File /oname=vulkan-$FileVersion.dll ..\build32\loader\Release\vulkan-${VERSION_ABI_MAJOR}.dll
        File /oname=vulkaninfo-$FileVersion.exe ..\build32\demos\Release\vulkaninfo.exe
        StrCpy $1 30
        Call CheckForError

        # 64-bit DLLs/EXEs
        ##########################################
        SetOutPath $WINDIR\System32
        File /oname=vulkan-$FileVersion.dll ..\build\loader\Release\vulkan-${VERSION_ABI_MAJOR}.dll
        StrCpy $1 35
        Call CheckForError

        # vulkaninfo.exe
        File /oname=vulkaninfo-$FileVersion.exe ..\build\demos\Release\vulkaninfo.exe
        SetOutPath "$INSTDIR"
        File ..\build\demos\Release\vulkaninfo.exe
        File /oname=vulkaninfo32.exe ..\build32\demos\Release\vulkaninfo.exe
        StrCpy $1 40
        Call CheckForError

        # Run the ConfigLayersAndVulkanDLL.ps1 script to copy the most recent version of
        # vulkan-<abimajor>-*.dll to vulkan-<abimajor>.dll, and to set up layer registry
        # entries to use layers from the corresponding SDK
        nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File ConfigLayersAndVulkanDLL.ps1 ${VERSION_ABI_MAJOR} 64'
        pop $PsErr
        ${If} $PsErr != 0
            SetErrors
        ${EndIf}
        StrCpy $1 45
        Call CheckForError

    # Else, running on a 32-bit OS machine
    ${Else}

        # 32-bit DLLs/EXEs destined for SysWOW64
        ##########################################
        SetOutPath $WINDIR\System32
        File /oname=vulkan-$FileVersion.dll ..\build32\loader\Release\vulkan-${VERSION_ABI_MAJOR}.dll
        StrCpy $1 50
        Call CheckForError

        # vulkaninfo.exe
        File /oname=vulkaninfo-$FileVersion.exe ..\build32\demos\Release\vulkaninfo.exe
        SetOutPath "$INSTDIR"
        File ..\build32\demos\Release\vulkaninfo.exe
        StrCpy $1 55
        Call CheckForError

        # Run the ConfigLayersAndVulkanDLL.ps1 script to copy the most recent version of
        # vulkan-<abimajor>-*.dll to vulkan-<abimajor>.dll, and to set up layer registry
        # entries to use layers from the corresponding SDK
        nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File ConfigLayersAndVulkanDLL.ps1 ${VERSION_ABI_MAJOR} 32'
        pop $PsErr
        ${If} $PsErr != 0
            SetErrors
        ${EndIf}
        StrCpy $1 60
        Call CheckForError

    ${Endif}

    # We are done using ConfigLayersAndVulkanDLL.ps1, delete it. It will be re-installed
    # by the uninstaller when it needs to be run again during uninstall.
    Delete ConfigLayersAndVulkanDLL.ps1

    # Add vulkaninfo to Start Menu
    SetShellVarContext all
    IfFileExists $WINDIR\System32\vulkaninfo.exe 0 +2
        CreateShortCut "$SMPROGRAMS\Vulkan\vulkaninfo.lnk" "$WINDIR\System32\vulkaninfo.exe"
    IfFileExists $WINDIR\SysWow64\vulkaninfo.exe 0 +2
        CreateShortCut "$SMPROGRAMS\Vulkan\vulkaninfo32.lnk" "$WINDIR\SysWow64\vulkaninfo.exe"

    # Possibly install MSVC 2013 redistributables
    ClearErrors
    ${If} ${RunningX64}
        ReadRegDword $1 HKLM "SOFTWARE\WOW6432Node\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum" "Install"
        ${If} ${Errors}
           StrCpy $1 0
           ClearErrors
        ${Endif}
    ${Else}
       StrCpy $1 1
    ${Endif}
    ReadRegDword $2 HKLM "SOFTWARE\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum" "Install"
    ${If} ${Errors}
       StrCpy $2 0
       ClearErrors
    ${Endif}
    IntOp $3 $1 + $2
    ${If} $3 <= 1
        # If either x86 or x64 redistributables are not present, install redistributables.
        # We install both resdistributables because we have found that the x86 redist
        # will uninstall the x64 redist if the x64 redistrib is an old version. Amazing, isn't it?
        SetOutPath "$TEMP\VulkanRT"
        ${If} ${RunningX64}
            File vcredist_x64.exe
            ExecWait '"$TEMP\VulkanRT\vcredist_x64.exe"  /quiet /norestart'
        ${Endif}
        File vcredist_x86.exe
        ExecWait '"$TEMP\VulkanRT\vcredist_x86.exe"  /quiet /norestart'
    ${Endif}
    StrCpy $1 65
    Call CheckForError

SectionEnd

# Uninstaller section start
!ifdef UNINSTALLER
Section "uninstall"

    # If running on a 64-bit OS machine, disable registry re-direct since we're running as a 32-bit executable.
    ${If} ${RunningX64}

        ${DisableX64FSRedirection}
        SetRegView 64

    ${Endif}

    # Look up the install dir and remove files from that directory.
    # We do this so that the uninstaller can be run from any directory.
    ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "InstallDir"
    StrCpy $IDir $0

    StrCpy $1 70
    Call un.CheckForError

    SetOutPath "$IDir"

    # Set up version number for file names
    ${StrRep} $0 ${VERSION_BUILDNO} "." "-"
    StrCpy $FileVersion ${VERSION_ABI_MAJOR}-${VERSION_API_MAJOR}-${VERSION_MINOR}-${VERSION_PATCH}-$0

    # Decrement the number of times we have been installed.
    ReadRegDword $IC HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "InstallCount"
    IntOp $1 $IC - 1
    WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "InstallCount" $1

    # Rename the install dir for this instance if is not the last uninstall
    ${If} $IC > 2
        IntOp $1 $IC - 1
        Rename "$IDir\Instance_$IC" "$IDir\Instance_$1"
    ${ElseIf} $IC = 2
        Delete /REBOOTOK "$IDir\Instance_$IC\Uninstall${PRODUCTNAME}.exe"
        Rmdir /REBOOTOK "$IDir\Instance_$IC"
    ${Endif}

    # Modify registry for Programs and Features

    ${If} $IC > 1
        DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC"
    ${EndIf}
    ${If} $IC > 2
        IntOp $IC $IC - 1
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}-$IC" "SystemComponent" 0
    ${ElseIf} $IC = 2
        WriteRegDword HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}" "SystemComponent" 0
    ${Else}
        # Last uninstall
        IntOp $IC $IC - 1
        DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCTNAME}${PRODUCTVERSION}"
    ${EndIf}


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
        nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File "$IDir\ConfigLayersAndVulkanDLL.ps1" ${VERSION_ABI_MAJOR} 64'

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
        nsExec::ExecToStack 'powershell -NoLogo -NonInteractive -WindowStyle Hidden -inputformat none -ExecutionPolicy RemoteSigned -File "$IDir\ConfigLayersAndVulkanDLL.ps1" ${VERSION_ABI_MAJOR} 32'

    ${EndIf}

    # If Ref Count is zero, uninstall everything
    ${If} $IC <= 0

        # Delete vulkaninfo from start menu.
        SetShellVarContext all
        Delete "$SMPROGRAMS\Vulkan\vulkaninfo.lnk"

        # If running on a 64-bit OS machine
        ${If} ${RunningX64}
            Delete "$SMPROGRAMS\Vulkan\vulkaninfo32.lnk"
        ${EndIf}

        # Possibly add vulkaninfo to Start Menu
        SetShellVarContext all
        IfFileExists $WINDIR\System32\vulkaninfo.exe 0 +2
            CreateShortCut "$SMPROGRAMS\Vulkan\vulkaninfo.lnk" "$WINDIR\System32\vulkaninfo.exe"
        IfFileExists $WINDIR\SysWow64\vulkaninfo.exe 0 +2
            CreateShortCut "$SMPROGRAMS\Vulkan\vulkaninfo32.lnk" "$WINDIR\SysWow64\vulkaninfo.exe"

        # Possibly delete vulkan Start Menu
        StrCpy $0 "$SMPROGRAMS\Vulkan"
        Call un.DeleteDirIfEmpty
        ClearErrors

        # Remove files in install dir
        Delete /REBOOTOK "$IDir\VULKANRT_LICENSE.rtf"
        Delete /REBOOTOK "$IDir\LICENSE.txt"
        Delete /REBOOTOK "$IDir\Uninstall${PRODUCTNAME}.exe"
        Delete /REBOOTOK "$IDir\V.ico"
        Delete /REBOOTOK "$IDir\ConfigLayersAndVulkanDLL.ps1"
        Delete /REBOOTOK "$IDir\vulkaninfo.exe"

        # If running on a 64-bit OS machine
        ${If} ${RunningX64}
            Delete /REBOOTOK "$IDir\vulkaninfo32.exe"
        ${EndIf}

        StrCpy $1 75
        Call un.CheckForError

        # Need to do a SetOutPath to something outside of install dir,
        # or the uninstall will think install dir is busy
        SetOutPath "$TEMP"

        # Remove install directories
        StrCpy $0 "$IDir"
        Call un.DeleteDirIfEmpty
        StrCpy $0 "$PROGRAMFILES\${PRODUCTNAME}"
        Call un.DeleteDirIfEmpty
        ClearErrors

        # If any of the remove commands failed, request a reboot
        IfRebootFlag 0 noreboot
            MessageBox MB_YESNO "A reboot is required to finish the uninstall. Do you wish to reboot now?" /SD IDNO IDNO returnerror
            Reboot

            returnerror:

            # Set an error message to output because we should reboot but didn't (whether because silent uninstall or user choice)
            SetErrorLevel 3 # ERROR_TOO_MANY_OPEN_FILES

        noreboot:

    ${Endif}

    StrCpy $1 80
    Call un.CheckForError

    # Remove temp dir
    SetOutPath "$TEMP"
    RmDir /R "$TEMP\VulkanRT"

SectionEnd
!endif

Function brandimage
  SetOutPath "$TEMP"
  SetFileAttributes V.bmp temporary
  File V.bmp
  SetBrandingImage "$TEMP/V.bmp"
Functionend


Function un.brandimage
  SetOutPath "$TEMP"
  SetFileAttributes V.bmp temporary
  File V.bmp
  SetBrandingImage "$TEMP/V.bmp"
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

# Check for errors during install.  If we hit an error, stop, uninstall what we've put in so far, and quit.
# NOTE: We return a non-zero error code as well.
Function CheckForError
    ${If} ${Errors}
        # IHV's using this install may want no message box.
        MessageBox MB_OK|MB_ICONSTOP "${errorMessage1}${errorMessage2}Errorcode: $1$\r$\n" /SD IDOK

        # Copy the uninstaller to a temp folder of our own creation so we can completely
        # delete the old contents.
        SetOutPath "$TEMP\VulkanRT"
        CopyFiles "$INSTDIR\Uninstall${PRODUCTNAME}.exe" "$TEMP\VulkanRT"

        # No uninstall using the version in the temporary folder.
        ExecWait '"$TEMP\VulkanRT\Uninstall${PRODUCTNAME}.exe" /S _?=$INSTDIR'

        # Delete the copy of the uninstaller we ran
        Delete /REBOOTOK "$TEMP\VulkanRT\Uninstall${PRODUCTNAME}.exe"
        RmDir /R /REBOOTOK "$TEMP\VulkanRT"

        # Set an error message to output
        SetErrorLevel $1

        Quit
    ${EndIf}
FunctionEnd

# Check for errors during uninstall.  If we hit an error, don't attempt
# to do anything. Just set a non-zero return code and quit.
Function un.CheckForError
    ${If} ${Errors}
        # IHV's using this install may want no message box.
        MessageBox MB_OK|MB_ICONSTOP "${errorMessage1un}${errorMessage2}Errorcode: $1$\r$\n" /SD IDOK

        # Set an error message to output
        SetErrorLevel $1

        Quit
    ${EndIf}
FunctionEnd
