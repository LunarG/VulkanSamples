#!/bin/bash
#set -x
if [ -t 1 ] ; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    NC='\033[0m' # No Color
else
    RED=''
    GREEN=''
    NC=''
fi

printf "$GREEN[ RUN      ]$NC $0\n"

# Create a temp directory to run the test in
rm -rf vktracereplay_tmp
mkdir vktracereplay_tmp
cd vktracereplay_tmp
cp ../../vktrace/vkreplay .
cp ../../vktrace/vktrace .
cp ../../vktrace/libvulkan_trace.so .
#cp ../../vktrace/libvulkan_replay.so .
cp ../../demos/cube .
cp ../../demos/*png .
cp ../../demos/*spv .
export LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH
export VK_LAYER_PATH=`pwd`/../../layers
(
    ./vktrace -s 1 -p cube  -o c01.vktrace -l0 libvulkan_trace.so &
    P=$!
    sleep 3
    kill $P
) >/dev/null 2>&1
mv 1.ppm 1_trace.ppm
./vkreplay -s 1 -t c01.vktrace >/dev/null 2>&1
#cp cube 1.ppm  # For testing this script -- force a failure
#rm 1.ppm       # For testing this script -- force a failure
cmp -s 1.ppm 1_trace.ppm
RES=$?
cd ..
rm -rf vktracereplay_tmp

if [ $RES -eq 0 ] ; then
   printf "$GREEN[  PASSED  ]$NC 1 test\n"
   exit 0
else
   printf "$RED[  FAILED  ]$NC screenshot file compare failed\n"
   printf "$RED[  FAILED  ]$NC 1 test\n"
   printf "1 TEST FAILED\n"
   exit 1
fi
