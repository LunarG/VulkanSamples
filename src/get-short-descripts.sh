#!/bin/bash

# return short description of identified or all samples
#    get-short-descript.sh [sample_source_file_name]

# save command directory
CMDDIR="$(dirname "$(readlink -f ${BASH_SOURCE[0]})")"

# determine if a specific sample is requested; if not display all
SAMP2DISP="$CMDDIR/*.cpp"
if [[ $# > 0 ]]; then
    SAMP2DISP=$1
fi

# read all identified .cpp file(s) and display the short description
for f in $SAMP2DISP
do
   SHORT_DESCRIPT=`sed -n '/VULKAN_SAMPLE_SHORT_DESCRIPTION/{n;p}' $f`
   BNAME=`basename $f`
   echo "$BNAME: $SHORT_DESCRIPT"
done

