#!/bin/bash -l
#
# Launches a job to extract the science observation from the
# appropriate measurement set, selecting the required beam and,
# optionally, a given scan or field
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

if [ "${splitNeeded}" == true ]; then
    
    # need to run mssplit as well as merging
    
    setJob splitmerge_science splitmerge
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPLIT_SCIENCE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-splitMergeSci-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

finalMS=${msSci}
sbScienceDir=$sbScienceDir
BEAM=$BEAM
inputMSlist=\$(\${PIPELINEDIR}/getMatchingMS.py -d \${sbScienceDir} -b \$BEAM)

COUNT=0
mergelist=""
for ms in \${inputMSlist}; do

    sedstr="s/\.ms/_\${COUNT}\.ms/g"
    outputms=\$(echo \$finalMS | sed -e \$sedstr)
    mergelist="\$mergelist \$outputms"
    parset=${parsets}/split_science_${FIELDBEAM}_\${SLURM_JOB_ID}_\${COUNT}.in
    cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = \${ms}

# Output measurement set
# Default: <no default>
outputvis   = \${outputms}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
$channelParam

# Beam selection via beam ID
# Select an individual beam
beams        = [${BEAM}]

# Scan selection for the science observation
$scanParam

# Field selection for the science observation
$fieldParam

# Set a larger bucketsize
stman.bucketsize  = ${BUCKET_SIZE}
# Make the tile size 54 channels, as that is what we will average over
stman.tilenchan   = ${TILE_NCHAN_SCIENCE}
EOFINNER

    log=${logs}/split_science_${FIELDBEAM}_\${SLURM_JOB_ID}_\${COUNT}.log

    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${mssplit} -c "\${parset}" > "\${log}"
    err=\$?
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_\${COUNT} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi
    ((COUNT++))
done

commandLineFlags="-c ${TILE_NCHAN_SCIENCE} -o \${msSci} \${mergelist}"
log=${logs}/mergeMS_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${msmerge} \${commandLineFlags} >> "\${log}"
err=\$?
rejuvenate ${msSci}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi
purgeMSs=${PURGE_INTERIM_MS_SCI}
if [ "\${purgeMS}" == "true" ]; then
    for ms in \${mergelist}; do
        rm -rf \${ms}
    done
fi


EOFOUTER

    verb="splitting and merging"
       
else

    # Just identify the input files needed and merge them to create
    # the desired dataset.
        
    setJob merge_science merge
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPLIT_SCIENCE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-merge-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

log=${logs}/mergeMS_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log

msSci=$msSci
sbScienceDir=$sbScienceDir
BEAM=$BEAM
inputMSlist=\$(\${PIPELINEDIR}/getMatchingMS.py -d \${sbScienceDir} -b \$BEAM)

commandLineFlags="-c ${TILE_NCHAN_SCIENCE} -o \${msSci} \${inputMSlist}"

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${msmerge} \${commandLineFlags} > "\${log}"
err=\$?
rejuvenate \${msSci}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi


EOFOUTER

        verb="merging"

fi