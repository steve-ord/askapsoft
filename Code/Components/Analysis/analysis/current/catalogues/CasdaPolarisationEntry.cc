/// @file
///
/// Implementation of the polarisation components for CASDA
///
/// @copyright (c) 2014 CSIRO
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
#include <catalogues/CasdaPolarisationEntry.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/CatalogueEntry.h>
#include <catalogues/CasdaIsland.h>
#include <catalogues/casda.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <polarisation/RMSynthesis.h>
#include <polarisation/RMData.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>

#include <vector>

ASKAP_LOGGER(logger, ".casdapolarisation");

namespace askap {

namespace analysis {

static const float defaultSNRthreshold = 8.0;
static const float defaultDebiasThreshold = 5.0;

CasdaPolarisationEntry::CasdaPolarisationEntry(CasdaComponent *comp,
                                               const LOFAR::ParameterSet &parset):
    CatalogueEntry(parset),
    itsDetectionThreshold(parset.getFloat("polThresholdSNR", defaultSNRthreshold)),
    itsDebiasThreshold(parset.getFloat("polThresholdDebias", defaultDebiasThreshold))
{

    itsRA = comp->ra();
    itsDec = comp->dec();
    itsName = comp->name();
    itsComponentID = comp->componentID();

    LOFAR::ParameterSet polParset=parset.makeSubset("RMSynthesis.");
    
    PolarisationData poldata(polParset);
    poldata.initialise(comp);

    // Do the RM Synthesis, and calculate all parameters.
    RMSynthesis rmsynth(polParset);
    rmsynth.calculate(poldata);
    RMData rmdata(polParset);
    rmdata.calculate(&rmsynth);

    casa::Unit cubeBunit=poldata.I().bunit();
    const double intFluxScale =
        casa::Quantum<float>(1.0,cubeBunit).getValue(casda::intFluxUnitContinuum);

    itsFluxImedian = poldata.I().median() * intFluxScale;
    itsFluxQmedian = poldata.Q().median() * intFluxScale;
    itsFluxUmedian = poldata.U().median() * intFluxScale;
    itsFluxVmedian = poldata.V().median() * intFluxScale;
    itsRmsI = poldata.I().medianNoise() * intFluxScale;
    itsRmsQ = poldata.Q().medianNoise() * intFluxScale;
    itsRmsU = poldata.U().medianNoise() * intFluxScale;
    itsRmsV = poldata.V().medianNoise() * intFluxScale;
    
    itsPolyCoeff0 = comp->intFlux();
    itsPolyCoeff1 = comp->alpha();
    itsPolyCoeff2 = comp->beta();
    itsPolyCoeff3 = itsPolyCoeff4 = 0.;

    itsLambdaSqRef = rmsynth.refLambdaSq();
    itsRmsfFwhm = rmsynth.rmsf_width();

    itsPintPeak.value() = rmdata.pintPeak() * intFluxScale;
    itsPintPeak.error() = rmdata.pintPeak_err() * intFluxScale;
    itsPintPeakDebias = rmdata.pintPeakEff() * intFluxScale;
    itsPintPeakFit.value() = rmdata.pintPeakFit() * intFluxScale;
    itsPintPeakFit.error() = rmdata.pintPeakFit_err() * intFluxScale;
    itsPintPeakFitDebias = rmdata.pintPeakFitEff() * intFluxScale;

    itsPintFitSNR.value() = rmdata.SNR();
    itsPintFitSNR.error() = rmdata.SNR_err();
    
    itsPhiPeak.value() = rmdata.phiPeak();
    itsPhiPeak.error() = rmdata.phiPeak_err();
    itsPhiPeakFit.value() = rmdata.phiPeakFit();
    itsPhiPeakFit.error() = rmdata.phiPeakFit_err();

    itsPolAngleRef.value() = rmdata.polAngleRef();
    itsPolAngleRef.error() = rmdata.polAngleRef_err();
    itsPolAngleZero.value() = rmdata.polAngleZero();
    itsPolAngleZero.error() = rmdata.polAngleZero_err();

    itsFracPol.value() = rmdata.fracPol();
    itsFracPol.error() = rmdata.fracPol_err();

    /// @todo
    itsComplexity = 0.;
    itsComplexity_screen = 0.;

    itsFlagDetection = rmdata.flagDetection();
    itsFlagEdge = rmdata.flagEdge();
    itsFlag3 = 0;
    itsFlag4 = 0;
}

const float CasdaPolarisationEntry::ra()
{
    return itsRA;
}

const float CasdaPolarisationEntry::dec()
{
    return itsDec;
}


void CasdaPolarisationEntry::printTableRow(std::ostream &stream,
        duchamp::Catalogues::CatalogueSpecification &columns)
{
    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i));
    }
    stream << "\n";
}


