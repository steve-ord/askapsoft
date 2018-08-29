/// @file BeamLogger.h
///
/// Class to log the restoring beams of individual channels of a spectral cube
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///

#include <imageaccess/BeamLogger.h>

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askapparallel/AskapParallel.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Quanta/Quantum.h>
#include <Common/ParameterSet.h>
#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>

// Local package includeas
#include <imageaccess/CasaImageAccess.h>
ASKAP_LOGGER(logger, ".BeamLogger");

namespace askap {
namespace accessors {

BeamLogger::BeamLogger():
    itsFilename("")
{
}

BeamLogger::BeamLogger(const LOFAR::ParameterSet &parset):
    itsFilename(parset.getString("beamLog", ""))
{
}

BeamLogger::BeamLogger(const std::string &filename):
    itsFilename(filename)
{
}

void BeamLogger::extractBeams(const std::vector<std::string>& imageList)
{
    itsBeamList.clear();
    std::vector<std::string>::const_iterator image = imageList.begin();

    unsigned int chan = 0;
    for (; image != imageList.end(); image++) {
        CasaImageAccess ia;
        itsBeamList[chan] = ia.beamInfo(*image);
        chan++;
    }
}

void BeamLogger::write()
{
    if (itsFilename != "") {

        std::ofstream fout(itsFilename.c_str());
        fout << "#Channel BMAJ[arcsec] BMIN[arcsec] BPA[deg]\n";

        std::map<unsigned int, casa::Vector<casa::Quantum<double> > >::iterator beam = itsBeamList.begin();
        for (; beam != itsBeamList.end(); beam++) {
            fout << beam->first << " "
                 << beam->second[0].getValue("arcsec") << " "
                 << beam->second[1].getValue("arcsec") << " "
                 << beam->second[2].getValue("deg") << "\n";
        }

    } else {
        ASKAPLOG_WARN_STR(logger,
                          "BeamLogger cannot write the log, as no filename has been specified");
    }
}

void BeamLogger::read()
{
    itsBeamList.clear();

    if (itsFilename != "") {

        std::ifstream fin(itsFilename.c_str());

        if (fin.is_open()) {

            unsigned int chan;
            double bmaj, bmin, bpa;
            std::string line, name;

            while (getline(fin, line),
                    !fin.eof()) {
                if (line[0] != '#') {
                    std::stringstream ss(line);
                    ss >> chan >> bmaj >> bmin >> bpa;
                    casa::Vector<casa::Quantum<double> > currentbeam(3);
                    currentbeam[0] = casa::Quantum<double>(bmaj, "arcsec");
                    currentbeam[1] = casa::Quantum<double>(bmin, "arcsec");
                    currentbeam[2] = casa::Quantum<double>(bpa, "deg");
                    itsBeamList[chan] = currentbeam;
                }
            }

        } else {
            ASKAPLOG_ERROR_STR(logger,
                               "Beam log file " << itsFilename << " could not be opened.");
        }

    }

}

void BeamLogger::gather(askapparallel::AskapParallel &comms, int rankToGather)
{
    if (comms.isParallel()) {

        for (int rank = 0; rank < comms.nProcs(); rank++) {

            if (rank != rankToGather) {
                if (rank == comms.rank()) {
                    // send to desired rank
                    LOFAR::BlobString bs;
                    bs.resize(0);
                    LOFAR::BlobOBufString bob(bs);
                    LOFAR::BlobOStream out(bob);
                    out.putStart("gatherBeam", 1);
                    unsigned int size = itsBeamList.size();
                    out << size;
                    if (itsBeamList.size() > 0) {
                        std::map<unsigned int, casa::Vector<casa::Quantum<double> > >::iterator beam = itsBeamList.begin();
                        for (; beam != itsBeamList.end(); beam++) {
                            out << beam->first
                                << beam->second[0].getValue("arcsec")
                                << beam->second[1].getValue("arcsec")
                                << beam->second[2].getValue("deg");
                        }
                    }
                    out.putEnd();
                    comms.sendBlob(bs, rankToGather);
                } else {
                    LOFAR::BlobString bs;
                    bs.resize(0);
                    comms.receiveBlob(bs, rank);
                    LOFAR::BlobIBufString bib(bs);
                    LOFAR::BlobIStream in(bib);
                    int version = in.getStart("gatherBeam");
                    ASKAPASSERT(version == 1);
                    unsigned int size, chan;
                    double bmaj, bmin, bpa;
                    in >> size;
                    if (size > 0) {
                        in >> chan >> bmaj >> bmin >> bpa;
                        casa::Vector<casa::Quantum<double> > currentbeam(3);
                        currentbeam[0] = casa::Quantum<double>(bmaj, "arcsec");
                        currentbeam[1] = casa::Quantum<double>(bmin, "arcsec");
                        currentbeam[2] = casa::Quantum<double>(bpa, "deg");
                        itsBeamList[chan] = currentbeam;
                    }
                    in.getEnd();
                }
            }

        }

    }

}


}
}

