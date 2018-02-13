/// ----------------------------------------------------------------------------
/// This file is generated by schema_definitions/generate.py.
/// Do not edit directly or your changes will be lost!
/// ----------------------------------------------------------------------------
///
/// @copyright (c) 2016 CSIRO
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
/// @author Daniel Collins <daniel.collins@csiro.au>

#pragma once

#include <CommonTypes.ice>

module askap
{
module interfaces
{
module skymodelservice
{

    /**
     * Continuum component polarisation data.
     **/
    struct ContinuumComponentPolarisation
    {
        /// @brief Reference wavelength squared (m^2)
        /// UCD: askap:em.wl.squared
        double lambdaRefSq;

        /// @brief Peak polarised intensity from a three-point parabolic fit (mJy/beam)
        /// UCD: phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
        double polPeakFit;

        /// @brief Peak polarised intensity, corrected for bias, from a three-point parabolic fit (mJy/beam)
        /// UCD: phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit;askap:meta.corrected
        double polPeakFitDebias;

        /// @brief Uncertainty in pol_peak_fit (mJy/beam)
        /// UCD: stat.error;phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
        double polPeakFitErr;

        /// @brief Signal-to-noise ratio of the peak polarisation
        /// UCD: stat.snr;phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
        double polPeakFitSnr;

        /// @brief Uncertainty in pol_peak_fit_snr
        /// UCD: stat.error;stat.snr;phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit
        double polPeakFitSnrErr;

        /// @brief Faraday Depth from fit to peak in Faraday Dispersion Function (rad/m^2)
        /// UCD: phys.polarization.rotMeasure;stat.fit
        double fdPeakFit;

        /// @brief uncertainty in fd_peak_fit (rad/m^2)
        /// UCD: stat.error;phys.polarization.rotMeasure;stat.fit
        double fdPeakFitErr;

    };

    /**
     * Define a sequence for storing the optional polarisation data inside the
     * component
     **/
    sequence<ContinuumComponentPolarisation> PolarisationOpt;

    /**
     * A continuum component.
     **/
    struct ContinuumComponent
    {
        /// @brief Should only ever contain 0 or 1 elements.
        PolarisationOpt polarisation;

        /// @brief Globally unique 64 bit integer ID
        long id;

        /// @brief Component identifier
        /// UCD: meta.id;meta.main
        string componentId;

        /// @brief J2000 right ascension (deg)
        /// UCD: pos.eq.ra;meta.main
        double ra;

        /// @brief J2000 declination (deg)
        /// UCD: pos.eq.dec;meta.main
        double dec;

        /// @brief Error in Right Ascension (arcsec)
        /// UCD: stat.error;pos.eq.ra
        float raErr;

        /// @brief Error in Declination (arcsec)
        /// UCD: stat.error;pos.eq.dec
        float decErr;

        /// @brief Frequency (MHz)
        /// UCD: em.freq
        float freq;

        /// @brief Peak flux density (mJy/beam)
        /// UCD: phot.flux.density;stat.max;em.radio;stat.fit
        float fluxPeak;

        /// @brief Error in peak flux density (mJy/beam)
        /// UCD: stat.error;phot.flux.density;stat.max;em.radio;stat.fit
        float fluxPeakErr;

        /// @brief Integrated flux density (mJy)
        /// UCD: phot.flux.density;em.radio;stat.fit
        float fluxInt;

        /// @brief Error in integrated flux density (mJy)
        /// UCD: stat.error;phot.flux.density;em.radio;stat.fit
        float fluxIntErr;

        /// @brief FWHM major axis before deconvolution (arcsec)
        /// UCD: phys.angSize.smajAxis;em.radio;stat.fit
        float majAxis;

        /// @brief FWHM minor axis before deconvolution (arcsec)
        /// UCD: phys.angSize.sminAxis;em.radio;stat.fit
        float minAxis;

        /// @brief Position angle before deconvolution (deg)
        /// UCD: phys.angSize;pos.posAng;em.radio;stat.fit
        float posAng;

        /// @brief Error in major axis before deconvolution (arcsec)
        /// UCD: stat.error;phys.angSize.smajAxis;em.radio
        float majAxisErr;

        /// @brief Error in minor axis before deconvolution (arcsec)
        /// UCD: stat.error;phys.angSize.sminAxis;em.radio
        float minAxisErr;

        /// @brief FWHM major axis after deconvolution (arcsec)
        /// UCD: phys.angSize.smajAxis;em.radio;askap:meta.deconvolved
        float majAxisDeconv;

        /// @brief FWHM minor axis after deconvolution (arcsec)
        /// UCD: phys.angSize.sminAxis;em.radio;askap:meta.deconvolved
        float minAxisDeconv;

        /// @brief Position angle after deconvolution (deg)
        /// UCD: phys.angSize;pos.posAng;em.radio;askap:meta.deconvolved
        float posAngDeconv;

        /// @brief Spectral index (First Taylor term)
        /// UCD: spect.index;em.radio
        float spectralIndex;

        /// @brief Spectral curvature (Second Taylor term)
        /// UCD: askap:spect.curvature;em.radio
        float spectralCurvature;

        /// @brief  (arcsec)
        /// UCD: stat.error;phys.angSize.smajAxis;em.radio;askap:meta.deconvolved
        float majAxisDeconvErr;

        /// @brief  (arcsec)
        /// UCD: stat.error;phys.angSize.sminAxis;em.radio;askap:meta.deconvolved
        float minAxisDeconvErr;

        /// @brief  (deg)
        /// UCD: stat.error;phys.angSize;pos.posAng;em.radio;askap:meta.deconvolved
        float posAngDeconvErr;

        /// @brief 
        /// UCD: stat.error;spect.index;em.radio
        float spectralIndexErr;

        /// @brief 
        /// UCD: meta.code
        int spectralIndexFromTt;

    };

};
};
};
