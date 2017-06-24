"""
This utility uses vswhere to find MSBuild.exe, which must be on the path to build VulkanSamples.

https://github.com/Microsoft/vswhere
https://github.com/Microsoft/vswhere/wiki/Find-MSBuild

You can add it to the PATH like this:
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin

"""
import os, subprocess

if __name__ == "__main__":
    pf86 = os.getenv('ProgramFiles(x86)')
    vswhere = '{pf86}\\Microsoft Visual Studio\\Installer\\vswhere.exe'.format(pf86=pf86)
    cmd = (vswhere, '-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild', '-property', 'installationPath')
    tool_path = subprocess.check_output(cmd).split(b'\r')[0].decode('utf8')
    msbuild_path = os.path.join(tool_path, 'MSBuild', '15.0', 'Bin', 'MSBuild.exe')
    isexe = os.path.isfile(msbuild_path) and os.access(msbuild_path, os.X_OK)
    if isexe:
        print('MSBuild.exe found')
        print('You can add the folder to your path with')
        print('SET PATH=%PATH%;' + '"{}"'.format(os.path.dirname(msbuild_path)))
    else:
        print('msbuild not found')
