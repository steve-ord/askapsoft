/// @file
///
/// Provides utility functions for simulations package
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
///
#ifndef ASKAP_SIMS_CONT_SELAVY_H_
#define ASKAP_SIMS_CONT_SELAVY_H_

#include <modelcomponents/Spectrum.h>
#include <modelcomponents/Continuum.h>

namespace askap {

namespace analysisutilities {

/// Default value for the flag indicating whether to use the deconvolved sizes
const bool defaultDeconvFlag = false;

/// @brief A class to hold spectral information for a continuum
/// spectrum.  @details This class holds information on the continuum
/// properties of a spectral profile. The continuuum source is assumed
/// to be taken from a CASDA continuum components catalogue. The shape
/// of the source is taken from the fitted component, along with the
/// spectral index and curvature (alpha & beta).
///
/// The flux at a given frequency is given by the relation:
/// \f$F(\nu) = F(\nu_0) \left(\frac{\nu}{\nu_0}\right)^{\alpha + \beta\log(\nu/\nu_0)} \f$
class ContinuumSelavy : public Continuum {
    public:
        /// @brief Default constructor
        ContinuumSelavy(const bool flagUseDeconvolvedSizes = defaultDeconvFlag);
        /// @brief Constructor from Spectrum object
        ContinuumSelavy(const Spectrum &s, const bool flagUseDeconvolvedSizes = defaultDeconvFlag);
        /// @brief Constructor from Continuum object
        ContinuumSelavy(Continuum &c);
        /// Constructs a Continuum object from a line of
        /// text from an ascii file. Uses the ContinuumSelavy::define()
        /// function.
        ContinuumSelavy(const std::string &line,
                        const float nuZero = defaultFreq,
                        const bool flagUseDeconvolvedSizes = defaultDeconvFlag);
        /// @brief Define parameters directly
        ContinuumSelavy(const float alpha,
                        const float beta,
                        const float nuZero);

        /// @brief Define parameters directly
        ContinuumSelavy(const float alpha,
                        const float beta,
                        const float nuZero,
                        const float fluxZero);

        /// @brief Destructor
        virtual ~ContinuumSelavy() {};
        // /// @brief Copy constructor for ContinuumSelavy.
        // ContinuumSelavy(const ContinuumSelavy& f);

        // /// @brief Assignment operator for Continuum.
        // ContinuumSelavy& operator= (const ContinuumSelavy& c);
        // /// @brief Assignment operator for Continuum, using a Continuum object
        // ContinuumSelavy& operator= (const Continuum& c);
        // /// @brief Assignment operator for Continuum, using a Spectrum object
        // ContinuumSelavy& operator= (const Spectrum& c);

        /// @brief Are we using the deconvolved sizes?
        bool getFlagUseDeconvolvedSizes() {return itsFlagUseDeconvolvedSizes;};
        /// @brief Set the deconvolved size flag
        void setFlagUseDeconvolvedSizes(bool b) {itsFlagUseDeconvolvedSizes = b;};

        /// Defines a Continuum object from a line of
        /// text from a fitResults file generated by Selavy:
        ///# island_id component_id component_name ra_hms_cont dec_dms_cont ra_deg_cont dec_deg_cont ra_err dec_err freq flux_peak flux_peak_err flux_int flux_int_err maj_axis min_axis pos_ang maj_axis_err min_axis_err pos_ang_err maj_axis_deconv min_axis_deconv pos_ang_deconv chi_squared_fit rms_fit_gauss spectral_index spectral_curvature rms_image has_siblings fit_is_estimate flag_c3 flag_c4
        void define(const std::string &line);

        /// @brief Was the "fit" actually from the pre-fit estimate (ie. a guess)?
        bool isGuess() {return itsFlagEstimate;};

        void print(std::ostream& theStream);
        /// @details Prints a summary of the parameters to the stream
        /// @param theStream The destination stream
        /// @param prof The profile object
        /// @return A reference to the stream
        friend std::ostream& operator<< (std::ostream& theStream, ContinuumSelavy &cont);

    protected:
    std::string itsInput;
    std::string itsIslandID;
    std::string itsName;
    float itsRAJD;
    float itsDecJD;
    float itsRAerr;
    float itsDecErr;
    float itsFreq;
    float itsFint;
    float itsFintErr;
    float itsFpeak;
    float itsFpeakErr;
    float itsMaj;
    float itsMin;
    float itsPA;
    float itsMajErr;
    float itsMinErr;
    float itsPAerr;
    float itsMajDECONV;
    float itsMinDECONV;
    float itsPADECONV;
    float itsChisq;
    float itsRMSimage;
    float itsRMSfit;
    int itsFlagSiblings;
    int itsFlagEstimate;
    int itsFlag3;
    int itsFlag4;
    bool itsFlagUseDeconvolvedSizes;
};

}

}

#endif
