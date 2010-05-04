/// @file MSSink.h
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

#ifndef ASKAP_CP_MSSINK_H
#define ASKAP_CP_MSSINK_H

// ASKAPsoft includes
#include "boost/shared_ptr.hpp"
#include "Common/ParameterSet.h"

// Local package includes
#include "ingestpipeline/ITask.h"
#include "ingestpipeline/datadef/VisChunk.h"

namespace askap {
namespace cp {

class MSSink : public askap::cp::ITask {
    public:
        MSSink(const LOFAR::ParameterSet& parset);
        virtual ~MSSink();

        virtual void process(VisChunk::ShPtr chunk);

    private:
        const LOFAR::ParameterSet itsParset;

};

}
}

#endif
