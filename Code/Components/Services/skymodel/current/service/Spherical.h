/// @file Spherical.h
/// @brief Spherical functions outside of the primary data service.
///
/// @copyright (c) 2016 CSIRO
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
/// @author Daniel Collins <daniel.collins@csiro.au>

#ifndef ASKAP_CP_SMS_SPHERICAL_H
#define ASKAP_CP_SMS_SPHERICAL_H

// ASKAPsoft includes
#include <boost/noncopyable.hpp>
#include <Common/ParameterSet.h>

// Local package includes


namespace askap {
namespace cp {
namespace sms {


class Spherical : private boost::noncopyable {
    public:
        /// @brief Constructor.
        ///
        /// @param order
        Spherical(long order);

        /// @brief Calculate the HEALPix index for a given RA and declination.
        ///
        /// @note   I will probably require a vectorisable version of this function
        ///         that operates on a vector of input coordinates, but this function
        ///         will suffice for the initial unit tests.
        /// @param[in] ra  J2000 right ascension (decimal degrees)
        /// @param[in] dec J2000 declination (decimal degrees)
        long calcHealPixIndex(double ra, double dec) const;

    private:
        long itsNSide;
};

};
};
};

#endif
