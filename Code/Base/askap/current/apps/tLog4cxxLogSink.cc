/// @copyright (c) 2008 CSIRO
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

// System includes
#include <iostream>
#include <cstring>

// ASKAPsoft includes
#include <askap_askap.h>
#include <askap/askap/AskapLogging.h>
#include <askap/askap/Log4cxxLogSink.h>
#include <casacore/casa/Logging/LogIO.h>

using namespace askap;

int main()
{
  ASKAPLOG_INIT("tLog4cxxLogSink.log_cfg");

  casacore::LogSinkInterface* globalSink = new Log4cxxLogSink();
  casacore::LogSink::globalSink (globalSink);
  casacore::LogIO logger;
  logger << casacore::LogOrigin("tLog4cxxLogSink") << casacore::LogIO::DEBUGGING
	 << "debug message" << casacore::LogIO::POST;
  logger << casacore::LogOrigin("tLog4cxxLogSink") << casacore::LogIO::NORMAL
	 << "info message" << casacore::LogIO::POST;
  logger << casacore::LogOrigin("tLog4cxxLogSink") << casacore::LogIO::WARN
	 << "warning message" << casacore::LogIO::POST;
  logger << casacore::LogOrigin("tLog4cxxLogSink") << casacore::LogIO::SEVERE
	 << "error message" << casacore::LogIO::POST;
  return 0;
}
