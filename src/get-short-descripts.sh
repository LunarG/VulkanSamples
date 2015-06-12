#!/bin/bash

# save command directory
CMDDIR="$(dirname "$(readlink -f ${BASH_SOURCE[0]})")"

# read all source .cpp files and display the short description
for f in $CMDDIR/*.cpp
do
   SHORT_DESCRIPT=`sed -n '/VULKAN_SAMPLE_SHORT_DESCRIPTION/{n;p}' $f`
   BNAME=`basename $f`
   echo "$BNAME: $SHORT_DESCRIPT"
done

