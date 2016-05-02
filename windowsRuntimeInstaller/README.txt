This folder contains the files required for building the Windows Vulkan
Runtime Installer Package.

To build the Vulkan Runtime Installer:

   1.  Install Nullsoft Install System (NSIS) version 3.0b3. The
       version of NSIS needed for building the Vulkan Runtime Installer
       must support logging (i.e. must have been built with NSIS_CONFIG_LOG=yes
       set), and must support long strings (i.e. NSIS_MAX_STRLEN=8192 must be
       set). The NSIS AccessControl plug-in should also be installed. (Available
       from http://nsis.sourceforge.net/AccessControl_plug-in.)

   1a. Note that the NSIS binary version available at
       http://nsis.sourceforge.net/Download is not built with
       NSIS_CONFIG_LOG=yes and NSIS_MAX_STRLEN=8192 set. Also, changes to need
       to be made to NSIS to increase the security of the Runtime Installer.

       The source for NSIS 3.0.b3 can be downloaded from
       https://sourceforge.net/projects/nsis/files/NSIS%203%20Pre-release/3.0b3/nsis-3.0b3-src.tar.bz2/download

       Instructions for building NSIS are available at
       http://nsis//sourceforge.net/Docs/AppendixG.html.

       The security changes to NSIS involve adding the /DYMANICBASE and /GS options
       to the NSIS compile/link steps, so that the Runtime Installer and Uninstaller
       are built with address space layout randomization and buffer overrun checks.

       The security changes to NSIS can be made by applying the patch in the
       NSIS_Security.patch file in this folder.

       After you have applied the security patch, build NSIS with this command:

           scons SKIPUTILS="NSIS Menu","MakeLangId" UNICODE=yes \
                 ZLIB_W32=<path_to_zlib>\zlib-1.2.7-win32-x86 NSIS_MAX_STRLEN=8192 \
                 NSIS_CONFIG_LOG=yes NSIS_CONFIG_LOG_TIMESTAMP=yes \
                 APPEND_CCFLAGS="/DYNAMICBASE /Zi" APPEND_LINKFLAGS="/DYNAMICBASE \
                 /DEBUG /OPT:REF /OPT:ICF" SKIPDOC=all dist-zip

       This will create a zip file in the nsis-3.0.b3-src directory.  Unpack
       the zip file anywhere on your system. The resulting tree will contain a
       Plugins directory. Install the NSIS AccessControl plugin in this directory.
       Add the Bin directory to your PATH enviroment variable so that the
       CreateInstaller.sh step below will use your custom-built version of
       NSIS.

       Before using NSIS and creating the installer, make sure that all shared
       libraries in your custom-built version of NSIS have the DYNAMIC_BASE and NX_COMPAT
       flags set. If they are not set, you will have to rebuild those libraries with
       those options enabled.

   2.  Build Vulkan-LoaderAndValidationLayers as described in ../BUILD.md.

   3.  Edit the InstallerRT.nsi file in this folder and modify the following
       lines to match the version of the Windows Vulkan Runtime you wish to
       build:

          !define VERSION_ABI_MAJOR
          !define VERSION_API_MAJOR
          !define VERSION_MINOR
          !define VERSION_PATCH
          !define VERSION_BUILDNO
          !define PUBLISHER

   4.  Edit the CreateInstaller.sh file and replace SIGNFILE with your
       command and necessary args for signing an executable. If you don't
       wish to sign the uninstaller, you can comment out that line.

   5.  Run the CreateInstaller.sh script from a Cygwin bash command prompt.
       The Cygwin bash shell must be running as Administrator.  The Windows
       Vulkan Runtime Installer will be created in this folder.  The name
       of the installer file is VulkanRT-<version>-Installer.exe.


Some notes on the behavior of the Windows Vulkan Runtime Installer:

   o  When VulkanRT-<version>-Installer.exe is run on a 64-bit version
      of Windows, it will install both the 32 and 64 bit versions of
      the Vulkan runtime.  When it is run on a 32-bit version of
      Windows, it will install the 32 bit version of the Vulkan runtime.

   o  The VulkanRT-<version>-Installer.exe created with the above steps
      can be run in silent mode by using the /S command line option when
      invoking the installer.

   o  If the Vulkan Runtime is already installed on the system,
      subsequent installs of the same version of the Vulkan Runtime
      will be installed to the same folder to which it is was
      previously installed.

   o  The Vulkan Runtime Installer uses "counted" installs. If the
      same version of the Vulkan Runtime is installed multiple times
      on a system, the Vulkan Runtime must be uninstalled the same
      number of times before it is completely removed from the system.

   o  If the Vulkan Runtime is already installed on a system and a
      different version is subsequently installed, both versions will
      then co-exist on the system. The Vulkan Runtime Installer does
      not remove prior versions of the Vulkan Runtime when a newer
      version is installed.

   o  The Vulkan Runtime can be uninstalled from Control Panel ->
      Programs and Features. It can also be uninstalled by running
      UninstallVulkanRT.exe in the install folder. The uninstall can
      be run in silent mode by using the /S command line option
      when invoking the uninstaller. The location of the the
      UninstallVulkanRT.exe can be found in the registry value
      HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\
      VulkanRT<version>\UninstallString

   o  The Vulkan Runtime Installer installs the Vulkan loader as
      C:\Windows\System32\vulkan-<version>.dll. It then runs the
      Powershell script ConfigLayersAndVulkanDLL.ps1, that compares
      versions of the loader in C:\Windows\System32 that have the
      same VERSION_ABI_MAJOR as the version being installed. The
      script selects the most recent one of these loader files and
      copies it to C:\Windows\System32\vulkan-<VERSION_ABI_MAJOR>.dll.
      For example, during the install of Vulkan Runtime version 2.0.1.1,
      the following files might be present in C:\Windows\System32:

           vulkan-1-1-0-2-3.dll
           vulkan-1-2-0-1-0.dll
           vulkan-1-2-0-1-1.dll

     [Note that the first "1" in the above files is VERSION_ABI_MAJOR.
     The other numbers identify the release version.] The script
     will copy the most recent one of these files (in this case
     vulkan-1-2-0-1-1.dll) to vulkan-1.dll. This is repeated for
     C:\Windows\SYSWOW64 on 64-bit Windows systems to set up the
     32-bit loader.

   o The Vulkan Runtime Uninstaller returns an exit code of 0-9
     to indicate success. An exit code of 3 indicates success, but
     a reboot is required to complete the uninstall.  All other
     exit codes indicate failure.  If the Uninstaller returns a
     failure exit code, it will have simply exited when the failure
     was detected and will not have attempted to do further uninstall
     work.

   o The ProductVersion of the installer executable (right click on
     the executable, Properties, then the Details tab) can be used
     by other installers/uninstallers to find the path to the
     uninstaller for the Vulkan Runtime in the Windows registry.
     This ProductVersion should always be identical to <version> in:

       HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\VulkanRT<version>\UninstallString

   o The Installer and Uninstaller create log files, which can be
     found in the VulkanrRT folder in the current TEMP folder.
     (The TEMP folder is generally identified by the TEMP environment
     variable). In addition to installer/uninstaller logs files,
     the folder also contains a log from the last run of the
     ConfigLayersAndVulkanDLL.ps1 script.
