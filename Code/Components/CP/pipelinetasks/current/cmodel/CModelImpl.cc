/// @file CModelImpl.cc
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "cmodel/CModelImpl.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <cmath>
#include <string>
#include <vector>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/scoped_ptr.hpp"
#include "Common/ParameterSet.h"
#include "skymodelclient/Component.h"

// Casacore includes
#include "casa/aipstype.h"
#include "casa/Quanta/Quantum.h"

// Local package includes
#include "cmodel/DuchampAccessor.h"
#include "cmodel/DataserviceAccessor.h"
#include "cmodel/CasaWriter.h"
#include "cmodel/ParsetUtils.h"

// Using
using namespace askap;
using namespace askap::cp::pipelinetasks;
using namespace casa;

ASKAP_LOGGER(logger, ".CModelImpl");

CModelImpl::CModelImpl(const LOFAR::ParameterSet& itsParset)
        : itsParset(itsParset)
{
}

void CModelImpl::run(void)
{
    // Interface to GSM data
    boost::scoped_ptr<IGlobalSkyModel> gsm;

    // Setup the GSM accessor
    const std::string database = itsParset.getString("gsm.database");

    if (database == "duchamp") {
        const std::string filename = itsParset.getString("gsm.file");
        gsm.reset(new DuchampAccessor(filename));
    } else if (database == "dataservice") {
        const std::string host = itsParset.getString("gsm.locator_host");
        const std::string port = itsParset.getString("gsm.locator_port");
        const std::string serviceName = itsParset.getString("gsm.service_name");
        gsm.reset(new DataserviceAccessor(host, port, serviceName));
    } else {
        ASKAPTHROW(AskapError, "Unknown GSM database type: " << database);
    }

    // Get the flux limit
    const std::string fluxLimitStr = itsParset.getString("flux_limit");
    const Quantity fluxLimit = ParsetUtils::asQuantity(fluxLimitStr, "Jy");

    // Get the centre of the image
    const std::vector<std::string> dirVector = itsParset.getStringVector("direction");
    const Quantity ra = ParsetUtils::asQuantity(dirVector.at(0), "deg");
    const Quantity dec = ParsetUtils::asQuantity(dirVector.at(1), "deg");

    // Detrmine the search radius
    // At the moment just use the 1D size of the image multiplied by the
    // cellsize to determine the search radius. Because the dimensions or scale
    // may not be identical, use the larger of the two. This is almost 2x the
    // field, but given the current implementations of cone search do not
    // include extended components with centre outside the field, it is best
    // to search a larger radius anyway.
    // TODO: When coneSearch implementations are fixed, restrict the radius
    // to only the required radius.
    const casa::uInt nx = itsParset.getUintVector("shape").at(0);
    const casa::uInt ny = itsParset.getUintVector("shape").at(1);
    const casa::uInt maxNPix = std::max(nx, ny);
    const std::vector<std::string> cellSizeVector = itsParset.getStringVector("cellsize");
    const Quantity xcellsize = ParsetUtils::asQuantity(cellSizeVector.at(0), "arcsec");
    const Quantity ycellsize = ParsetUtils::asQuantity(cellSizeVector.at(1), "arcsec");
    const casa::Double largestCellSize = std::max(xcellsize.getValue("deg"), ycellsize.getValue("deg"));
    const casa::Quantity searchRadius(largestCellSize * maxNPix, "deg");

    const std::vector<askap::cp::skymodelservice::Component> list = gsm->coneSearch(ra, dec, searchRadius, fluxLimit);
    gsm.reset(0);
    ASKAPLOG_INFO_STR(logger, "Number of components in result set: " << list.size());

    CasaWriter writer(itsParset);
    writer.write(list);
}
