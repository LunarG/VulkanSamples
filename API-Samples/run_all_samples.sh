#!/bin/bash

# run all of the samples

# save command directory; note that this file is a link to file of same
# name in samples/src/
CMDDIR="$(dirname "$(readlink -f ${BASH_SOURCE[0]})")"

# get the list of built samples to run
SAMP2RUN=`find $CMDDIR -name *.cpp`
#echo "SAMP2RUN is $SAMP2RUN"

# display short description of the sample and run it
for f in $SAMP2RUN
do
   # get short description of the sample source file
   DESCRIPT=`$CMDDIR/get-short-descripts.sh --nofname --sampfname $f`
   BNAME=$(basename $f)
   echo "RUNNING SAMPLE:  $BNAME"
   echo "  ** $DESCRIPT"

   # run the built sample; need to remove .cpp from name
   RNAME=./${BNAME%.cpp}
   $RNAME
   echo ""
done

