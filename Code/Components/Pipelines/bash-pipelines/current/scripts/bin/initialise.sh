#!/usr/bin/env bash
#
# A script to set up the working directory prior to starting the job
# submission. This defines all necessary subdirectories and sets the
# date-time stamp.
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

####################
# Define & create directories
CWD=`pwd`
parsets=${CWD}/parsets
logs=${CWD}/logs
slurms=${CWD}/slurmFiles
slurmOut=${CWD}/slurmOutput
tools=${CWD}/tools

mkdir -p $parsets
mkdir -p $logs
mkdir -p $slurms
mkdir -p $slurmOut
mkdir -p $tools

####################
# Date and time stamp
NOW=`date +%F-%H%M`

# File to record list of jobs and descriptions
JOBLIST="${slurmOut}/jobList-${NOW}.txt"

####################
# Define the default

. ${PIPELINEDIR}/defaultConfig.sh
