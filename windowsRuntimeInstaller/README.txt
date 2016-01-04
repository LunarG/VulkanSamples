This folder contains the files required for building the Windows Vulkan Runtime
Installer Package.

To build the Installer:

   1. Install Nullsoft Install System version 3.0b1 or greater. (Available from
      http://nsis.sourceforge.net/Download.)

   2. Build Vulkan LoaderAndTools as described in ../BUILD.md.

   3. Edit the InstallerRT.nsi file in this folder and modify the following lines
      to match the version of the Windows Vulkan Runtime you wish to build:
         !define VERSION_ABI_MAJOR
         !define VERSION_API_MAJOR
         !define VERSION_MINOR
         !define VERSION_PATCH
   
   4. Right click on the InstallerRT.nsi file and select "Compile NSIS Script".
      The Windows Vulkan Runtime Installer package file will be created in this
      folder. The name of the installer file is VulkanRT-<version>-Installer.exe.
