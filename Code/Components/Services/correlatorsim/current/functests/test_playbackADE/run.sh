#!/bin/bash

cd `dirname $0`

# Setup the environment
if [ ! -f ../../init_package_env.sh ]; then
    echo "Error: init_package_env.sh does not exist, please run rbuild in package dir"
    exit 1
fi
source ../../init_package_env.sh
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

# Start the Ice Services
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
if [ $? -ne 0 ]; then
    echo "Error: Failed to start Ice Services"
    exit 1
fi
sleep 1

# Start the metadata subscriber 
# (don't use the script so this script can kill it)
../../apps/msnoop -c msnoop.in -v > msnoop.log 2>&1 &
MDPID=$!

# Start the visibilities receiver 
# (don't use the script so this script can kill it)
../../apps/vsnoopADE -v -p 3001 > vsnoop.log 2>&1 &
VISPID=$!

# Run the test
# For the moment, keep to 2 processes:
# process 0 for configuration validation
# process 1 for streaming
mpirun -np 2 ../../apps/playbackADE.sh -c playback.in
STATUS=$?
echo "playbackADE status: " $STATUS

# Give the subscriber a moment to get the last messages then exit
sleep 5
kill $MDPID
echo "Killed msnoop"
kill $VISPID 
echo "Killed vsnoop"
sleep 1
kill -9 $MDPID $VISPID > /dev/null 2>&1

# Stop the Ice Services
../stop_ice.sh ../icegridadmin.cfg

exit $STATUS
