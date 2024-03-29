///
/// @file : Duchamp driver
///
///
/// Control parameters are passed in from a LOFAR ParameterSet file.
///
/// @copyright (c) 2007 CSIRO
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>

// Package level header file
#include <askap_analysis.h>

// System includes
#include <stdexcept>
#include <iostream>

// ASKAPsoft includes
#include <askap/Application.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askap/StatReporter.h>
#include <askapparallel/AskapParallel.h>
#include <parallelanalysis/DuchampParallel.h>
#include <duchamp/duchamp.hh>
#include <Common/ParameterSet.h>

using std::cout;
using std::endl;
using namespace askap;
using namespace askap::analysis;

ASKAP_LOGGER(logger, "cduchamp.log");

class DuchampApp : public askap::Application {
    public:
        virtual int run(int argc, char* argv[])
        {
            // This class must have scope outside the main try/catch block
            askap::askapparallel::AskapParallel comms(argc, const_cast<const char**>(argv));
            try {
                StatReporter stats;

                ASKAPLOG_INFO_STR(logger, "ASKAP source finder " << ASKAP_PACKAGE_VERSION);

                // Create a new parset with a nocase key compare policy, then
                // adopt the contents of the real parset
                LOFAR::ParameterSet parset(LOFAR::StringUtil::Compare::NOCASE);
                parset.adoptCollection(config());
                LOFAR::ParameterSet subset(parset.makeSubset("Cduchamp."));

                DuchampParallel duchamp(comms, subset);
                if (!comms.isParallel() || comms.isMaster()) {
                    ASKAPLOG_INFO_STR(logger, "Parset file contents:\n" << config());
                }
                duchamp.readData();
                duchamp.setupLogfile(argc, const_cast<const char**>(argv));
                duchamp.preprocess();
                duchamp.gatherStats();
                duchamp.setThreshold();
                duchamp.findSources();
                duchamp.fitSources();
                duchamp.sendObjects();
                duchamp.receiveObjects();
                duchamp.cleanup();
                duchamp.printResults();
                duchamp.extract();
                duchamp.writeToFITS();

                stats.logSummary();
                ///==============================================================================
            } catch (const askap::AskapError& x) {
                ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
                std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
                exit(1);
            } catch (const duchamp::DuchampError& x) {
                ASKAPLOG_FATAL_STR(logger, "Duchamp error in " << argv[0] << ": " << x.what());
                std::cerr << "Duchamp error in " << argv[0] << ": " << x.what() << std::endl;
                exit(1);
            } catch (const std::exception& x) {
                ASKAPLOG_FATAL_STR(logger,
                                   "Unexpected exception in " << argv[0] << ": " << x.what());
                std::cerr << "Unexpected exception in " << argv[0] << ": " <<
                          x.what() << std::endl;
                exit(1);
            }

            return 0;
        }
};

int main(int argc, char *argv[])
{
    DuchampApp app;
    return app.main(argc, argv);
}
