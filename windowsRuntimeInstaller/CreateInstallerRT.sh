#!/bin/bash
# Bash script to create the Vulkan Runtime Installer.

# Create the uinstaller
makensis /DUNINSTALLER InstallerRT.nsi
$TEMP/tempinstaller.exe
mv $TEMP/UninstallVulkanRT.exe .

# Sign the Uninstaller
# Replace SIGNFILE with your command and necessary args for
# signing an executable. If you don't need to sign the uninstaller,
# you can comment out this line.
./SIGNFILE ./UninstallVulkanRT.exe

# Create the RT Installer, using the signed uninstaller
makensis InstallerRT.nsi