void CasdaPolarisationEntry::printTableEntry(std::ostream &stream,
        duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ID") {
        column.printEntry(stream, itsComponentID);
    } else if (type == "NAME") {
        column.printEntry(stream, itsName);
    } else if (type == "RAJD") {
        column.printEntry(stream, itsRA);
    } else if (type == "DECJD") {
        column.printEntry(stream, itsDec);
    } else if (type == "IFLUX") {
        column.printEntry(stream, itsFluxImedian);
    } else if (type == "QFLUX") {
        column.printEntry(stream, itsFluxQmedian);
    } else if (type == "UFLUX") {
        column.printEntry(stream, itsFluxUmedian);
    } else if (type == "VFLUX") {
        column.printEntry(stream, itsFluxVmedian);
    } else if (type == "RMS_I") {
        column.printEntry(stream, itsRmsI);
    } else if (type == "RMS_Q") {
        column.printEntry(stream, itsRmsQ);
    } else if (type == "RMS_U") {
        column.printEntry(stream, itsRmsU);
    } else if (type == "RMS_V") {
        column.printEntry(stream, itsRmsV);
    } else if (type == "CO1") {
        column.printEntry(stream, itsPolyCoeff0);
    } else if (type == "CO2") {
        column.printEntry(stream, itsPolyCoeff1);
    } else if (type == "CO3") {
        column.printEntry(stream, itsPolyCoeff2);
    } else if (type == "CO4") {
        column.printEntry(stream, itsPolyCoeff3);
    } else if (type == "CO5") {
        column.printEntry(stream, itsPolyCoeff4);
    } else if (type == "LAMSQ") {
        column.printEntry(stream, itsLambdaSqRef);
    } else if (type == "RMSF") {
        column.printEntry(stream, itsRmsfFwhm);
    } else if (type == "POLPEAK") {
        column.printEntry(stream, itsPintPeak.value());
    } else if (type == "POLPEAKDB") {
        column.printEntry(stream, itsPintPeakDebias);
    } else if (type == "POLPEAKERR") {
        column.printEntry(stream, itsPintPeak.error());
    } else if (type == "POLPEAKFIT") {
        column.printEntry(stream, itsPintPeakFit.value());
    } else if (type == "POLPEAKFITDB") {
        column.printEntry(stream, itsPintPeakFitDebias);
    } else if (type == "POLPEAKFITERR") {
        column.printEntry(stream, itsPintPeakFit.error());
    } else if (type == "POLPEAKFITSNR") {
        column.printEntry(stream, itsPintFitSNR.value());
    } else if (type == "POLPEAKFITSNRERR") {
        column.printEntry(stream, itsPintFitSNR.error());
    } else if (type == "FDPEAK") {
        column.printEntry(stream, itsPhiPeak.value());
    } else if (type == "FDPEAKERR") {
        column.printEntry(stream, itsPhiPeak.error());
    } else if (type == "FDPEAKFIT") {
        column.printEntry(stream, itsPhiPeakFit.value());
    } else if (type == "FDPEAKFITERR") {
        column.printEntry(stream, itsPhiPeakFit.error());
    } else if (type == "POLANG") {
        column.printEntry(stream, itsPolAngleRef.value());
    } else if (type == "POLANGERR") {
        column.printEntry(stream, itsPolAngleRef.error());
    } else if (type == "POLANG0") {
        column.printEntry(stream, itsPolAngleZero.value());
    } else if (type == "POLANG0ERR") {
        column.printEntry(stream, itsPolAngleZero.error());
    } else if (type == "POLFRAC") {
        column.printEntry(stream, itsFracPol.value());
    } else if (type == "POLFRACERR") {
        column.printEntry(stream, itsFracPol.error());
    } else if (type == "COMPLEX1") {
        column.printEntry(stream, itsComplexity);
    } else if (type == "COMPLEX2") {
        column.printEntry(stream, itsComplexity_screen);
    } else if (type == "FLAG1") {
        column.printEntry(stream, itsFlagDetection);
    } else if (type == "FLAG2") {
        column.printEntry(stream, itsFlagEdge);
    } else if (type == "FLAG3") {
        column.printEntry(stream, itsFlag3);
    } else if (type == "FLAG4") {
        column.printEntry(stream, itsFlag4);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }
}

