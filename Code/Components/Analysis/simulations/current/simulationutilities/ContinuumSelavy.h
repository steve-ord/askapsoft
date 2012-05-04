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

#include <simulationutilities/Spectrum.h>
#include <simulationutilities/Continuum.h>

namespace askap {

    namespace simulations {

        /// @brief A class to hold spectral information for a
        /// continuum spectrum.  @details This class holds information
        /// on the continuum properties of a spectral profile. The
        /// continuuum source is assumed to be taken from a fitResults
        /// file generated by Selavy/Cduchamp. The shape of the source
        /// is taken from the fitted component, along with the
        /// spectral index and curvature (alpha & beta).
        ///
        /// The flux at a given frequency is given by the relation:
        /// \f$F(\nu) = F(\nu_0) \left(\frac{\nu}{\nu_0}\right)^{\alpha + \beta\log(\nu/\nu_0)} \f$
        class ContinuumSelavy : public Continuum {
            public:
                /// @brief Default constructor
                ContinuumSelavy();
                /// @brief Constructor from Spectrum object
                ContinuumSelavy(Spectrum &s);
                /// @brief Constructor from Continuum object
                ContinuumSelavy(Continuum &c);
                /// @brief Set up parameters using a line of input from an ascii file
                ContinuumSelavy(std::string &line);
                /// @brief Define parameters directly
                ContinuumSelavy(float alpha, float beta, float nuZero) {defineSource(alpha, beta, nuZero);};
                /// @brief Define parameters directly
                ContinuumSelavy(float alpha, float beta, float nuZero, float fluxZero) {defineSource(alpha, beta, nuZero); setFluxZero(fluxZero);};
                /// @brief Destructor
                virtual ~ContinuumSelavy() {};
                /// @brief Copy constructor for ContinuumSelavy.
                ContinuumSelavy(const ContinuumSelavy& f);

                /// @brief Assignment operator for Continuum.
                ContinuumSelavy& operator= (const ContinuumSelavy& c);
                /// @brief Assignment operator for Continuum, using a Continuum object
                ContinuumSelavy& operator= (const Continuum& c);
                /// @brief Assignment operator for Continuum, using a Spectrum object
                ContinuumSelavy& operator= (const Spectrum& c);

		/// @brief Define using a line of input from an ascii file
		void define(const std::string &line);

		/// @brief Was the "fit" actually from the pre-fit estimate (ie. a guess)?
		bool isGuess(){return itsFlagGuess;};

		void print(std::ostream& theStream);
                /// @brief Output the parameters for the source
                friend std::ostream& operator<< (std::ostream& theStream, ContinuumSelavy &cont);

            protected:
		std::string itsID;
		std::string itsName;
		float itsFint;
		float itsFpeak;
		float itsFintFIT;
		float itsFpeakFIT;
		float itsMajFIT; 
		float itsMinFIT; 
		float itsPAFIT; 
		float itsMajDECONV; 
		float itsMinDECONV; 
		float itsPADECONV; 
		float itsChisq;
		float itsRMSimage;
		float itsRMSfit;
		int itsNfree;
		int itsNdof;
		int itsNpixFIT;
		int itsNpixObj;
		bool itsFlagGuess;
        };

    }

}

#endif
