/// @file CorrProdMap.h
///
/// @copyright (c) 2012 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>
///
/// The file contains functions to map correlation products
/// (also known as "baselines"):
///   correlation product index <-> antenna1, antenna2, coupled polarization
/// The base number of correlation product and antenna indices are changeable
/// (default = 0).
/// Values for coupled polarization: 0:XX, 1:XY, 2:YX, 3:YY.
/// The index numbering is according to "revtriangle.txt"

#ifndef ASKAP_CP_CORRPRODMAP_H
#define ASKAP_CP_CORRPRODMAP_H

//#define OUTSIDEASKAP

#include "Permutation.h"

// System include
#include <stdint.h>

#ifndef OUTSIDEASKAP
// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "measures/Measures/Stokes.h"

namespace askap {
namespace cp {
#endif

/// @brief Class for correlation product map.

class CorrProdMap {

    public:
		
		CorrProdMap (uint32_t nAntenna);
		
		virtual ~CorrProdMap ();

		/// Set antenna base number. Usual value is either 0 (default) or 1.
		void setAntennaBase (uint32_t antBase);

		/// Get antenna base number.
		uint32_t getAntennaBase ();

		/// Set the base number of correlation product index.
		/// Usual value is either 0 (default) or 1.
		void setIndexBase (uint32_t indexBase);

		/// Get index base number.
		uint32_t getIndexBase ();

		/// Return the total number of correlation products.
		uint32_t totalCount ();

		/// Given the antennas and coupled polarization, 
		/// return correlation product index.
		uint32_t getIndex (uint32_t ant1, uint32_t ant2, uint32_t coupledPol);

		/// Given correlation product index, return the antennas 
		std::pair<uint32_t,uint32_t> getAntennas (uint32_t index);
		
		/// Given correlation product index, return the coupled polarization
		/// (values are 0:XX, 1:XY, 2:YX, 3:YY).
		uint32_t getCoupledPolarisation (uint32_t index);
		
		/// Given correlation product, return the indices of antennas & coupled 
		/// polarization. Values for coupled polarization: 
        /// 0:XX, 1:XY, 2:YX, 3:YY
		/// TO BE DEPRECATED
		int getAntennaAndCoupledPol (uint32_t index, 
				uint32_t& ant1, uint32_t& ant2, uint32_t& coupledPol);
			
	private:
	
		uint32_t antBase;
		uint32_t indexBase;
		uint32_t nAnt;
		uint32_t nTotal;
		//Permutation perm;
		
		// Given antenna and polarity indices, return composite index
		// Antenna index is 0-based
		// Polarity index: 0 = X, 1 = Y
		uint32_t compositeIndex (uint32_t ant, uint32_t pol);

		// Given composite index, return antenna index.
		// Both are 0-based.
		uint32_t antenna (uint32_t comp);

		// Given composite index, return polarization index.
		// Both are 0-based.
		uint32_t polar (uint32_t comp);

		// Return the coupled index of 2 polarization.
		// Polarization: 0=X, Y=1
		// Coupled polarization: 0=XX, 1=XY, 2=YX, 3=YY
		uint32_t polarCouple (uint32_t pol1, uint32_t pol2);

		// Return the polarization indices from their coupled index.
		std::pair<uint32_t,uint32_t> polarDecouple (uint32_t couple);
		
		void alertAntennaValue (uint32_t ant);
		
		void alertIndexValue (uint32_t index);

		void alertCoupledPolarisationValue (uint32_t coupledPol);

		void alertValueOutsideRange (uint32_t value, uint32_t minValue, 
                uint32_t maxValue);
		
		void alertWrongAntennaOrder (uint32_t ant1, uint32_t ant2);
};

#ifndef OUTSIDEASKAP
};
};
#endif

#endif