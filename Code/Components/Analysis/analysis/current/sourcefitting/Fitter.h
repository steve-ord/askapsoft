/// @file
///
/// Contains the calls to the fitting routines
///
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
///

#ifndef ASKAP_ANALYSIS_FITTER_H_
#define ASKAP_ANALYSIS_FITTER_H_

#include <sourcefitting/FittingParameters.h>
#include <sourcefitting/SubComponent.h>

#include <casacore/scimath/Fitting/FitGaussian.h>
#include <casacore/scimath/Functionals/Gaussian2D.h>

#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>

#include <duchamp/fitsHeader.hh>

#include <casacore/casa/namespace.h>

#include <Common/ParameterSet.h>

#include <map>
#include <vector>
#include <utility>


namespace askap {

namespace analysis {

namespace sourcefitting {
class FittingParameters; // foreshadowing

/// @ingroup sourcefitting
/// @brief Simple function to write a list of parameters to the ASKAPLOG
void logparameters(Matrix<Double> &m, std::string loc = "DEBUG");

/// @ingroup sourcefitting
/// @brief A class to manage the 2D profile fitting.
/// @details The class handles the calling of the fitting
/// functions, and stores the results using the casa::FitGaussian class
/// and a casa::Matrix with the best fit. The FittingParameters class
/// holds the relevant parameters.
class Fitter {
    public:
        /// @brief Default constructor
        Fitter(FittingParameters &fitParams);
        /// @brief Default destructor
        virtual ~Fitter() {};

        /// @brief Does a fit (good or otherwise) exist for this Fitter instance?
        bool fitExists() {return itsFitExists;};

        /// @brief Set and return the set of fitting parameters
        /// @{
        void setParams(FittingParameters p) {itsParams = p;};
        FittingParameters params() {return itsParams;};
        FittingParameters &rparams() {FittingParameters& rfitpars = itsParams; return rfitpars;};
        ///@}

        /// @brief Set and return the number of Gaussian components to be fitted.
        /// @{
        void setNumGauss(int i) {itsNumGauss = i;};
        unsigned int  numGauss() {return itsNumGauss;};
        /// @}

        /// @brief Return the chi-squared value from the fit.
        float chisq() {return itsFitter.chisquared();};
        /// @brief Return the reduced chi-squared value from the fit.
        float redChisq() {return itsRedChisq;};
        /// @brief Return the RMS of the fit
        float RMS() {return itsFitter.RMS();};
        /// @brief Return the number of degrees of freedom of the fit.
        int ndof() {return itsNDoF;};

        /// @brief Set the intial estimates for the Gaussian components.
        void setEstimates(std::vector<SubComponent> cmpntList);

        /// @brief Set the retry factors
        void setRetries();
        /// @brief Set the mask values
        void setMasks();

        /// @brief Fit components to the data.
        /// @details Fits the required number of Gaussians to the
        /// data.  The fit(s) are only performed if itsFitExists is
        /// true, which is determined by whether the number of degrees
        /// of freedom for the fitting is positive.
        void fit(casa::Matrix<casa::Double> pos,
                 casa::Vector<casa::Double> f,
                 casa::Vector<casa::Double> sigma);

        /// @brief Functions to test the fit according to various criteria.
        /// @{

        /// @brief Has the fit converged?
        bool passConverged();
        /// @brief Does the fit have an acceptable chi-squared value?
        bool passChisq();
        /// @brief Are the fitted components suitably within the box?
        bool passLocation();
        /// @brief Are the component sizes big enough?
        bool passComponentSize();
        /// @brief Are the component fluxes larger than half the detection threshold?
        bool passComponentFlux();
    /// @brief If negative components are not wanted, are they all with positive flux?
        bool passNegativeComponents();
    /// @brief Are all components below twice the island peak flux?
        bool passPeakFlux();
    /// @brief The sum of the integrated fluxes of all components must
    /// not be more than twice the total flux in the box - only
    /// considered if fitting to the box pixels
        bool passIntFlux();
    /// @brief Are pairs of components separated by 2 pixels at least?
        bool passSeparation();

        /// @brief Is the fit acceptable overall?
        /// Acceptance criteria for a fit are as follows (after the
        /// FIRST survey criteria, White et al 1997, ApJ 475, 479):
        /// @li Fit must have converged
        /// @li Fit must be acceptable according to its chisq value
        /// @li The centre of each component must be inside the box
        /// @li The separation between any pair of components must be
        /// more than 2 pixels.
        /// @li The flux of each component must be positive and more
        /// than half the detection threshold
        /// @li No component's peak flux can exceed twice the highest
        /// pixel in the box
        /// @li The sum of the integrated fluxes of all components
        /// must not be more than twice the total flux in the box.
        /// @li No component can have a major axis bigger that 100,000,000
        /// pixels (the fit can occasionally converge to a
        /// near-infinite-size component.
        bool acceptable();
        /// @}

        /// @brief Is the fit acceptable, leaving aside the chi-squared value?
        /// @details Uses the same acceptance criteria as for
        /// acceptable(), except that it ignores the chi-squared
        /// value. This allows us to consider poor but otherwise
        /// acceptable fits.
        bool acceptableExceptChisq();
    
        /// @brief Return an ordered list of peak fluxes
        std::multimap<double, int> peakFluxList();

        /// @brief Return a casa::Gaussian2D version of a particular component.
        casa::Gaussian2D<casa::Double> gaussian(unsigned int num);

        /// @brief Return a vector of errors for the given component
        casa::Vector<casa::Double> error(unsigned int num);

        /// @brief Subtract fit from the flux values
        casa::Vector<casa::Double> subtractFit(casa::Matrix<casa::Double> pos,
                                               casa::Vector<casa::Double> f);


    protected:
        /// @brief The set of parameters defining the fits
        FittingParameters itsParams;

        /// @brief Whether a fit has been made
        bool itsFitExists;
        /// @brief The number of Gaussian functions to fit.
        unsigned int itsNumGauss;
        /// @brief The casa Gaussian Fitter
        FitGaussian<casa::Double> itsFitter;
        /// @brief The number of degrees of freedom in the fit
        int itsNDoF;
        /// @brief The reduced chi-squared of the fit
        float itsRedChisq;

        /// @brief The fitted components
        casa::Matrix<casa::Double> itsSolution;
        /// @brief The errors on the fitted components
        casa::Matrix<casa::Double> itsErrors;

};


}

}

}

#endif

