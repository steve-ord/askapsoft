/// @file ChannelMergeTask.cc
///
/// Merge of channel space handled by adjacent ranks
///
/// @copyright (c) 2010 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Max Voronkov <maxim.voronkov@csiro.au>

// Include own header file first
#include "ChannelMergeTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Cube.h"
#include "cpcommon/VisChunk.h"

// boost includes
#include "boost/shared_array.hpp"

// Local package includes
#include "configuration/Configuration.h"
#include "monitoring/MonitoringSingleton.h"

ASKAP_LOGGER(logger, ".ChannelMergeTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

ChannelMergeTask::ChannelMergeTask(const LOFAR::ParameterSet& parset,
        const Configuration& config) : itsConfig(config),
    itsRanksToMerge(static_cast<int>(parset.getUint32("ranks2merge", config.nprocs() + 1))),
    itsCommunicator(MPI_COMM_NULL), itsRankInUse(false), itsGroupWithActivatedRank(true), 
    itsUseInactiveRanks(parset.getBool("spare_ranks",false))
{
    ASKAPLOG_DEBUG_STR(logger, "Constructor");
    ASKAPCHECK(config.nprocs() > 1,
            "This task is intended to be used in parallel mode only");
}

ChannelMergeTask::~ChannelMergeTask()
{
    ASKAPLOG_DEBUG_STR(logger, "Destructor");
    if (itsCommunicator != MPI_COMM_NULL) {
        const int response = MPI_Comm_free(&itsCommunicator);
        ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_free = "<<response);
    }
}

void ChannelMergeTask::process(VisChunk::ShPtr& chunk)
{
    if (itsCommunicator == MPI_COMM_NULL) {
        // this is the first iteration
        configureRanks(chunk);
        if (!itsRankInUse) {
            return;
        }
    } else {
        ASKAPASSERT(itsRankInUse);
    }

    // the following should create chunk of the correct dimensions
    checkChunkForConsistency(chunk);
    ASKAPDEBUGASSERT(chunk);

    if (localRank() > 0) {
        // these ranks just send VisChunks they handle to the master (rank 0)
        sendVisChunk(chunk);
        // reset chunk as this rank now becomes inactive
        chunk.reset();
    } else {
        // this is the master process which receives the data
        receiveVisChunks(chunk);
    }
}

/// @brief receive chunks in the rank 0 process
/// @details This method implements the part of the process method
/// which is intended to be executed in rank 0 (the master process)
/// @param[in] chunk the instance of VisChunk to work with
void ChannelMergeTask::receiveVisChunks(askap::cp::common::VisChunk::ShPtr chunk) const
{
   const int rankOffset = itsGroupWithActivatedRank ? 1 : 0;
   const int nLocalRanks = itsRanksToMerge + rankOffset;

   // 1) create new frequency vector, visibilities and flags

   const casa::uInt nChanOriginal = 
           itsGroupWithActivatedRank ? chunk->nChannel() / itsRanksToMerge : chunk->nChannel();
   // buffers or references to the new chunk
   casa::Vector<casa::Double> newFreq;
   casa::Cube<casa::Complex> newVis;
   casa::Cube<casa::Bool> newFlag;
   if (itsGroupWithActivatedRank) {
       ASKAPDEBUGASSERT(nChanOriginal * itsRanksToMerge == chunk->nChannel());
       newFreq.reference(chunk->frequency());
       newVis.reference(chunk->visibility());
       newVis.set(casa::Complex(0.,0.));
       newFlag.reference(chunk->flag());
       newFlag.set(true);
   } else {
       newFreq.reference(casa::Vector<casa::Double>(nChanOriginal * itsRanksToMerge));
       newVis.reference(casa::Cube<casa::Complex>(chunk->nRow(), nChanOriginal * itsRanksToMerge, 
                                    chunk->nPol(), casa::Complex(0.,0.)));
       newFlag.reference(casa::Cube<casa::Bool>(chunk->nRow(), nChanOriginal * itsRanksToMerge, 
                                    chunk->nPol(), true));
   }

   // 2) receive times from all ranks to ensure consistency
   // (older data will not be copied and therefore will be flagged)
   
   // MVEpoch is basically two doubles
   ASKAPDEBUGASSERT(itsRanksToMerge > 1);
   boost::shared_array<double> timeRecvBuf(new double[2 * nLocalRanks]);
   // not really necessary to set values for the master rank, but handy for consistency
   timeRecvBuf[0] = chunk->time().getDay();
   timeRecvBuf[1] = chunk->time().getDayFraction();
   const int response = MPI_Gather(MPI_IN_PLACE, 2, MPI_DOUBLE, timeRecvBuf.get(),
                2, MPI_DOUBLE, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering times, response from MPI_Gather = "<<response);

   // 3) find the latest time - we ignore all chunks which are from the past
   // (could, in principle, find time corresponding to the largest portion of valid data,
   // but probably not worth it as we don't normally expect to loose any packets).

   //casa::MVEpoch latestTime(chunk->time());
   casa::MVEpoch latestTime(timeRecvBuf[2 * rankOffset], timeRecvBuf[2 *  rankOffset + 1]);

   // invalid chunk flag per rank, zero length array means that all chunks are valid
   // (could've stored validity flags as opposed to invalidity flags, but it makes the
   //  code a bit less readable).
   std::vector<bool> invalidFlags;

   for (int rank = 1 + rankOffset; rank < nLocalRanks; ++rank) {
        const casa::MVEpoch currentTime(timeRecvBuf[2 * rank], timeRecvBuf[2 *  rank + 1]);
        if ((latestTime - currentTime).get() < 0.) {
             latestTime = currentTime;
             // this also means that there is at least one bad chunk to exclude
             if (invalidFlags.size() == 0) {
                 invalidFlags.resize(itsRanksToMerge, true);
             }
        }
   }
   if (itsGroupWithActivatedRank) {
       ASKAPDEBUGASSERT(localRank() == 0);
       chunk->time() = latestTime;
   }

   if (invalidFlags.size() > 0) {
       ASKAPLOG_DEBUG_STR(logger, "VisChunks being merged correspond to different times, keep only the latest = "<<latestTime);
       int counter = 0;
       for (size_t rank = 0; rank < invalidFlags.size(); ++rank) {
            const casa::MVEpoch currentTime(timeRecvBuf[2 * (rank + rankOffset)], timeRecvBuf[2 *  (rank + rankOffset) + 1]);
            if (latestTime.nearAbs(currentTime)) {
                invalidFlags[rank] = false;
                ++counter;
            }
       }
       ASKAPCHECK(counter != 0, "It looks like comparison of time stamps failed due to floating point precision, this shouldn't have happened!");
       // case of counter == itsRanksToMerge is not supposed to be inside this if-statement
       ASKAPDEBUGASSERT(counter < itsRanksToMerge);
       ASKAPLOG_DEBUG_STR(logger, "      - keeping "<<counter<<" chunks out of "<<itsRanksToMerge<<
                                  " merged");
       MonitoringSingleton::update<int32_t>("MisalignedStreamsCount", counter);
       ASKAPDEBUGASSERT(itsRanksToMerge > 0);
       MonitoringSingleton::update<float>("MisalignedStreamsPercent", static_cast<float>(counter) / itsRanksToMerge * 100.);
   } else {
       MonitoringSingleton::update<int32_t>("MisalignedStreamsCount", 0);
       MonitoringSingleton::update<float>("MisalignedStreamsPercent", 0.);
   }
   
   // 4) receive and merge frequency axis
   {
      boost::shared_array<double> freqRecvBuf(new double[nChanOriginal * nLocalRanks]);
      const int response = MPI_Gather(chunk->frequency().data(), nChanOriginal, MPI_DOUBLE, 
                freqRecvBuf.get(), nChanOriginal, MPI_DOUBLE, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Error gathering frequencies, response from MPI_Gather = "<<response);
      
      for (int rank = 0; rank < itsRanksToMerge; ++rank) {
           // always merge frequencies, even if the data are not valid
           casa::Vector<double> thisFreq;
           thisFreq.takeStorage(casa::IPosition(1,nChanOriginal), freqRecvBuf.get() + (rank + rankOffset) * nChanOriginal, casa::SHARE);
           newFreq(casa::Slice(rank * nChanOriginal, nChanOriginal)) = thisFreq;
      }
   }
  
   // 5) receive and merge visibilities (each is two floats)
   {
      boost::shared_array<float> visRecvBuf(new float[2 * chunk->visibility().nelements() * nLocalRanks]);
      ASKAPASSERT(chunk->visibility().contiguousStorage());
      const int response = MPI_Gather((float*)chunk->visibility().data(), chunk->visibility().nelements() * 2, 
            MPI_FLOAT, visRecvBuf.get(), chunk->visibility().nelements() * 2, MPI_FLOAT, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Error gathering visibilities, response from MPI_Gather = "<<response);
     
      // it is a bit ugly to rely on actual representation of casa::Complex, but this is done
      // to benefit from optimised MPI routines
      fillCube((casa::Complex*)visRecvBuf.get(), newVis, invalidFlags);
   }
    
   // 6) receive flags (each is casa::Bool)
   {
      ASKAPASSERT(chunk->flag().contiguousStorage());
      ASKAPDEBUGASSERT(sizeof(casa::Bool) == sizeof(char));
      boost::shared_array<casa::Bool> flagRecvBuf(new casa::Bool[chunk->flag().nelements() * nLocalRanks]);
      const int response = MPI_Gather((char*)chunk->flag().data(), chunk->flag().nelements(), 
            MPI_CHAR, (char*)flagRecvBuf.get(), chunk->flag().nelements(), MPI_CHAR, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Error gathering flags, response from MPI_Gather = "<<response);

      // it is a bit ugly to rely on actual representation of casa::Bool, but this is done
      // to benefit from optimised MPI routines
      fillCube(flagRecvBuf.get(), newFlag, invalidFlags);
   }
   
   // 7) update the chunk, unless this is a brand new chunk
   if (!itsGroupWithActivatedRank) {
       chunk->resize(newVis, newFlag, newFreq);
   }

   // 8) check that the resulting frequency axis is contiguous
   if (newFreq.nelements() > 1) {
       const double resolution = (newFreq[newFreq.nelements() - 1] - newFreq[0]) / (newFreq.nelements() - 1);
       for (casa::uInt chan = 0; chan < newFreq.nelements(); ++chan) {
            const double expected = newFreq[0] + resolution * chan;
            // 1 kHz tolerance should be sufficient for practical purposes
            if (fabs(expected - newFreq[chan]) > 1e3) {
                ASKAPLOG_WARN_STR(logger, "Frequencies in the merged chunks seem to be non-contiguous, "<<
                "for resulting channel = "<<chan<<" got "<<newFreq[chan]/1e6<<" MHz, expected "<<
                expected / 1e6<<" MHz, estimated resolution "<<resolution / 1e3<<" kHz");
                break;
            }
       }
   }
}

/// @brief helper method to copy data from flat buffer
/// @details MPI routines work with raw pointers. This method encasulates
/// all ugliness of marrying this with casa cubes.
/// @param[in] buf contiguous memory buffer with stacked individual slices side to side
/// @param[in,out]  out output cube, number of channels is itsRanksToMerge times the number
///                 of channels in each slice (same number of rows and polarisations)
/// @param[in] invalidFlags if this vector has non-zero length, slices corresponding to 
///                 'true' are not copied
template<typename T>
void ChannelMergeTask::fillCube(const T* buf, casa::Cube<T> &out, 
                       const std::vector<bool> &invalidFlags) const
{
   ASKAPDEBUGASSERT(out.ncolumn() % itsRanksToMerge == 0);
   const int rankOffset = itsGroupWithActivatedRank ? 1 : 0;
   const casa::IPosition sliceShape(3, out.nrow(), out.ncolumn() / itsRanksToMerge, out.nplane());

   for (int rank = 0; rank < itsRanksToMerge; ++rank) {
        if (invalidFlags.size() > 0) {
            if (invalidFlags[rank]) {
                continue;
            }
        }
        casa::Cube<T> currentSlice;

        // it is a bit ugly to rely on exact representation of the casa::Cube, but
        // this is the only way to benefit from optimised MPI routines
        // const_cast is required due to the generic interface,
        // we don't actually change data using the cast pointer
        currentSlice.takeStorage(sliceShape, const_cast<T*>(buf) + 
                                 (rank + rankOffset) * sliceShape.product(), casa::SHARE);

        const casa::IPosition start(3, 0, rank * sliceShape(1), 0);
        const casa::Slicer slicer(start,sliceShape);
        ASKAPDEBUGASSERT(start(1) < static_cast<int>(out.ncolumn()));
              
        out(slicer) = currentSlice;
   }
}

/// @brief send chunks to the rank 0 process
/// @details This method implements the part of the process method
/// which is intended to be executed in ranks [1..itsRanksToMerge-1].
/// @param[in] chunk the instance of VisChunk to work with
void ChannelMergeTask::sendVisChunk(askap::cp::common::VisChunk::ShPtr chunk) const
{
   // 1) send times corresponding to the current chunk 

   const double timeSendBuf[2] = {chunk->time().getDay(), chunk->time().getDayFraction()};
   int response = MPI_Gather(const_cast<double*>(timeSendBuf), 2, MPI_DOUBLE, NULL, 2, MPI_DOUBLE, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering times, response from MPI_Gather = "<<response);

   // 2) send frequencies corresponding to the current chunk

   ASKAPASSERT(chunk->frequency().contiguousStorage());
   response = MPI_Gather(chunk->frequency().data(), chunk->nChannel(), MPI_DOUBLE, NULL,
            chunk->nChannel(), MPI_DOUBLE, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering frequencies, response from MPI_Gather = "<<response);
   
   // 3) send visibilities (each is two floats)

   ASKAPASSERT(chunk->visibility().contiguousStorage());
   response = MPI_Gather((float*)chunk->visibility().data(), chunk->visibility().nelements() * 2, 
            MPI_FLOAT, NULL, chunk->visibility().nelements() * 2, MPI_FLOAT, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering visibilities, response from MPI_Gather = "<<response);

   // 4) send flags (each is Bool)

   ASKAPASSERT(chunk->flag().contiguousStorage());
   
   ASKAPDEBUGASSERT(sizeof(casa::Bool) == sizeof(char));
   response = MPI_Gather((char*)chunk->flag().data(), chunk->flag().nelements(), 
            MPI_CHAR, NULL, chunk->flag().nelements(), MPI_CHAR, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering flags, response from MPI_Gather = "<<response);
  
}

/// @brief checks chunks presented to different ranks for consistency
/// @details To limit complexity, only a limited number of merging
/// options is supported. This method checks chunks for the basic consistency
/// like matching dimensions. It is intended to be executed on all ranks and
/// uses collective MPI calls. In addition, this method creates a chunk if a new
/// rank is activated.
/// @param[in] chunk the instance of VisChunk to work with
void ChannelMergeTask::checkChunkForConsistency(askap::cp::common::VisChunk::ShPtr& chunk) const
{
    const int nLocalRanks = itsRanksToMerge + (itsGroupWithActivatedRank ? 1 : 0);
    if (itsGroupWithActivatedRank) {
        ASKAPCHECK(bool(chunk) == (localRank() != 0), "Expect idle input stream for the zero local rank, and data for other ranks");
    } else {
        ASKAPCHECK(chunk, "Expect no idle input streams for the local communicator");
    }
    int sendBuf[4];
    if (itsGroupWithActivatedRank && (localRank() == 0)) {
        sendBuf[0] = sendBuf[1] = sendBuf[2] = sendBuf[3] = 0;
    } else {
        sendBuf[0] = static_cast<int>(chunk->nRow());
        sendBuf[1] = static_cast<int>(chunk->nChannel());
        sendBuf[2] = static_cast<int>(chunk->nPol());
        sendBuf[3] = static_cast<int>(chunk->nAntenna());
    }
   
    boost::shared_array<int> receiveBuf(new int[4 * nLocalRanks]);

    const int response = MPI_Allgather(sendBuf, 4, MPI_INT, receiveBuf.get(),
             4, MPI_INT, itsCommunicator);
    ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allgather = "<<response);

    if (itsGroupWithActivatedRank && (localRank() == 0)) {
        // now have to create new visChunk if it is a master rank which has idle input
        // do it straight with the correct shape
        ASKAPDEBUGASSERT(!chunk);
        ASKAPDEBUGASSERT(nLocalRanks > 1);
        // use dimensions of the rank = 1 to set up the new chunk
        chunk.reset(new VisChunk(receiveBuf[4], receiveBuf[5] * itsRanksToMerge, receiveBuf[6], receiveBuf[7]));
    } else {
        for (int rank = itsGroupWithActivatedRank ? 1 : 0; rank < nLocalRanks; ++rank) {
             ASKAPCHECK(sendBuf[0] == receiveBuf[4 * rank], "Number of rows "<<sendBuf[0]<<
                    " is different from that of rank "<<rank<<" ("<<receiveBuf[4 * rank]<<")");
             ASKAPCHECK(sendBuf[1] == receiveBuf[4 * rank + 1], "Number of channels "<<sendBuf[1]<<
                    " is different from that of rank "<<rank<<" ("<<receiveBuf[4 * rank + 1]<<")");
             ASKAPCHECK(sendBuf[2] == receiveBuf[4 * rank + 2], "Number of polarisations  "<<sendBuf[2]<<
                    " is different from that of rank "<<rank<<" ("<<receiveBuf[4 * rank + 2]<<")");
             ASKAPCHECK(sendBuf[3] == receiveBuf[4 * rank + 3], "Number of antennas  "<<sendBuf[3]<<
                    " is different from that of rank "<<rank<<" ("<<receiveBuf[4 * rank + 3]<<")");
        }
    }
    // could in principle check that antenna1, antenna2, etc are consistent but it will waste
    // the resourses
    
}

/// @brief should this task be executed for inactive ranks?
/// @details If a particular rank is inactive, process method is
/// not called unless this method returns true. Possible use cases:
///   - Splitting the datastream expanding parallelism, i.e
///     inactive rank(s) become active after this task.
///   - Need for collective operations 
/// @return true, if process method should be called even if
/// this rank is inactive (i.e. uninitialised chunk pointer
/// will be passed to process method).
/// @note Currently, always return true, but throw an exception if
/// any input data stream is inactive (full implementation would involve
/// setting up MPI communicators dynamcally, rather than in the constructor)
bool ChannelMergeTask::isAlwaysActive() const
{
   // itsGroupWithActivatedRank is true upon initialisation to ensure all ranks call process()
   // after that it is true only for groups of ranks which need activation of additional rank
   return itsGroupWithActivatedRank;
}


/// @brief local rank in the group
/// @details Returns the rank against the local communicator, i.e.
/// the process number in the group of processes contributing to the
/// single output stream.
/// @return rank against itsCommunicator
int ChannelMergeTask::localRank() const
{
   int rank;
   const int response = MPI_Comm_rank(itsCommunicator, &rank);
   
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_rank = "<<response);
   return rank;
}

/// @brief checks the number of ranks to merge against number of ranks
/// @details This method obtains the number of available ranks against
/// the local communicator, i.e. the number of streams to merge and checks
/// that it is the same as itsRanksToMerge or one more, if spare ranks are
/// activated. It also does consistency checks that only one spare rank is
/// activated per group of ranks with valid inputs.
/// @param[in] beingActivated true if this rank is being activated
void ChannelMergeTask::checkRanksToMerge(bool beingActivated) const
{
   int nprocs;
   const int response = MPI_Comm_size(itsCommunicator, &nprocs);
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_size = "<<response);
   ASKAPASSERT(nprocs > 0);
   ASKAPASSERT(nprocs <= itsConfig.nprocs());
   ASKAPASSERT(localRank() < nprocs);

   if (!itsRankInUse) {
       ASKAPLOG_DEBUG_STR(logger, "Rank "<<itsConfig.rank()<<" is unused (total number of unused rank(s): "<<nprocs<<")");
   } else {
       std::vector<int> activityFlags(nprocs, 0);
       if (beingActivated) {
           activityFlags[localRank()] = 1;
       }
       int response = MPI_Allreduce(MPI_IN_PLACE, (void*)activityFlags.data(),
                  activityFlags.size(), MPI_INT, MPI_SUM, itsCommunicator);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
       // obtain the number of ranks being activated in this group
       const int numRanksActivated = std::accumulate(activityFlags.begin(), activityFlags.end(), 0);

       std::vector<int> hasNewRankFlags(nprocs, 0);
       if (itsGroupWithActivatedRank) {
           hasNewRankFlags[localRank()] = 1;
       }
       response = MPI_Allreduce(MPI_IN_PLACE, (void*)hasNewRankFlags.data(),
                  hasNewRankFlags.size(), MPI_INT, MPI_SUM, itsCommunicator);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
       const int numNewRankFlags = std::accumulate(hasNewRankFlags.begin(), hasNewRankFlags.end(), 0);
       ASKAPASSERT((numNewRankFlags == 0) || (numNewRankFlags == static_cast<int>(hasNewRankFlags.size()))); 

       std::vector<int> inUseFlags(nprocs, 0);
       if (itsRankInUse) {
           inUseFlags[localRank()] = 1;
       }
       response = MPI_Allreduce(MPI_IN_PLACE, (void*)inUseFlags.data(),
                  inUseFlags.size(), MPI_INT, MPI_SUM, itsCommunicator);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
       const int numInUseFlags = std::accumulate(inUseFlags.begin(), inUseFlags.end(), 0);
       ASKAPASSERT(numInUseFlags == nprocs);

       if (itsGroupWithActivatedRank) {
           ASKAPCHECK(nprocs > 2, "Expected to have at least 3 ranks in this group");
           ASKAPCHECK(nprocs == itsRanksToMerge + 1, "Number of ranks available through local communicator ("<<
               nprocs<<") doesn't match the chosen number of ranks to merge ("<<itsRanksToMerge<<") plus one");
           ASKAPCHECK(numRanksActivated == 1, "Exactly one service rank is expected to be activated, you have: "<<numRanksActivated);
           ASKAPASSERT(numNewRankFlags == nprocs);
           ASKAPCHECK(!beingActivated || (localRank() == 0), "Rank handling the output is expected to become zero rank w.r.t. local communicator");
       } else {
           ASKAPCHECK(nprocs == itsRanksToMerge, "Number of ranks available through local communicator ("<<
               nprocs<<") doesn't match the chosen number of ranks to merge ("<<itsRanksToMerge<<")");
           ASKAPCHECK(numRanksActivated == 0, "No ranks are expected to be activated, you have: "<<numRanksActivated);
           ASKAPASSERT(numNewRankFlags == 0);
       }
   }
}

/// @brief configure local communucator and rank roles
/// @details This is the main method determining data distribution logic.
/// It uses MPI collective calls to figure out what other ranks are up to.
/// Therefore, this method should always be called on the first iteration 
/// when all ranks are expected to be active and call process()
/// @param[in] isActive true, if input is active for this rank
void ChannelMergeTask::configureRanks(bool isActive)
{
   ASKAPDEBUGASSERT(itsCommunicator == MPI_COMM_NULL);
   ASKAPDEBUGASSERT(itsRanksToMerge == itsConfig.nprocs() + 1);
   ASKAPLOG_DEBUG_STR(logger, "Initialising merge task for given data distribution and ranks available; this rank has "<<
        (isActive ? "active" : "inactive")<<" input");

   ASKAPDEBUGASSERT(itsConfig.rank() < itsConfig.nprocs());
   std::vector<int> activityFlags(itsConfig.nprocs(), 0);
   if (isActive) {
       activityFlags[itsConfig.rank()] = 1;
   }
   const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)activityFlags.data(),
              activityFlags.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);

   // now activityFlags should be consistent across all ranks - figure out the role for
   // this particular rank
   const int numInputs = std::accumulate(activityFlags.begin(), activityFlags.end(), 0);
   // 0-based sequence number of this receiving rank or -1, if it is not receiving
   const int seqNumber = isActive ? std::accumulate(activityFlags.begin(), activityFlags.begin() + itsConfig.rank(), 0) : -1;

   if (itsRanksToMerge > itsConfig.nprocs()) {
       // this is the default case meaning merge all available inputs
       itsRanksToMerge = numInputs;
   }
   const int numSpare = itsConfig.nprocs() - numInputs;
   ASKAPDEBUGASSERT(numSpare >= 0);
   ASKAPCHECK(numInputs > 0, "Merge task seems to receive no data in this ingest configuration");
   
   ASKAPLOG_DEBUG_STR(logger, "Will aggregate data handled by "<<itsRanksToMerge<<
            " consecutive active ranks");
   ASKAPCHECK(itsRanksToMerge > 1, "Number of aggregated data chunks should be more than 1!");
   ASKAPCHECK(numInputs % itsRanksToMerge == 0, "Total number of MPI ranks with data ("<<numInputs<<
               ") should be an integral multiple of selected number of ranks to merge ("<<
               itsRanksToMerge<<")");

   if (itsUseInactiveRanks) {
       const int numGroups = numInputs / itsRanksToMerge;
       if (itsConfig.rank() == 0) {
           ASKAPLOG_DEBUG_STR(logger, "Inactive ranks ("<<numSpare<<" available) will be used as much as possible for the output");
           if (numGroups < numSpare) {
               ASKAPLOG_WARN_STR(logger, "Unbalanced configuration - number of output streams ("<<numGroups<<
                        ") does not match number of spare ranks available ("<<numSpare<<")");
           }
       }
       
       // by default, assume no additional rank handles the output
       itsRankInUse = isActive;
       itsGroupWithActivatedRank = false;

       // there should be no group with colour = itsConfig.nprocs() - so this is the colour for unused group of ranks
       int colour = isActive ? seqNumber / itsRanksToMerge : itsConfig.nprocs();

       if (isActive) {
           // colour is the group number, if there is enough spare ranks it will handle the output
           if (colour < numSpare) {
               itsGroupWithActivatedRank = true;
           }
       } else {
          // figure out which group inactive ranks belong to, if any
          ASKAPDEBUGASSERT(seqNumber == -1);
          // zero-based group number this spare rank can be assigned to (we know there is already at least one group if it gets here)
          const int group = std::count(activityFlags.begin(), activityFlags.begin() + itsConfig.rank(),0);
          if (group < numGroups) {
              ASKAPLOG_DEBUG_STR(logger, "Rank "<<itsConfig.rank()<<" will be used to handle "<<group + 1<<" output stream (1-based)");
              itsGroupWithActivatedRank = true;
              itsRankInUse = true;
              colour = group;
          } else {
              ASKAPLOG_DEBUG_STR(logger, "Rank "<<itsConfig.rank()<<" will be de-activated");
          }
       }

       ASKAPLOG_DEBUG_STR(logger, "rank "<<itsConfig.rank()<<(isActive ? " active " : " inactive ")<<" seqNumber "<<seqNumber<<" colour "<<colour);

       ASKAPDEBUGASSERT(colour >= 0);
       // just set up ascending order in original ranks for local group ranks, put rank handling the output to zero
       const int response = MPI_Comm_split(MPI_COMM_WORLD, colour, isActive == itsRankInUse ? itsConfig.rank() + 1 : 0, &itsCommunicator);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_split = "<<response);

   } else {
      // inactive ranks continue to be inactive (matches behaviour before October commissioning run with additional
      // support for inactive ranks and, therefore, ability to chain them allowing tree-reduction)

      itsRankInUse = isActive;
      itsGroupWithActivatedRank = false;

      // there should be no group with colour = itsConfig.nprocs() - so this is the colour for unused group of ranks
      const int colour = isActive ? seqNumber / itsRanksToMerge : itsConfig.nprocs();

      ASKAPLOG_DEBUG_STR(logger, "rank "<<itsConfig.rank()<<(isActive ? " active " : " inactive ")<<" seqNumber "<<seqNumber<<" colour "<<colour);

      ASKAPDEBUGASSERT(colour >= 0);
      // just set up ascending order in original ranks for local group ranks
      const int response = MPI_Comm_split(MPI_COMM_WORLD, colour, itsConfig.rank(), &itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_split = "<<response);
   }

   // consistency check, argument is true for ranks being activated
   checkRanksToMerge(isActive != itsRankInUse);
}


