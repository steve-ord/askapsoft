#!/bin/bash -l
#
# Launches a job to create a catalogue of sources in the continuum image.
#
# @copyright (c) 2015 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# @author Matthew Whiting <Matthew.Whiting@csiro.au>
#

if [ $DO_SOURCE_FINDING == true ]; then

    # List of images to convert to FITS in the Selavy job
    imlist=""

    if [ $NUM_TAYLOR_TERMS == 1 ]; then
        image=${OUTPUT}/image.${imageBase}.restored
        weights=${OUTPUT}/weights.${imageBase}
        imlist="${imlist} ${image}"
    else
        image=${OUTPUT}/image.${imageBase}.taylor.0.restored
        weights=${OUTPUT}/weights.${imageBase}.taylor.0
        imlist="${imlist} ${image}"
        if [ -e ${OUTPUT}/image.${imageBase}.taylor.1.restored ]; then
            imlist="${imlist} ${OUTPUT}/image.${imageBase}.taylor.1.restored"
        fi
        if [ -e ${OUTPUT}/image.${imageBase}.taylor.2.restored ]; then
            imlist="${imlist} ${OUTPUT}/image.${imageBase}.taylor.2.restored"
        fi
            
    fi

    if [ $BEAM == "all" ]; then
        weightpars="Selavy.Weights.weightsImage = ${weights##*/}.fits
Selavy.Weights.weightsCutoff = ${LINMOS_CUTOFF}"
        imlist="${imlist} ${weights}"
    else
        weightpars="#"
    fi

    # get the text that does the FITS conversion - put in $fitsConvertText
    convertToFITStext

    setJob science_selavy selavy
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SOURCEFINDING}
#SBATCH --ntasks=${NUM_CPUS_SELAVY}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SELAVY}
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-selavy-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

seldir=selavy_${imageBase}
mkdir -p \$seldir

cd \${seldir}
ln -s ${logs} .
ln -s ${parsets} .

for im in ${imlist}; do 
    casaim="../\${im##*/}"
    fitsim="../\${im##*/}.fits"
    ${fitsConvertText}
    # make a link so we point to a file in the current directory for Selavy
    ln -s \${im}.fits .
    rejuvenate \${casaim}
done

parset=${parsets}/science_selavy_${FIELDBEAM}_\${SLURM_JOB_ID}.in
log=${logs}/science_selavy_${FIELDBEAM}_\${SLURM_JOB_ID}.log

cat > \$parset <<EOFINNER
Selavy.image = ${image##*/}.fits
Selavy.SBid = ${SB_SCIENCE}
Selavy.nsubx = ${SELAVY_NSUBX}
Selavy.nsuby = ${SELAVY_NSUBY}
#
Selavy.snrCut = ${SELAVY_SNR_CUT}
Selavy.flagGrowth = ${SELAVY_FLAG_GROWTH}
Selavy.growthCut = ${SELAVY_GROWTH_CUT}
#
Selavy.VariableThreshold = ${SELAVY_VARIABLE_THRESHOLD}
Selavy.VariableThreshold.boxSize = ${SELAVY_BOX_SIZE}
Selavy.VariableThreshold.ThresholdImageName=detThresh.img
Selavy.VariableThreshold.NoiseImageName=noiseMap.img
Selavy.VariableThreshold.AverageImageName=meanMap.img
Selavy.VariableThreshold.SNRimageName=snrMap.img
${weightpars}
#
Selavy.Fitter.doFit = true
Selavy.Fitter.fitTypes = [full]
Selavy.Fitter.numGaussFromGuess = true
Selavy.Fitter.maxReducedChisq = 10.
#
Selavy.threshSpatial = 5
Selavy.flagAdjacent = false
#
Selavy.minPix = 3
Selavy.minVoxels = 3
Selavy.minChannels = 1
Selavy.sortingParam = -pflux
EOFINNER

NCORES=${NUM_CPUS_SELAVY}
NPPN=${CPUS_PER_CORE_SELAVY}
aprun -n \${NCORES} -N \${NPPN} $selavy -c \$parset >> \$log
err=\$?
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi
EOFOUTER

    # Dependencies for the job
    DEP=""
    if [ $BEAM == "all" ]; then
        DEP=`addDep "$DEP" "$ID_LINMOS_SCI"`
    else
        if [ $DO_SELFCAL == true ]; then
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI_SC"`
        else
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI"`
        fi
    fi
    
    if [ $SUBMIT_JOBS == true ]; then
	ID_SOURCEFINDING_SCI=`sbatch ${DEP} $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SOURCEFINDING_SCI} "Run the source-finder on the science observation"
    else
	echo "Would run the source-finder on the science observation with slurm file $sbatchfile"
    fi

    echo " "

    
fi
