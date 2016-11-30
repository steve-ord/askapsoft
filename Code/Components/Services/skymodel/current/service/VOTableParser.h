/// ----------------------------------------------------------------------------
/// This file is generated by schema_definitions/generate.py.
/// Do not edit directly or your changes will be lost!
/// ----------------------------------------------------------------------------
///
/// @file VOTableParser.h
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

#ifndef ASKAP_CP_SMS_VOTABLEPARSER_H
#define ASKAP_CP_SMS_VOTABLEPARSER_H

// System includes
#include <string>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

// ASKAPsoft includes
#include <votable/VOTable.h>

// Local package includes
#include "datamodel/ContinuumComponent.h"

namespace askap {
namespace cp {
namespace sms {


void parseComponentRowField(
    size_t row_index,
    const std::string& ucd,
    const std::string& name,
    const std::string& type,
    const std::string& unit,
    const std::string& value,
    std::vector<datamodel::ContinuumComponent>& components,
    std::vector<double>& ra_buffer,
    std::vector<double>& dec_buffer) {

    // TODO: Asserts here are unacceptable in the production code.
    // They need to be replaced with more robust error handling and log messages.
    ASKAPASSERT(row_index >= 0);
    ASKAPASSERT(row_index < components.size());
    ASKAPASSERT(row_index < ra_buffer.size());
    ASKAPASSERT(row_index < dec_buffer.size());

    if (boost::iequals(ucd, "meta.id;meta.main")) {
        ASKAPASSERT(unit == "" || unit == "--");
        ASKAPASSERT(type == "char");
        components[row_index].component_id = boost::lexical_cast<std::string>(value);
    }
    else 
    if (boost::iequals(ucd, "pos.eq.ra;meta.main")) {
        ASKAPASSERT(unit == "deg");
        ASKAPASSERT(type == "double");
        components[row_index].ra_deg_cont = boost::lexical_cast<double>(value);
    }
    else 
    if (boost::iequals(ucd, "pos.eq.dec;meta.main")) {
        ASKAPASSERT(unit == "deg");
        ASKAPASSERT(type == "double");
        components[row_index].dec_deg_cont = boost::lexical_cast<double>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.error;pos.eq.ra")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].ra_err = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.error;pos.eq.dec")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].dec_err = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "em.freq")) {
        ASKAPASSERT(unit == "MHz");
        ASKAPASSERT(type == "float");
        components[row_index].freq = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phot.flux.density;stat.max;em.radio;stat.fit")) {
        ASKAPASSERT(unit == "mJy/beam");
        ASKAPASSERT(type == "float");
        components[row_index].flux_peak = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.error;phot.flux.density;stat.max;em.radio;stat.fit")) {
        ASKAPASSERT(unit == "mJy/beam");
        ASKAPASSERT(type == "float");
        components[row_index].flux_peak_err = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phot.flux.density;em.radio;stat.fit")) {
        ASKAPASSERT(unit == "mJy");
        ASKAPASSERT(type == "float");
        components[row_index].flux_int = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.error;phot.flux.density;em.radio;stat.fit")) {
        ASKAPASSERT(unit == "mJy");
        ASKAPASSERT(type == "float");
        components[row_index].flux_int_err = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phys.angSize.smajAxis;em.radio;stat.fit")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].maj_axis = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phys.angSize.sminAxis;em.radio;stat.fit")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].min_axis = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phys.angSize;pos.posAng;em.radio;stat.fit")) {
        ASKAPASSERT(unit == "deg");
        ASKAPASSERT(type == "float");
        components[row_index].pos_ang = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.error;phys.angSize.smajAxis;em.radio")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].maj_axis_err = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.error;phys.angSize.sminAxis;em.radio")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].min_axis_err = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.error;phys.angSize;pos.posAng;em.radio")) {
        ASKAPASSERT(unit == "deg");
        ASKAPASSERT(type == "float");
        components[row_index].pos_ang_err = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phys.angSize.smajAxis;em.radio;askap:meta.deconvolved")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].maj_axis_deconv = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phys.angSize.sminAxis;em.radio;askap:meta.deconvolved")) {
        ASKAPASSERT(unit == "arcsec");
        ASKAPASSERT(type == "float");
        components[row_index].min_axis_deconv = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "phys.angSize;pos.posAng;em.radio;askap:meta.deconvolved")) {
        ASKAPASSERT(unit == "deg");
        ASKAPASSERT(type == "float");
        components[row_index].pos_ang_deconv = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.fit.chi2")) {
        ASKAPASSERT(unit == "" || unit == "--");
        ASKAPASSERT(type == "float");
        components[row_index].chi_squared_fit = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.stdev;stat.fit")) {
        ASKAPASSERT(unit == "mJy/beam");
        ASKAPASSERT(type == "float");
        components[row_index].rms_fit_Gauss = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "spect.index;em.radio")) {
        ASKAPASSERT(unit == "" || unit == "--");
        ASKAPASSERT(type == "float");
        components[row_index].spectral_index = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "askap:spect.curvature;em.radio")) {
        ASKAPASSERT(unit == "" || unit == "--");
        ASKAPASSERT(type == "float");
        components[row_index].spectral_curvature = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(ucd, "stat.stdev;phot.flux.density")) {
        ASKAPASSERT(unit == "mJy/beam");
        ASKAPASSERT(type == "float");
        components[row_index].rms_image = boost::lexical_cast<float>(value);
    }
    else 
    if (boost::iequals(name, "has_siblings")) {
        // Some fields do not have a unique UCD. They are matched by name.
        ASKAPASSERT(unit == "" || unit == "--");
        ASKAPASSERT(type == "int");
        components[row_index].has_siblings = boost::lexical_cast<bool>(value);
    }
    else 
    if (boost::iequals(name, "fit_is_estimate")) {
        // Some fields do not have a unique UCD. They are matched by name.
        ASKAPASSERT(unit == "" || unit == "--");
        ASKAPASSERT(type == "int");
        components[row_index].fit_is_estimate = boost::lexical_cast<bool>(value);
    }
}

}
}
}

#endif
