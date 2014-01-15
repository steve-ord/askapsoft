#!/bin/bash -l

# Need to define the following beforehand:
# sourcelist, databaseCR, freqChanZeroMHz, useGaussianComponents,
# baseimage, msbase

##############################
## Directories

# Model
logdirCR=${crdir}/Logs
parsetdirCR=${crdir}/Parsets
imagedir=${crdir}/Images
chunkdir=${imagedir}/Chunks
slicedir=${imagedir}/Slices

# Sky Model
scriptdirSM=${smdir}/Scripts
logdirSM=${smdir}/Logs
subimagedirSM=${smdir}/Subimages
parsetdirSM=${smdir}/Parsets

# Visibilities
msdir=${visdir}/MS
parsetdirVis=${visdir}/Parsets
logdirVis=${visdir}/Logs

##############################
## Switches

# Model
doFlatSpectrum=false
doCreateCR=true
doSliceCR=true
if [ $doFlatSpectrum == "true" ]; then
    doSliceCR=false
fi

# Sky Model
doSmoothSM=true
doSF_SM=true
doComparisonSM=true

# Visibilities
doCsim=true
doVisCleanup=false
failureListVis="EmptyFile"
#failureListVis="/scratch/astronomy554/whi550/DataChallenge/Simulation/failure-vis-at200120318-2nd.txt"
#failureListVis="/scratch/astronomy554/whi550/EarlyScienceSimulations/Continuum/failure-vis-dec04-run61.txt"
#failureListVis="/scratch/astronomy554/whi550/EarlyScienceSimulations/Continuum/failure-vis-dec04-run68.txt"
#failureListVis="/scratch/astronomy554/whi550/EarlyScienceSimulations/Continuum/failure-vis-dec04-run79.txt"
#######
# Original list generated by: 
# > grep Exit Visibilities/run47/mkVis.o3858569.* | grep -v "Code: 0"  | sed -e 's/:/ /g' | sed -e 's/\./ /g' | awk '{print $3}'
# Then second list of failures by:
# > for N in `grep Exit Visibilities/run50/mkVis.o3859034.* | grep -v "Code: 0" | sed -e 's/:/ /g' | sed -e 's/\./ /g' | awk '{print $3}'`; do head -$N failure-vis-dec30-run47.txt | tail -1; done

doMergeVis=true
doClobberMergedVis=true
doMergeStage1=true
doMergeStage2=true

doNoise=true
varNoise=false
doCorrupt=false
doAntennaBased=false

###########################################
# Model creation definitions

#nfeeds=9
nfeeds=4

npix=4096
rpix=`echo $npix | awk '{print $1/2}'`
cellsize=6
delt=`echo $cellsize | awk '{print $1/3600.}'`
ra=187.5
dec=-4.0
decStringVis="-04.00.00"
raCat=0.
decCat=0.
decSuffix=`echo $dec | awk '{printf "dec%02d",-$1}'`
baseimage="${baseimage}_${decSuffix}"
msbase="${msbase}_${decSuffix}"

baseimage="${baseimage}_${freqChanZeroMHz}"

if [ ${nfeeds} -eq 4 ]; then
    raCat=1.325
    decCat=1.08
    baseimage="${baseimage}_4beam"
fi

nchan=16416
if [ $doFlatSpectrum == "true" ]; then
    nchan=1
    baseimage="${baseimage}_flat"
fi
rchan=0
chanw=-18.5185185e3
rfreq=`echo ${freqChanZeroMHz} | awk '{printf "%8.6e",$1*1.e6}'`

nstokes=1
rstokes=0
stokesZero=0
dstokes=0

if [ $doFlatSpectrum == "true" ]; then
    nsubxCR=9
    nsubyCR=11
else
    nsubxCR=16
    nsubyCR=21
fi
CREATORWIDTH=`echo $nsubxCR $nsubyCR | awk '{print $1*$2+1}'`
CREATORPPN=20

writeByNode=true
modelimage=${imagedir}/${baseimage}
createTT_CR=true
if [ $writeByNode == "true" ]; then
    modelimage=${chunkdir}/${baseimage}
fi
if [ $doFlatSpectrum == "true" ]; then
    slicebase=${modelimage}
else
    slicebase=${slicedir}/${baseimage}_slice
fi
SLICERWIDTH=100
SLICERNPPN=20

###########################################
# Make the visibilities

csimSelect="#PBS -l select=1:ncpus=1:mem=23GB:mpiprocs=1"

array=ADE12.in
feeds=ASKAP${nfeeds}feeds.in
inttime=30s
dur=6

pol="XX YY"
npol=2
polName="${npol}pol"

doNoise=true
varNoise=false
noiseSlope=0.2258
noiseIntercept=-188.71
freqTsys50=`echo $noiseSlope $noiseIntercept | awk '{print (50.-$2)/$1}'`
tsys=50.
if [ $doNoise == true ]; then
    if [ $varNoise == true ]; then
	noiseName="noiseVariable"
    else
	noiseName="noise${tsys}K"
    fi
else
    noiseName="noNoise"
fi

NGROUPS_CSIM=19
NWORKERS_CSIM=216
NCPU_CSIM=`echo $NWORKERS_CSIM | awk '{print $1+1}'`
NPPN_CSIM=20
chanPerMSchunk=4

doSnapshot=true
wtol=10000
gridder=AWProject
if [ $doSnapshot == true ]; then
    wmax=10000
    maxsup=4096
else
    wmax=15000
    maxsup=8192
fi
nw=129
os=4
pad=1.
doFreqDep=true

if [ $useGaussianComponents != "true" ]; then
    msbase="${msbase}_discs"
fi
if [ $databaseCR == "POSSUM" ]; then
    msbase="${msbase}_cont"
elif [ $databaseCR == "POSSUMHI" ]; then
    msbase="${msbase}_HI"
fi
if [ $doFlatSpectrum == "true" ]; then
    msbase="${msbase}_flat"
fi

###########################################
# Make the sky model image

smoothBmaj=21.7
smoothBmin=17.2
smoothBpa=-90.

SFthresh=5.e-4
SFflagGrowth=true
SFgrowthThresh=2.e-4
SFnsubx=7
SFnsuby=5
SFnNodes=`echo $SFnsubx $SFnsuby | awk '{print int(($1*$2-0.001)/12.)+1}'`