void CasdaPolarisationEntry::checkCol(duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ID") {
        column.check(itsComponentID);
    } else if (type == "NAME") {
        column.check(itsName);
    } else if (type == "RAJD") {
        column.check(itsRA);
    } else if (type == "DECJD") {
        column.check(itsDec);
    } else if (type == "IFLUX") {
        column.check(itsFluxImedian);
    } else if (type == "QFLUX") {
        column.check(itsFluxQmedian);
    } else if (type == "UFLUX") {
        column.check(itsFluxUmedian);
    } else if (type == "VFLUX") {
        column.check(itsFluxImedian);
    } else if (type == "RMS_I") {
        column.check(itsRmsI);
    } else if (type == "RMS_Q") {
        column.check(itsRmsQ);
    } else if (type == "RMS_U") {
        column.check(itsRmsU);
    } else if (type == "RMS_V") {
        column.check(itsRmsV);
    } else if (type == "CO1") {
        column.check(itsPolyCoeff0);
    } else if (type == "CO2") {
        column.check(itsPolyCoeff1);
    } else if (type == "CO3") {
        column.check(itsPolyCoeff2);
    } else if (type == "CO4") {
        column.check(itsPolyCoeff3);
    } else if (type == "CO5") {
        column.check(itsPolyCoeff4);
    } else if (type == "LAMSQ") {
        column.check(itsLambdaSqRef);
    } else if (type == "RMSF") {
        column.check(itsRmsfFwhm);
    } else if (type == "POLPEAK") {
        column.check(itsPintPeak.value());
    } else if (type == "POLPEAKDB") {
        column.check(itsPintPeakDebias);
    } else if (type == "POLPEAKERR") {
        column.check(itsPintPeak.error());
    } else if (type == "POLPEAKFIT") {
        column.check(itsPintPeakFit.value());
    } else if (type == "POLPEAKFITDB") {
        column.check(itsPintPeakFitDebias);
    } else if (type == "POLPEAKFITERR") {
        column.check(itsPintPeakFit.error());
    } else if (type == "POLPEAKFITSNR") {
        column.check(itsPintFitSNR.value());
    } else if (type == "POLPEAKFITSNRERR") {
        column.check(itsPintFitSNR.error());
    } else if (type == "FDPEAK") {
        column.check(itsPhiPeak.value());
    } else if (type == "FDPEAKERR") {
        column.check(itsPhiPeak.error());
    } else if (type == "FDPEAKFIT") {
        column.check(itsPhiPeakFit.value());
    } else if (type == "FDPEAKFITERR") {
        column.check(itsPhiPeakFit.error());
    } else if (type == "POLANG") {
        column.check(itsPolAngleRef.value());
    } else if (type == "POLANGERR") {
        column.check(itsPolAngleRef.error());
    } else if (type == "POLANG0") {
        column.check(itsPolAngleZero.value());
    } else if (type == "POLANG0ERR") {
        column.check(itsPolAngleZero.error());
    } else if (type == "POLFRAC") {
        column.check(itsFracPol.value());
    } else if (type == "POLFRACERR") {
        column.check(itsFracPol.error());
    } else if (type == "COMPLEX1") {
        column.check(itsComplexity);
    } else if (type == "COMPLEX2") {
        column.check(itsComplexity_screen);
    } else if (type == "FLAG1") {
        column.check(itsFlagDetection);
    } else if (type == "FLAG2") {
        column.check(itsFlagEdge);
    } else if (type == "FLAG3") {
        column.check(itsFlag3);
    } else if (type == "FLAG4") {
        column.check(itsFlag4);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaPolarisationEntry::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec)
{
    for (size_t i = 0; i < spec.size(); i++) {
        this->checkCol(spec.column(i));
    }
}



}

}
