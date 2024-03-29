/// @file
///
/// XXX Notes on program XXX
///
/// @copyright (c) 2011 CSIRO
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
/// @author XXX XXX <XXX.XXX@csiro.au>
///
#ifndef ASKAP_ANALYSIS_EXTRACTION_MOMENT_MAP_H_
#define ASKAP_ANALYSIS_EXTRACTION_MOMENT_MAP_H_
#include <askap_analysis.h>

#include <map>
#include <string>

#include <extraction/SourceDataExtractor.h>
#include <Common/ParameterSet.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/IPosition.h>

namespace askap {

namespace analysis {


class MomentMapExtractor : public SourceDataExtractor {
    public:
        MomentMapExtractor() {};
        MomentMapExtractor(const LOFAR::ParameterSet& parset);
        virtual ~MomentMapExtractor() {};

        void extract();
        void writeImage();

    casa::Array<float> mom0() { return itsMom0map; };
    casa::Array<float> mom1() { return itsMom1map; };
    casa::Array<float> mom2() { return itsMom2map; };

    casa::LogicalArray mom0mask() { return itsMom0mask; };
    casa::LogicalArray mom1mask() { return itsMom1mask; };
    casa::LogicalArray mom2mask() { return itsMom2mask; };

    protected:
        void defineSlicer();
        casa::IPosition arrayShape();
        void initialiseArray();
        std::string outfile(int moment);
        double getSpectralIncrement();
        double getSpectralIncrement(int z);
        double getSpecVal(int z);
        void getMom0(const casa::Array<Float> &subarray);
        void getMom1(const casa::Array<Float> &subarray);
        void getMom2(const casa::Array<Float> &subarray);

        /// What sort of cutout to do - full field or box around the source?
        std::string itsSpatialMethod;

        /// For the box method, how many pixels to pad around the source?
        unsigned int itsPadSize;

        /// Use just the detected pixels for the calculation?
        bool itsFlagUseDetection;

        /// List of moments to calculate
        std::map<int, bool> itsMomentRequest;
        /// Array containing the moment-0 map
        casa::Array<Float> itsMom0map;
        casa::LogicalArray itsMom0mask;
        /// Array containing the moment-1 map
        casa::Array<Float> itsMom1map;
        casa::LogicalArray itsMom1mask;
        /// Array containing the moment-2 map
        casa::Array<Float> itsMom2map;
        casa::LogicalArray itsMom2mask;

};

}

}

#endif
