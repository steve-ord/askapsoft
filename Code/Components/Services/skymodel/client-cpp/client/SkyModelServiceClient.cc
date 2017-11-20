/// @file SkyModelServiceClient.cc
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
#include "SkyModelServiceClient.h"

// Include package level header file
#include "askap_smsclient.h"

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "Ice/Ice.h"
#include "iceutils/CommunicatorConfig.h"
#include "iceutils/CommunicatorFactory.h"
#include "SkyModelService.h" // Ice generated interface
#include "casacore/casa/Quanta/Quantum.h"

// Local package includes

// Using
using namespace std;
using namespace askap;
using namespace askap::cp::sms;
using namespace askap::interfaces::skymodelservice;

SkyModelServiceClient::SkyModelServiceClient(const std::string& locatorHost,
                            const std::string& locatorPort,
                            const std::string& serviceName)
{
    askap::cp::icewrapper::CommunicatorConfig config(locatorHost, locatorPort);
    config.setProperty("Ice.MessageSizeMax", "131072");
    askap::cp::icewrapper::CommunicatorFactory commFactory;
    itsComm = commFactory.createCommunicator(config);

    ASKAPDEBUGASSERT(itsComm);

    Ice::ObjectPrx base = itsComm->stringToProxy(serviceName);
    itsService = askap::interfaces::skymodelservice::ISkyModelServicePrx::checkedCast(base);

    if (!itsService) {
        ASKAPTHROW(AskapError, "SkyModelService proxy is invalid");
    }
}

SkyModelServiceClient::~SkyModelServiceClient()
{
}

vector<Component> SkyModelServiceClient::coneSearch(const casa::Quantity& ra,
        const casa::Quantity& dec, const casa::Quantity& searchRadius,
        const casa::Quantity& fluxLimit)
{
    ASKAPCHECK(ra.isConform("deg"), "ra must conform to degrees");
    ASKAPCHECK(dec.isConform("deg"), "dec must conform to degrees");
    ASKAPCHECK(searchRadius.isConform("deg"), "searchRadius must conform to degrees");
    ASKAPCHECK(fluxLimit.isConform("Jy"), "fluxLimit must conform to Jy");

    // ICE API method signature:
    // ComponentSeq coneSearch(Coordinate centre, double radius, SearchCriteria criteria);

    Coordinate centre;
    centre.rightAscension = ra.getValue("deg");
    centre.declination = dec.getValue("deg");

    SearchCriteria criteria;
    criteria.maxFluxInt = fluxLimit.getValue("mJy");

    askap::interfaces::skymodelservice::ComponentSeq ice_resultset =
        itsService->coneSearch(
            centre,
            searchRadius.getValue("deg"),
            criteria);

    // Temporary code: return an empty result set
    return vector<Component>();
}
