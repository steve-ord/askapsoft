/// @file
///
/// Continuum source from the NVSS catalogue, using the full content
/// as obtained from CDS, with ascii text/plain option
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
///
#include <askap_analysisutilities.h>

#include <modelcomponents/Spectrum.h>
#include <modelcomponents/Continuum.h>
#include <modelcomponents/ContinuumNVSS.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <utility>
#include <string>
#include <stdlib.h>
#include <math.h>

ASKAP_LOGGER(logger, ".ContNVSS");

namespace askap {

namespace analysisutilities {

ContinuumNVSS::ContinuumNVSS():
    Continuum()
{
    this->defineSource(0., 0., 1400.);
}

ContinuumNVSS::ContinuumNVSS(Spectrum &s):
    Continuum(s)
{
    this->defineSource(0., 0., 1400.);
}

ContinuumNVSS::ContinuumNVSS(std::string &line)
{
    this->define(line);
}

void ContinuumNVSS::define(const std::string &line)
{
    this->itsInputLine      = line;
    this->itsRadius         = atof(line.substr(0, 9).c_str());
    this->itsXoff           = atof(line.substr(9, 10).c_str());
    this->itsYoff           = atof(line.substr(19, 10).c_str());
    this->itsRecno          = atoi(line.substr(30, 8).c_str());
    this->itsField          = line.substr(38, 8);
    this->itsFieldXpos      = atof(line.substr(47, 7).c_str());
    this->itsFieldYpos      = atof(line.substr(55, 7).c_str());
    this->itsName           = line.substr(63, 14);
    this->itsRAstring       = line.substr(78, 11);
    this->itsDecstring      = line.substr(90, 11);
    this->itsRA_err         = atof(line.substr(102, 5).c_str());
    this->itsDec_err        = atof(line.substr(107, 4).c_str());
    this->itsS1400          = atof(line.substr(113, 8).c_str());
    this->itsS1400_err      = atof(line.substr(122, 7).c_str());
    this->itsMajorAxisLimit = line[130];
    this->itsMajorAxis      = atof(line.substr(132, 5).c_str());
    this->itsMinorAxisLimit = line[138];
    this->itsMinorAxis      = atof(line.substr(140, 5).c_str());
    this->itsPA_input       = atof(line.substr(146, 5).c_str());
    this->itsMajorAxis_err  = atof(line.substr(152, 4).c_str());
    this->itsMinorAxis_err  = atof(line.substr(157, 4).c_str());
    this->itsPA_err         = atof(line.substr(161, 4).c_str());
    this->itsFlagResidual   = line.substr(167, 2);
    this->itsResidualFlux   = atoi(line.substr(170, 4).c_str());
    this->itsPolFlux        = atof(line.substr(175, 6).c_str());
    this->itsPolPA          = atof(line.substr(182, 5).c_str());
    this->itsPolFlux_err    = atof(line.substr(188, 5).c_str());
    this->itsPolPA_err      = atof(line.substr(194, 4).c_str());

    this->itsRA = this->itsRAstring;
    for (size_t i = 0; i < this->itsRA.size(); i++) {
        if (this->itsRA[i] == ' ') {
            this->itsRA[i] = ':';
        }
    }
    this->itsDec = this->itsDecstring;
    for (size_t i = 0; i < this->itsDec.size(); i++) {
        if (this->itsDec[i] == ' ') {
            this->itsDec[i] = ':';
        }
    }

    this->itsID = this->itsName;

    this->itsFlux = this->itsS1400 / 1.e3; // put into Jy
    this->itsMaj = this->itsMajorAxisLimit == '<' ? 0. : this->itsMajorAxis;
    this->itsMin = this->itsMinorAxisLimit == '<' ? 0. : this->itsMinorAxis;
    this->itsPA = this->itsPA_input;

    this->itsAlpha = 0.;
    this->itsBeta = 0.;

    this->checkShape();

}

ContinuumNVSS::ContinuumNVSS(const ContinuumNVSS& c):
    Continuum(c)
{
    operator=(c);
}

ContinuumNVSS& ContinuumNVSS::operator= (const ContinuumNVSS& c)
{
    if (this == &c) return *this;

    ((Continuum &) *this) = c;
    this->itsAlpha      = c.itsAlpha;
    this->itsBeta       = c.itsBeta;
    this->itsNuZero     = c.itsNuZero;
    return *this;
}

ContinuumNVSS& ContinuumNVSS::operator= (const Spectrum& c)
{
    if (this == &c) return *this;

    ((Continuum &) *this) = c;
    this->defineSource(0., 0., 1400.);
    return *this;
}


void ContinuumNVSS::print(std::ostream& theStream)
{
    theStream << this->itsInputLine << "\n";
    // theStream.setf(std::ios::showpoint);
    // theStream << std::setw(11) << this->itsComponentNum << " "
    //    << std::setw(9) << this->itsGalaxyNum << " "
    //    << std::setw(9)  << this->itsStructure << " "
    //    << std::setw(15) << std::setprecision(5) << this->itsRA << " "
    //    << std::setw(11) << std::setprecision(5) << this->itsDec << " "
    //    << std::setw(14) << std::setprecision(3) << this->itsPA() << " "
    //    << std::setw(10) << std::setprecision(3) << this->itsMaj() << " "
    //    << std::setw(10) << std::setprecision(3) << this->itsMin() << " "
    //    << std::setw(7) << std::setprecision(4) << this->itsI151 << " "
    //    << std::setw(7) << std::setprecision(4) << this->itsI610 << " "
    //    << std::setw(7) << std::setprecision(4) << this->itsI1400 << " "
    //    << std::setw(7) << std::setprecision(4) << this->itsI4860 << " "
    //    << std::setw(7) << std::setprecision(4) << this->itsI18000 << "\n";
}
std::ostream& operator<< (std::ostream& theStream, ContinuumNVSS &cont)
{
    cont.print(theStream);
    return theStream;
}

void ContinuumNVSS::printDetails(std::ostream& theStream)
{
    theStream << "radius = " << this->itsRadius
              << "\nXoff = " << this->itsXoff
              << "\nYoff = " << this->itsYoff
              << "\nRecno = " << this->itsRecno
              << "\nField = " << this->itsField
              << "\nXpos = " << this->itsFieldXpos
              << "\nYpos = " << this->itsFieldYpos
              << "\nName = " << this->itsName
              << "\nRA = " << this->itsRAstring << " +- " << this->itsRA_err
              << "\nDec = " << this->itsDecstring << " +- " << this->itsDec_err
              << "\nFlux = " << this->itsS1400 << " +- " << this->itsS1400_err
              << "\nMajor axis = " << this->itsMajorAxisLimit << " "
              << this->itsMajorAxis << " +- " << this->itsMajorAxis_err
              << "\nMinor axis = " << this->itsMinorAxisLimit << " "
              << this->itsMinorAxis << " +- " << this->itsMinorAxis_err
              << "\nPA = " << this->itsPA << " +- " << this->itsPA_err
              << "\nResidual = " << this->itsFlagResidual << " " << this->itsResidualFlux
              << "\nPol flux = " << this->itsPolFlux << " +- " << this->itsPolFlux_err
              << "\nPol PA = " << this->itsPolPA << " +- " << this->itsPolPA_err
              << "\n"
              << "\nRA = " << this->itsRA
              << "\nDec = " << this->itsDec
              << "\n";
}

}


}
