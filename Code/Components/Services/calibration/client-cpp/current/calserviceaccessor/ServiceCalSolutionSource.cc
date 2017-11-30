/// @file
/// @brief Service based implementation of the calibration solution source
/// @details This implementation is to be used with the Calibration Data Service
/// Main functionality is implemented in the corresponding ServiceCalSolutionAccessor class.
/// This class just creates an instance of the accessor and manages it.
///
/// @copyright (c) 2017 CSIRO
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
/// @author Max Voronkov <Maxim.Voronkov@csiro.au>
/// @author Stephen Ord <Stephen.Ord@csiro.au>


#include <calibaccess/ServiceCalSolutionSourceStub.h>
#include "ServiceCalSolutionSource.h"
#include "ServiceCalSolutionAccessor.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>


// logging stuff
#include <askap_accessors.h>
#include <askap/AskapLogging.h>
ASKAP_LOGGER(logger, ".calibaccess");

namespace askap {

namespace accessors {

/// @brief constructor
/// @details Creates solution source object for a given parset file
/// (whether it is for writing or reading depends on the actual methods
/// used). Also need to decide whether it is the source or the accessor that
/// creates the client. I think it should be the source then an accessor of any
/// particular type can be instaniated when required.
/// But the accessor does need a communicator....
/// how about the source instantiates the client and the accessor is instantiaed using the client
///
/// @param[in] parset parset file name
ServiceCalSolutionSource::ServiceCalSolutionSource(const LOFAR::ParameterSet &parset) : ServiceCalSolutionSourceStub(parset)
  {
  ASKAPLOG_WARN_STR(logger, "ServiceCalSolutionSource constructor - override the stub ... or just reimplement");

  // Need to generate the calibrationclient and set up all the solutions


  const string locatorHost = parset.getString("ice.locator.host");
  const string locatorPort = parset.getString("ice.locator.port");
  const string serviceName = parset.getString("calibrationdataservice.name");

  itsClient = boost::make_shared<askap::cp::caldataservice::CalibrationDataServiceClient> (locatorHost, locatorPort, serviceName);

  itsAccessor.reset(new ServiceCalSolutionAccessor(itsClient, 0, false));

}

/// @brief obtain ID for the most recent solution
/// @return ID for the most recent solution
/// @note This particular implementation doesn't support multiple
/// solutions and, therefore, always returns the same ID.
long ServiceCalSolutionSource::mostRecentSolution() const
{

  return 0;

}

/// @brief obtain solution ID for a given time
/// @details This method looks for a solution valid at the given time
/// and returns its ID. It is equivalent to mostRecentSolution() if
/// called with a time sufficiently into the future.
/// @return solution ID
///
long ServiceCalSolutionSource::solutionID(const double) const
{
  ASKAPTHROW(AskapError, "ServiceCalSolutionSource::solutionID not yet implemented");
}

/// @brief obtain read-only accessor for a given solution ID
/// @details This method returns a shared pointer to the solution accessor, which
/// can be used to read the parameters. If a solution with the given ID doesn't
/// exist, an exception is thrown. Existing solutions with undefined parameters
/// are managed via validity flags of gains, leakages and bandpasses
/// @return shared pointer to an accessor object
/// @note This particular implementation doesn't support multiple solutions and
/// always returns the same accessor (for both reading and writing)
boost::shared_ptr<ICalSolutionConstAccessor> ServiceCalSolutionSource::roSolution(const long ID) const
{

  ASKAPDEBUGASSERT(accessor());
  boost::shared_ptr<ICalSolutionAccessor> acc = boost::dynamic_pointer_cast<ICalSolutionAccessor>(accessor());
  ASKAPCHECK(acc,
     "Unable to cast solution accessor to read-write type, CalSolutionSourceStub has been initialised with an incompatible object");
  return acc;
}

/// @brief obtain a solution ID to store new solution
/// @details This method provides a solution ID for a new solution. It must
/// be called before any write operation (one needs a writable accessor to
/// write the actual solution and to get this accessor one needs an ID).
/// @param[in] time time stamp of the new solution in seconds since MJD of 0.
/// @return solution ID

long ServiceCalSolutionSource::newSolutionID(const double time)
{

  return 0;

}
/// @brief obtain a writeable accessor for a given solution ID
/// @details This method returns a shared pointer to the solution accessor, which
/// can be used to both read the parameters and write them back. If a solution with
/// the given ID doesn't exist, an exception is thrown. Existing solutions with undefined
/// parameters are managed via validity flags of gains, leakages and bandpasses
/// @return shared pointer to an accessor object
/// @note This particular implementation returns the same accessor regardless of the
/// chosen ID (for both reading and writing)
boost::shared_ptr<ICalSolutionAccessor> ServiceCalSolutionSource::rwSolution(const long ID) const
{

  ASKAPDEBUGASSERT(accessor());
  boost::shared_ptr<ICalSolutionAccessor> acc = boost::dynamic_pointer_cast<ICalSolutionAccessor>(accessor());
  ASKAPCHECK(acc,
     "Unable to cast solution accessor to read-write type, CalSolutionSourceStub has been initialised with an incompatible object");
  return acc;
}

} // accessors

} // namespace askap
