#!/usr/bin/env python3
#
# Copyright (c) 2016 The Khronos Group Inc.
# Copyright (c) 2016 Valve Corporation
# Copyright (c) 2016 LunarG, Inc.
# Copyright (c) 2016 Google Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and/or associated documentation files (the "Materials"), to
# deal in the Materials without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Materials, and to permit persons to whom the Materials
# are furnished to do so, subject to the following conditions:
#
# The above copyright notice(s) and this permission notice shall be included
# in all copies or substantial portions of the Materials.
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
# USE OR OTHER DEALINGS IN THE MATERIALS
#
# Author: Mark Young <marky@lunarg.com>

import sys
import os

# Following function code snippet was found on StackOverflow (with a change to lower
# camel-case on the variable names):
#   http://stackoverflow.com/questions/377017/test-if-executable-exists-in-python
def find_executable(program):
    def is_exe(fPath):
        return os.path.isfile(fPath) and os.access(fPath, os.X_OK)

    fPath, fName = os.path.split(program)
    if fPath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None
	
def determine_year(version):
	if version == 8:
		return 2005
	elif version == 9:
		return 2008
	elif version == 10:
		return 2010
	elif version == 11:
		return 2012
	elif version == 12:
		return 2013
	elif version == 14:
		return 2015
	else:
		return 0000
	
# Determine if msbuild is in the path, then call it to determine the version and parse
# it into a format we can use, which is "<version_num> <version_year>".
if __name__ == '__main__':
	exeName     = 'msbuild.exe'
	versionCall = exeName + ' /ver'

	# Determine if the executable exists in the path, this is critical.
	#
	foundExeName = find_executable(exeName)

	# If not found, return an invalid number but in the appropriate format so it will
	# fail if the program above tries to use it.
	if foundExeName == None:
		print('00 0000')
		print('Executable ' + exeName + ' not found in PATH!')		
	else:
		sysCallOut = os.popen(versionCall).read()
		
		version = None

		# Split around any spaces first
		spaceList  = sysCallOut.split(' ')
		for spaceString in spaceList:

			# If we've already found it, bail.
			if version != None:
				break
		
			# Now split around line feeds
			lineList = spaceString.split('\n')
			for curLine in lineList:

				# If we've already found it, bail.
				if version != None:
					break
			
				# We only want to continue if there's a period in the list
				if '.' not in curLine:
					continue

				# Get the first element and determine if it is a number, if so, we've
				# got our number.
				splitAroundPeriod = curLine.split('.')
				if splitAroundPeriod[0].isdigit():
					version = int (splitAroundPeriod[0])
					break
		
		# Failsafe to return a number in the proper format, but one that will fail.
		if version == None:
			version = 00

		# Determine the year associated with that version
		year = determine_year(version)
        
		# Output the string we need for Cmake to properly build for this version
		print(str(version) + ' ' + str(year))
