#!/bin/bash
#To be run on remote machine
# --------------------------------------------------
# Configuration ccache
# --------------------------------------------------
export CCACHE_DIR=$PWD/.ccache
mkdir -p $CCACHE_DIR

#Take input arguments as an array
myArray=( "$@" )
#Array: Size=$#, an element=$1, all element = $@

printf "Start skimming at ";/bin/date
printf "Worker node hostname ";/bin/hostname

if [ -z ${_CONDOR_SCRATCH_DIR} ] ; then 
    echo "Running Interactively" ; 
else
    echo "Running In Batch"
    echo ${_CONDOR_SCRATCH_DIR}
	tar -zxf Skim.tar.gz
    cd Skim
    make clean
    make
fi

#Run for Base, Signal region
echo "All arguements: "$@
echo "Number of arguements: "$#
oName=$1
outDir=$2
echo "./runMain -o oName"
./runMain -o ${oName}

printf "Done skimming at ";/bin/date
#---------------------------------------------
#Copy the ouput root files
#---------------------------------------------
if [ -z ${_CONDOR_SCRATCH_DIR} ] ; then
    echo "Running Interactively" ;
else
    xrdcp -f output/${oName} ${outDir}/${oName}
    echo "Cleanup"
    cd ..
    rm -rf Skim 
fi
printf "Done ";/bin/date
