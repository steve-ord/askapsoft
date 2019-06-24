/// @file TosMetadata.h
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

#ifndef ASKAP_CP_TOSMETADATA_H
#define ASKAP_CP_TOSMETADATA_H

// System includes
#include <vector>
#include <string>
#include <map>

// ASKAPsoft includes
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta.h"
#include "casacore/measures/Measures/MDirection.h"

// Local package includes
#include "cpcommon/TosMetadataAntenna.h"

// for serialisation
#include "Blob/BlobOStream.h"
#include "Blob/BlobIStream.h"

namespace askap {
namespace cp {

/// @brief This class encapsulates the dataset which comes from the
/// Telescope Operating System (TOS) for each correlator integration
/// cycle.
class TosMetadata {
    public:
        /// @brief Constructor
        TosMetadata();

        /// @brief copy constructor
        /// @details It is needed due to reference semantics of casa arrays
        /// @param[in] other an object to copy from
        TosMetadata(const TosMetadata &other);

        /// @brief assignment operator
        /// @details It is needed due to reference semantics of casa arrays
        /// @param[in] other an object to copy from
        TosMetadata& operator=(const TosMetadata &other);

        /////////////////////
        // Getters
        /////////////////////

        /// @brief Return the number of antennas.
        /// @return the number of antennas.
        casacore::uInt nAntenna(void) const;

        /// @brief Return the integration cycle start time.
        ///
        /// @return the integration cycle start time. This is an
        ///     absolute time expressed as microseconds since MJD=0.
        casacore::uLong time(void) const;

        /// @brief Get the Scan ID. Valid values are:
        casacore::Int scanId(void) const;

        /// @brief Get the FLAG which indicates the entire integration should
        /// be flagged.
        casacore::Bool flagged(void) const;

        /// @return the centre frequency
        casacore::Quantity centreFreq(void) const;

        /// @return a string describing the target
        std::string targetName(void) const;

        /// @return the target dish pointing direction
        casacore::MDirection targetDirection(void) const;

        /// @return the phase centre
        casacore::MDirection phaseDirection(void) const;

        /// @return the correlator mode
        std::string corrMode(void) const;

        /// @return the reference to 2xnBeam beam offset matrix
        /// @note the current design/metadata datagram assumes same beam offsets
        /// for all antennas. Also in some special modes we set these offsets to zero
        /// regardless of the actual beam pointings. Values are in radians (although this
        /// class doesn't rely on particular units and passes whatever value was set.
        const casacore::Matrix<casacore::Double>& beamOffsets() const;

        /////////////////////
        // Setters
        /////////////////////

        /// @brief Set the integration cycle start time.
        /// @param[in] time the integration cycle start time. This is
        ///     an absolute time expressed as microseconds since MJD=0.
        void time(const casacore::uLong time);

        /// @brief Set the Scan ID. Valid values are:
        /// * -1 - Which indicates no observation is executing
        /// * > 0 - The scan ID.
        void scanId(const casacore::Int id);

        /// @brief Set the FLAG which indicates the entire integration should
        /// be flagged.
        void flagged(const casacore::Bool flag);

        /// @brief Set the centre frequency
        void centreFreq(const casacore::Quantity& freq);

        /// @brief Set the target name. (i.e. a string describing the target)
        void targetName(const std::string& name);

        /// @brief Set the target dish pointing direction
        void targetDirection(const casacore::MDirection& dir);

        /// @brief Set the phase centre
        void phaseDirection(const casacore::MDirection& dir);

        /// @brief Set the correlator mode
        void corrMode(const std::string& mode);

        /// @brief Set beam offsets
        /// @param[in] offsets 2xnBeam beam offsets matrix 
        void beamOffsets(const casacore::Matrix<casacore::Double> &offsets);
        

        /////////////////////////
        // Antenna access methods
        /////////////////////////

        /// @brief Add an antenna to the metadata.
        /// This method is used by the caller to build a complete
        /// TosMetadata object.
        ///
        /// @param[in] name the name of the antenna to add.
        ///
        /// @throw AskapError if an antenna with this name already
        ///     exists.
        void addAntenna(const TosMetadataAntenna& ant);

        /// @brief Returns a vector of antenna names
        std::vector<std::string> antennaNames(void) const;

        /// @brief Return a const reference to the specified antenna.
        ///
        /// @param[in] id the identity of the antenna to be returned.
        ///
        /// @throw AskapError if the antenna ID is not valid.
        /// @return a const reference to the antenna specified by the
        ///     id parameter.
        const TosMetadataAntenna& antenna(const std::string& name) const;

    private:

        // Integration cycle start time.
        casacore::uLong itsTime;

        // Scan ID
        casacore::Int itsScanId;

        // Indicates this integration (as indicated by the timestamp) should be flagged
        // in its entirety
        casacore::Bool itsFlagged;

        // The centre frequency
        casacore::Quantity itsCentreFreq;

        // Target name
        std::string itsTargetName;

        // The target dish pointing direction
        casacore::MDirection itsTargetDirection;

        // The phase centre
        casacore::MDirection itsPhaseDirection;

        // The correlator mode
        std::string itsCorrMode;

        // Beam offsets (2xnBeam matrix)
        casacore::Matrix<casacore::Double> itsBeamOffsets;

        // Map of antenna names to TosMetadataAntenna objects.
        std::map<std::string, TosMetadataAntenna> itsAntennas;
};

}
}

// serialisation
namespace LOFAR {

        /// serialise TosMetadata
        /// @param[in] os output stream
        /// @param[in] obj object to serialise
        /// @return output stream
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const askap::cp::TosMetadata& obj);

        /// deserialise TosMetadata
        /// @param[in] is input stream
        /// @param[out] obj object to deserialise
        /// @return input stream
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, askap::cp::TosMetadata& obj);
}


#endif
