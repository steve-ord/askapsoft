/// @file CubeMakerHelperFunctions.cc
///
/// @copyright (c) 2013 CSIRO
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///

// Include own header file first
#include <makecube/CubeMakerHelperFunctions.h>

// Include package level header file
#include <askap_pipelinetasks.h>

// System includes
#include <vector>
#include <string>
#include <cstdlib>
#include <limits>

// ASKAPsoft includes
#include <askap/askap/AskapError.h>
#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapUtil.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/SpectralCoordinate.h>
#include <casacore/casa/Arrays/Vector.h>
#include <boost/regex.hpp>

ASKAP_LOGGER(logger, ".CubeMakerHelper");

namespace askap {
namespace cp {
namespace pipelinetasks {

std::vector<std::string> CubeMakerHelperFunctions::expandPattern(const std::string& pattern)
{
    // Find the prefix (i.e. "image.i.") in the above example
    const size_t openBracketPos = pattern.find_first_of("[");
    if (openBracketPos == string::npos) {
        ASKAPTHROW(AskapError, "Could not find [ in valid range expression");
    }
    const std::string prefix = pattern.substr(0, openBracketPos);

    // Find the suffix (i.e. ".spectral") in the above example
    const size_t closeBracketPos = pattern.find_first_of("]");
    if (closeBracketPos == string::npos) {
        ASKAPTHROW(AskapError, "Could not find ] in valid range expression");
    }
    const std::string suffix = pattern.substr(closeBracketPos + 1, pattern.length());

    // Find the [n..n] pattern
    const boost::regex expr(".*\\[([\\d]+)\\.\\.([\\d]+)\\].*");
    boost::smatch what;
    int begin = -1;
    int end = -1;
    if (regex_match(pattern, what, expr)) {
        begin = utility::fromString<int>(what[1]);
        end = utility::fromString<int>(what[2]);
    } else {
        ASKAPTHROW(AskapError, "Could not find range expression");
    }

    std::vector<std::string> filenames;

    for (int i = begin; i <= end; ++i) {
        std::stringstream ss;
        ss << prefix << i << suffix;
        filenames.push_back(ss.str());
    }

    return filenames;
}

bool CubeMakerHelperFunctions::compatibleCoordinates(
                           const casacore::CoordinateSystem& c1,
                           const casacore::CoordinateSystem& c2)
{
    return ((c1.nCoordinates() == c2.nCoordinates())
            && c1.type() == c2.type()
            && c1.nPixelAxes() == c2.nPixelAxes()
            && c1.nWorldAxes() == c2.nWorldAxes()
            && c1.findCoordinate(casacore::Coordinate::SPECTRAL) == c2.findCoordinate(casacore::Coordinate::SPECTRAL)
            && c1.findCoordinate(casacore::Coordinate::STOKES) == c2.findCoordinate(casacore::Coordinate::STOKES)
            && c1.findCoordinate(casacore::Coordinate::DIRECTION) == c2.findCoordinate(casacore::Coordinate::DIRECTION));
}

void CubeMakerHelperFunctions::assertValidCoordinates(const casacore::CoordinateSystem& csys)
{
    const int whichSpectral = csys.findCoordinate(casacore::Coordinate::SPECTRAL);
    ASKAPCHECK(whichSpectral > -1,
               "No spectral coordinate present in the coordinate system of the first image.");

    const casacore::Vector<casacore::Int> axesSpectral = csys.pixelAxes(whichSpectral);
    ASKAPCHECK(axesSpectral.nelements() == 1, "Spectral axis " << whichSpectral
               << " is expected to correspond to just one pixel axes, you have "
               << axesSpectral);
}

double CubeMakerHelperFunctions::getChanFreq(const casacore::CoordinateSystem& csys)
{
    assertValidCoordinates(csys);
    const int whichSpectral = csys.findCoordinate(casacore::Coordinate::SPECTRAL);
    const casacore::Vector<casacore::Int> axesSpectral = csys.pixelAxes(whichSpectral);

    casacore::SpectralCoordinate freq(csys.spectralCoordinate(whichSpectral));
    double chanFreq;
    freq.toWorld(chanFreq, 0.0);
    return chanFreq;
}

double CubeMakerHelperFunctions::getFreqIncrement(
                        const casacore::CoordinateSystem& c1,
                        const casacore::CoordinateSystem& c2)
{
    return getChanFreq(c2) - getChanFreq(c1);
}

casacore::CoordinateSystem CubeMakerHelperFunctions::makeCoordinates(
                                       const casacore::CoordinateSystem& c1,
                                       const casacore::CoordinateSystem& c2,
                                       const casacore::IPosition& refShape)
{
    assertValidCoordinates(c1);
    assertValidCoordinates(c2);
    const int whichSpectral = c1.findCoordinate(casacore::Coordinate::SPECTRAL);

    const casacore::Vector<casacore::Int> axesSpectral = c1.pixelAxes(whichSpectral);
    ASKAPASSERT(casacore::uInt(axesSpectral[0]) < refShape.nelements());

    // Copy and update the spectral coordinate
    casacore::SpectralCoordinate freq(c1.spectralCoordinate(whichSpectral));
    freq.setReferencePixel(casacore::Vector<double>(1, 0.0));
    freq.setReferenceValue(casacore::Vector<double>(1, getChanFreq(c1)));
    const double freqdelt = getFreqIncrement(c1, c2);
    if (freqdelt < std::numeric_limits<double>::epsilon()) {
        ASKAPLOG_ERROR_STR(logger, "Frequency increment is zero - Spectral coordinate will be invalid");
    }
    freq.setIncrement(casacore::Vector<double>(1, freqdelt));

    // Build the coordinate system
    casacore::CoordinateSystem csys;
    for (casacore::uInt axis = 0; axis < c1.nCoordinates(); ++axis) {
        if (c1.type(axis) != casacore::Coordinate::SPECTRAL) {
            csys.addCoordinate(c1.coordinate(axis));
        } else {
            csys.addCoordinate(freq);
        }
    }

    return csys;
}

}
}
}
