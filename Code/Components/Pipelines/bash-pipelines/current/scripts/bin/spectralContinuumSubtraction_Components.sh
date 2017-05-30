#!/bin/bash -l
#
# Sets up the continuum-subtraction job for the case where the
# continuum is represented by a model image created by a collection of
# components generated by Selavy
#
# @copyright (c) 2017 CSIRO
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

# In this bit we use Selavy to make a continuum catalogue, then
# cmodel to create the corresponding continuum model image, which
# is then subtracted from the MS

imageCode=restored
setImageProperties cont
selavyImage="${OUTPUT}/${imageName}"
if [ "${IMAGETYPE_CONT}" == "fits" ]; then
    selavyImage="${selavyImage}.fits"
fi

NPROCS_CONTSUB=$(echo "${CONTSUB_SELAVY_NSUBX}" "${CONTSUB_SELAVY_NSUBY}" | awk '{print $1*$2+1}')
if [ "${NPROCS_CONTSUB}" -eq 1 ]; then
    # If Selavy is just serial, increase nprocs to 2 for cmodel
    NPROCS_CONTSUB=2
fi
if [ "${NPROCS_CONTSUB}" -le 20 ]; then
    CPUS_PER_CORE_CONTSUB=${NPROCS_CONTSUB}
else
    CPUS_PER_CORE_CONTSUB=20
fi

components=modelComponents.in

cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPECTRAL_CONTSUB}
#SBATCH --ntasks=${NPROCS_CONTSUB}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONTSUB}
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-contsubSLsci-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

log=${logs}/mslist_for_contsub_\${SLURM_JOB_ID}.log
NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} $mslist --full "${msSci}" 1>& "\${log}"
ra=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$log" --val=RA)
ra=\$(echo \$ra | awk -F':' '{printf "%sh%sm%s",\$1,\$2,\$3}')
dec=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$log" --val=Dec)
epoch=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$log" --val=Epoch)
refDirection="[\${ra}, \${dec}, \${epoch}]"

contsubdir=ContSubBeam${BEAM}
mkdir -p \${contsubdir}
cd \${contsubdir}

#################################################
# First, source-finding

parset=${parsets}/selavy_for_contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.in
log=${logs}/selavy_for_contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.log
cat >> "\$parset" <<EOFINNER
##########
## Source-finding with selavy
##
# The image to be searched
Selavy.image                                    = ${selavyImage}
#
# This is how we divide it up for distributed processing, with the
#  number of subdivisions in each direction, and the size of the
#  overlap region in pixels
Selavy.nsubx                                    = ${SELFCAL_SELAVY_NSUBX}
Selavy.nsuby                                    = ${SELFCAL_SELAVY_NSUBY}
Selavy.overlapx                                 = 50
Selavy.overlapy                                 = 50
#
# The search threshold, in units of sigma
Selavy.snrCut                                   = ${SELFCAL_SELAVY_THRESHOLD}
# Grow the detections to a secondary threshold
Selavy.flagGrowth                               = true
Selavy.growthCut                                = 5
#
# Turn on the variable threshold option
Selavy.VariableThreshold                        = true
Selavy.VariableThreshold.boxSize                = 50
Selavy.VariableThreshold.ThresholdImageName     = detThresh.img
Selavy.VariableThreshold.NoiseImageName         = noiseMap.img
Selavy.VariableThreshold.AverageImageName       = meanMap.img
Selavy.VariableThreshold.SNRimageName           = snrMap.img
#
# Parameters to switch on and control the Gaussian fitting
Selavy.Fitter.doFit                             = true
# Fit all 6 parameters of the Gaussian
Selavy.Fitter.fitTypes                          = [full]
# Limit the number of Gaussians to 1
Selavy.Fitter.maxNumGauss = 1
# Do not use the number of initial estimates to determine how many Gaussians to fit
Selavy.Fitter.numGaussFromGuess = false
# The fit may be a bit poor, so increase the reduced-chisq threshold
Selavy.Fitter.maxReducedChisq = 15.
#
# Allow islands that are slightly separated to be considered a single 'source'
Selavy.flagAdjacent = false
# The separation in pixels for islands to be considered 'joined'
Selavy.threshSpatial = 7
#
# Saving the fitted components to a parset for use by ccalibrator
Selavy.outputComponentParset                    = true
Selavy.outputComponentParset.filename           = ${components}
# Reference direction for which component positions should be measured
#  relative to.
Selavy.outputComponentParset.referenceDirection = \${refDirection}
# Keep *all* components in the output parset
#
# Size criteria for the final list of detected islands
Selavy.minPix                                   = 3
Selavy.minVoxels                                = 3
Selavy.minChannels                              = 1
#
# How the islands are sorted in the final catalogue - by
#  integrated flux in this case
Selavy.sortingParam                             = -iflux
EOFINNER

NCORES=${NPROCS_CONTSUB}
NPPN=${CPUS_PER_CORE_CONTSUB}
aprun -n \${NCORES} -N \${NPPN} ${selavy} -c "\${parset}" > "\${log}"
err=\$?
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_selavy "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

cd ..

if [ ! -e "\${contsubdir}/${components}" ]; then

    # Nothing detected !
    echo "Continuum subtraction : No continuum components found!"

else

    #################################################
    # Then, continuum subtraction

    parset=${parsets}/contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.in
    log=${logs}/contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.log
    cat > "\$parset" <<EOFINNER
# The measurement set name - this will be overwritten
CContSubtract.dataset                             = ${msSciSL}
# The model definition
CContSubtract.sources.definition                  = \${contsubdir}/${components}
# The gridding parameters
CContSubtract.gridder.snapshotimaging             = ${GRIDDER_SNAPSHOT_IMAGING}
CContSubtract.gridder.snapshotimaging.wtolerance  = ${GRIDDER_SNAPSHOT_WTOL}
CContSubtract.gridder.snapshotimaging.longtrack   = ${GRIDDER_SNAPSHOT_LONGTRACK}
CContSubtract.gridder.snapshotimaging.clipping    = ${GRIDDER_SNAPSHOT_CLIPPING}
CContSubtract.gridder                             = WProject
CContSubtract.gridder.WProject.wmax               = ${GRIDDER_WMAX}
CContSubtract.gridder.WProject.nwplanes           = ${GRIDDER_NWPLANES}
CContSubtract.gridder.WProject.oversample         = ${GRIDDER_OVERSAMPLE}
CContSubtract.gridder.WProject.maxfeeds           = 1
CContSubtract.gridder.WProject.maxsupport         = ${GRIDDER_MAXSUPPORT}
CContSubtract.gridder.WProject.frequencydependent = true
CContSubtract.gridder.WProject.variablesupport    = true
CContSubtract.gridder.WProject.offsetsupport      = true
EOFINNER

    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} ${ccontsubtract} -c "\${parset}" > "\${log}"
    err=\$?
    rejuvenate ${msSciSL}
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch "$CONT_SUB_CHECK_FILE"
    fi

fi

EOFOUTER
