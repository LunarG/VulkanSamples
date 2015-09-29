#!/bin/bash

# run all of the samples

# save command directory; note that this file is a link to file of same
# name in samples/src/
CMDDIR="$(dirname "$(readlink -f ${BASH_SOURCE[0]})")"

# get the list of built samples to run
SAMP2RUN=`ls $CMDDIR/vk*`
#echo "SAMP2RUN is $SAMP2RUN"

# display short description of the sample and run it
for f in $SAMP2RUN
do
   # get short description of the sample source file
   DESCRIPT=`$CMDDIR/get-short-descripts.sh --nofname --sampfname $f`
   BNAME=$(basename $f)
   echo "RUNNING SAMPLE:  $BNAME"
   echo "  ** $DESCRIPT"

   # run the built sample; need to add build to path and remove .cpp from name
   SNAME=$(echo $f | sed -e "s/src/build\/src/g")
   #RNAME=$(echo $SNAME | sed -e "s/.cpp//g")
   RNAME=${SNAME%.cpp}
   $RNAME
   echo ""
done

