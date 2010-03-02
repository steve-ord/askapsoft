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

#include <gridding/TableVisGridder.h>
#include <askap_synthesis.h>
#include <askap/AskapLogging.h>
#include <dataaccess/IDataAccessor.h>
#include <dataaccess/OnDemandBufferDataAccessor.h>
#include <dataaccess/PolConverter.h>
ASKAP_LOGGER(logger, ".gridding");

#include <askap/AskapError.h>
#include <askap/AskapUtil.h>
#include <fft/FFTWrapper.h>

#include <casa/BasicSL/Constants.h>
#include <casa/Arrays/ArrayIter.h>
#include <casa/Arrays/Slicer.h>

#include <measures/Measures/MDirection.h>
#include <measures/Measures/UVWMachine.h>

#include <fitting/Params.h>
#include <fitting/ParamsCasaTable.h>

#include <gridding/GridKernel.h>

#include <utils/PaddingUtils.h>
#include <measurementequation/ImageParamsHelper.h>
#include <measurementequation/SynthesisParamsHelper.h>

using namespace askap::scimath;
using namespace askap;

#include <ostream>
#include <sstream>
#include <iomanip>

#include <casa/OS/Timer.h>

namespace askap {
namespace synthesis {


/// @brief a helper method for a deep copy of casa arrays held in
/// stl vectors
/// @param[in] in input array
/// @param[out] out output array (will be resized)
template<typename T>
void deepCopyOfSTDVector(const std::vector<T> &in,
                         std::vector<T> &out)
{
   out.resize(in.size());

   const typename std::vector<T>::const_iterator inEnd = in.end();
   typename std::vector<T>::iterator outIt = out.begin();
   for (typename std::vector<T>::const_iterator inIt = in.begin();
        inIt != inEnd; ++inIt,++outIt) {
        *outIt = inIt->copy();
   }
}

TableVisGridder::TableVisGridder() : itsSumWeights(),
        itsSupport(-1), itsOverSample(-1),
	itsName(""), itsModelIsEmpty(false), itsSamplesGridded(0),
			itsSamplesDegridded(0), itsVectorsFlagged(0), itsNumberGridded(0), itsNumberDegridded(0),
	itsTimeCoordinates(0.0), itsTimeConvFunctions(0.0), itsTimeGridded(0.0), 
	itsTimeDegridded(0.0), itsDopsf(false),
	itsPaddingFactor(1),
	itsFirstGriddedVis(true), itsFeedUsedForPSF(0), itsUseAllDataForPSF(false),
	itsMaxPointingSeparation(-1.), itsRowsRejectedDueToMaxPointingSeparation(0)

{}

TableVisGridder::TableVisGridder(const int overSample, const int support,
        const int padding, const std::string& name) : itsSumWeights(),
		 itsSupport(support), itsOverSample(overSample), itsName(name),
				itsModelIsEmpty(false), itsSamplesGridded(0),
				itsSamplesDegridded(0), itsVectorsFlagged(0), itsNumberGridded(0), itsNumberDegridded(0),
		itsTimeCoordinates(0.0), itsTimeConvFunctions(0.0), itsTimeGridded(0.0), 
		itsTimeDegridded(0.0), itsDopsf(false),
		itsPaddingFactor(padding),
		itsFirstGriddedVis(true), itsFeedUsedForPSF(0), itsUseAllDataForPSF(false), 	
		itsMaxPointingSeparation(-1.), itsRowsRejectedDueToMaxPointingSeparation(0)		
	{
		
		ASKAPCHECK(overSample>0, "Oversampling must be greater than 0");
		ASKAPCHECK(support>0, "Maximum support must be greater than 0");
		ASKAPCHECK(padding>0, "Padding factor must be greater than 0");
	}

	/// @brief copy constructor
	/// @details it is required to decouple arrays between the input object
	/// and the copy.
	/// @param[in] other input object
	TableVisGridder::TableVisGridder(const TableVisGridder &other) : 
	     itsAxes(other.itsAxes), itsShape(other.itsShape), 
	     itsUVCellSize(other.itsUVCellSize.copy()), 
	     itsSumWeights(other.itsSumWeights.copy()), 
     itsSupport(other.itsSupport), itsOverSample(other.itsOverSample),
     itsCSize(other.itsCSize), itsCCenter(other.itsCCenter), itsName(other.itsName),
     itsModelIsEmpty(other.itsModelIsEmpty), itsSamplesGridded(other.itsSamplesGridded),
     itsSamplesDegridded(other.itsSamplesDegridded), itsVectorsFlagged(other.itsVectorsFlagged),
     itsNumberGridded(other.itsNumberGridded),
     itsNumberDegridded(other.itsNumberDegridded), itsTimeCoordinates(other.itsTimeCoordinates),
     itsTimeConvFunctions(other.itsTimeConvFunctions),
     itsTimeGridded(other.itsTimeGridded),
     itsTimeDegridded(other.itsTimeDegridded),
     itsDopsf(other.itsDopsf), itsPaddingFactor(other.itsPaddingFactor),
     itsFirstGriddedVis(other.itsFirstGriddedVis),
     itsFeedUsedForPSF(other.itsFeedUsedForPSF),
     itsPointingUsedForPSF(other.itsPointingUsedForPSF),
     itsUseAllDataForPSF(other.itsUseAllDataForPSF),
     itsFreqMapper(other.itsFreqMapper),
     itsMaxPointingSeparation(other.itsMaxPointingSeparation),
     itsRowsRejectedDueToMaxPointingSeparation(other.itsRowsRejectedDueToMaxPointingSeparation)     
{
   deepCopyOfSTDVector(other.itsConvFunc,itsConvFunc);
   deepCopyOfSTDVector(other.itsGrid, itsGrid);   
   if(other.itsVisWeight) {
      itsVisWeight = other.itsVisWeight->clone();
   } else {
      itsVisWeight = other.itsVisWeight;
   }
}
     

TableVisGridder::~TableVisGridder() {
	if (itsNumberGridded>0) {
		if (isPSFGridder()) {
            ASKAPLOG_INFO_STR(logger, "TableVisGridder PSF gridding statistics");
		    ASKAPLOG_INFO_STR(logger, "   PSF samples gridded       = "
                              << itsSamplesGridded);
            ASKAPLOG_INFO_STR(logger, "   Visibility vectors flagged (psf)     = "
                              << itsVectorsFlagged);                  
		    ASKAPLOG_INFO_STR(logger, "   Total time for PSF gridding   = "
				<< itsTimeGridded << " (s)");
		    ASKAPLOG_INFO_STR(logger, "   PSF gridding time         = " << 1e6
			 	*itsTimeGridded/itsSamplesGridded << " (us) per sample");
		    ASKAPLOG_INFO_STR(logger, "   Total time converting for PSF = "
	            << itsTimeCoordinates << " (s)");
	        ASKAPLOG_INFO_STR(logger, "   Total time building CFs and indices for PSF = "
				<< itsTimeConvFunctions << " (s)");				
		    ASKAPLOG_INFO_STR(logger, "   PSF coord conversion      = "
				  << 1e6 * itsTimeCoordinates/itsSamplesGridded << " (us) per sample");
		    ASKAPLOG_INFO_STR(logger, "   PSF CFs and indices      = "
				  << 1e6 * itsTimeConvFunctions/itsSamplesGridded << " (us) per sample");
		    ASKAPLOG_INFO_STR(logger, "   " << GridKernel::info());
		    ASKAPLOG_INFO_STR(logger, "   Points gridded (psf)        = "
	              << itsNumberGridded);
		    ASKAPLOG_INFO_STR(logger, "   Time per point (psf)        = " << 1e9
	              *itsTimeGridded/itsNumberGridded << " (ns)");
		    ASKAPLOG_INFO_STR(logger, "   Performance for PSF         = "
				<< 8.0 * 1e-9 * itsNumberGridded/itsTimeGridded << " GFlops");
		} else {
            ASKAPLOG_INFO_STR(logger, "TableVisGridder gridding statistics");
		    ASKAPLOG_INFO_STR(logger, "   Samples gridded       = "
                         << itsSamplesGridded);		
            ASKAPLOG_INFO_STR(logger, "   Visibility vectors flagged       = "
                          << itsVectorsFlagged);                                           
		    ASKAPLOG_INFO_STR(logger, "   Total time gridding   = "
			             << itsTimeGridded << " (s)");
		    ASKAPLOG_INFO_STR(logger, "   Gridding time         = " << 1e6
			  	*itsTimeGridded/itsSamplesGridded << " (us) per sample");
		    ASKAPLOG_INFO_STR(logger, "   Total time converting = "
				<< itsTimeCoordinates << " (s)");
            ASKAPLOG_INFO_STR(logger, "   Total time building CFs and indices = "
				<< itsTimeConvFunctions << " (s)");				
		    ASKAPLOG_INFO_STR(logger, "   Coord conversion      = "
				  << 1e6 * itsTimeCoordinates/itsSamplesGridded << " (us) per sample");
		    ASKAPLOG_INFO_STR(logger, "   CFs and indices      = "
				  << 1e6 * itsTimeConvFunctions/itsSamplesGridded << " (us) per sample");
		    ASKAPLOG_INFO_STR(logger, "   " << GridKernel::info());
		    ASKAPLOG_INFO_STR(logger, "   Points gridded        = "
				<< itsNumberGridded);
		    ASKAPLOG_INFO_STR(logger, "   Time per point        = " << 1e9
				*itsTimeGridded/itsNumberGridded << " (ns)");
		    ASKAPLOG_INFO_STR(logger, "   Performance           = "
				<< 8.0 * 1e-9 * itsNumberGridded/itsTimeGridded << " GFlops");
	    }			
	}
	if (itsNumberDegridded>0) {
		ASKAPLOG_INFO_STR(logger, "TableVisGridder degridding statistics");
		ASKAPLOG_INFO_STR(logger, "   Samples degridded     = "
				<< itsSamplesDegridded);
		ASKAPLOG_INFO_STR(logger, "   Total time degridding = "
				<< itsTimeDegridded << " (s)");
		ASKAPLOG_INFO_STR(logger, "   Degridding time       = " << 1e6
				*itsTimeDegridded/itsSamplesDegridded << " (us) per sample");
		ASKAPLOG_INFO_STR(logger, "   Total time converting = "
				<< itsTimeCoordinates << " (s)");
		ASKAPLOG_INFO_STR(logger, "   Total time building CFs and indices = "
				<< itsTimeConvFunctions << " (s)");				
		ASKAPLOG_INFO_STR(logger, "   Coord conversion      = "
				  << 1e6 * itsTimeCoordinates/itsSamplesDegridded << " (us) per sample");
		ASKAPLOG_INFO_STR(logger, "   CFs and indices      = "
				  << 1e6 * itsTimeConvFunctions/itsSamplesDegridded << " (us) per sample");
		ASKAPLOG_INFO_STR(logger, "   " << GridKernel::info());
		ASKAPLOG_INFO_STR(logger, "   Points degridded      = "
				<< itsNumberDegridded);
		ASKAPLOG_INFO_STR(logger, "   Time per point        = " << 1e9
				*itsTimeDegridded/itsNumberDegridded << " (ns)");
		ASKAPLOG_INFO_STR(logger, "   Performance           = "
				<< 8.0 * 1e-9 * itsNumberDegridded/itsTimeDegridded << " GFlops");
	}
	if (itsMaxPointingSeparation > 0.) {
	    ASKAPLOG_INFO_STR(logger, "   Samples rejected due to MaxPointingSeparation = "<<
	                               itsRowsRejectedDueToMaxPointingSeparation);
	}
	if((itsNumberGridded<1) && (itsNumberDegridded<1)) {
	  ASKAPLOG_WARN_STR(logger, "Unused gridder");
	  if (itsRowsRejectedDueToMaxPointingSeparation != 0) {
          ASKAPLOG_WARN_STR(logger, "It looks like all samples were rejected due to MaxPointingSeparation!");
	  }
	} else {
	  ASKAPLOG_INFO_STR(logger, "   Padding factor    = " << itsPaddingFactor);
	  if(itsName!="") {
	    save(itsName);
	  }
	}
}

void TableVisGridder::save(const std::string& name) {
    if (name.find("image:") != 0) {
	    askap::scimath::ParamsCasaTable iptable(name, false);
	    askap::scimath::Params ip;
	    ASKAPLOG_INFO_STR(logger, "Saving " << itsConvFunc.size() << " entries in convolution function");
	    for (unsigned int i=0; i<itsConvFunc.size(); i++) {
		   
			casa::Array<double> realC(itsConvFunc[i].shape());
			toDouble(realC, itsConvFunc[i]);
			//			ASKAPLOG_INFO_STR(logger, "Entry[" <<  i <<  "] has shape " <<  itsConvFunc[i].shape());
			std::ostringstream os;
			os<<"Real.Convolution";
			os.width(5);
			os.fill('0');
			os<<i;
			ip.add(os.str(), realC);
	    }
	    iptable.setParameters(ip);
	} else {
	    if (itsNumberGridded == 0) {
	        ASKAPLOG_INFO_STR(logger, "Ignore tablename="<<name<<" option as no visibilities were gridded");
	        return;
        }
	    if (isPSFGridder()) {
	        ASKAPLOG_INFO_STR(logger, "Ignore tablename="<<name<<" option for the PSF gridder");
	        return;
	    }
   
	    const std::string imgName = name.substr(6);
	    // number of planes before oversampling
	    const unsigned long nPlanes = itsConvFunc.size()/itsOverSample/itsOverSample; 
	    if (nPlanes > 0) {
	        ASKAPLOG_INFO_STR(logger, "Saving convolution functions into a cube "<<imgName<<" with " << nPlanes<<
	                              " planes (first oversampling plane only)");
	        ASKAPDEBUGASSERT(itsConvFunc.size()>0);
	        casa::Cube<casa::Float> imgBuffer(itsConvFunc[0].shape()[0], itsConvFunc[0].shape()[1], nPlanes);
	        for (unsigned int plane = 0; plane<nPlanes; ++plane) {
	            for (unsigned int x = 0; x<imgBuffer.nrow(); ++x) {
	                 for (unsigned int y = 0; y<imgBuffer.ncolumn(); ++y) {
	                      const casa::Matrix<casa::Complex> thisCF = itsConvFunc[plane*itsOverSample*itsOverSample];
	                      if ( (x >= thisCF.nrow()) || (y >= thisCF.ncolumn())) {
	                           continue;
	                      }
	                      imgBuffer(x,y,plane) = casa::abs(itsConvFunc[plane](x,y));	                  
	                 }
	            }
	        }
	        SynthesisParamsHelper::saveAsCasaImage(imgName,imgBuffer);
	    }       
	}                   
}

/// This is a generic grid/degrid
void TableVisGridder::generic(IDataAccessor& acc, bool forward) {
   if (forward&&itsModelIsEmpty)
		return;
   
   if (forward && isPSFGridder()) {
       ASKAPTHROW(AskapError, "Logic error: the gridder is not supposed to be used for degridding in the PSF mode")
   }
      
   casa::Timer timer;

   // Time CFs and indices
   timer.mark();
      
   initIndices(acc);
   initConvolutionFunction(acc);
   if (!forward) {
      ASKAPCHECK(itsSumWeights.nelements()>0, "SumWeights not yet initialised");
   }
	  
   itsTimeConvFunctions += timer.real();
   
   // Now time the coordinate conversions, etc.
   // some conversion may have already happened during CF calculation
   timer.mark();
   
   const casa::Vector<casa::RigidVector<double, 3> > &outUVW = acc.rotatedUVW(getTangentPoint());
   const casa::Vector<double> &delay = acc.uvwRotationDelay(getTangentPoint(), getImageCentre());

   itsTimeCoordinates += timer.real();

   // Now time the gridding
   timer.mark();

   ASKAPCHECK(itsSupport>0, "Support must be greater than 0");
   ASKAPCHECK(itsUVCellSize.size()==2, "UV cell sizes not yet set");
   
   const uint nSamples = acc.nRow();
   const uint nChan = acc.nChannel();
   const uint nPol = acc.nPol();
   const casa::Vector<casa::Double>& frequencyList = acc.frequency();
   itsFreqMapper.setupMapping(frequencyList);
   
   // for now setup the converter inside this method, although it would cause a rebuild 
   // of the matrices for every accessor. More intelligent caching is possible with a bit
   // more effort (i.e. one has to detect whether polarisation frames change from the
   // previous call). Need to think about parallactic angle dependence.
   PolConverter gridPolConv(acc.stokes(), getStokes());
   PolConverter degridPolConv(getStokes(),acc.stokes(), false);
			      
   ASKAPDEBUGASSERT(itsShape.nelements()>=2);
   const casa::IPosition onePlane4D(4, itsShape(0), itsShape(1), 1, 1);
   const casa::IPosition onePlane(2, itsShape(0), itsShape(1));
   
   // Loop over all samples adding them to the grid
   // First scale to the correct pixel location
   // Then find the fraction of a pixel to the nearest pixel
   // Loop over the entire itsSupport, calculating weights from
   // the convolution function and adding the scaled
   // visibility to the grid.
   
   ASKAPDEBUGASSERT(casa::uInt(nChan) <= frequencyList.nelements());
   ASKAPDEBUGASSERT(casa::uInt(nSamples) == acc.uvw().nelements());
   
   const casa::MVDirection imageCentre = getImageCentre();
   
   for (uint i=0; i<nSamples; ++i) {
       if (itsMaxPointingSeparation > 0.) {
           // need to reject samples, if too far from the image centre
           const casa::MVDirection thisPointing  = acc.pointingDir1()(i);
           if (imageCentre.separation(thisPointing) > itsMaxPointingSeparation) {
               ++itsRowsRejectedDueToMaxPointingSeparation;
               continue;
           }
       }
   
       if (itsFirstGriddedVis && isPSFGridder()) {
           // data members related to representative feed and field are used for
           // reverse problem only (from visibilities to image). 
           if (itsUseAllDataForPSF) {
               ASKAPLOG_INFO_STR(logger, "All data are used to estimate PSF");       
           } else {
               itsFeedUsedForPSF = acc.feed1()(i);
               itsPointingUsedForPSF = acc.dishPointing1()(i);    
               ASKAPLOG_INFO_STR(logger, "Using the data for feed "<<itsFeedUsedForPSF<<
                  " and field at "<<printDirection(itsPointingUsedForPSF)<<" to estimate the PSF");
           }
           itsFirstGriddedVis = false;
       }
	   
	   for (uint chan=0; chan<nChan; ++chan) {
		   
		   const double reciprocalToWavelength = frequencyList[chan]/casa::C::c;
		   if (chan == 0) {
		      // check for ridiculous frequency to pick up a possible error with input file,
		      // not essential for processing as such
		      ASKAPCHECK((reciprocalToWavelength>0.1) && (reciprocalToWavelength<1000), 
		          "Check frequencies in the input file as the order of magnitude is likely to be wrong, "
		          "comment this statement in the code if you're trying something non-standard. Frequency = "<<
		          frequencyList[chan]/1e9<<" GHz");
		   }
		   
		   /// Scale U,V to integer pixels plus fractional terms
		   double uScaled=frequencyList[chan]*outUVW(i)(0)/(casa::C::c *itsUVCellSize(0));
		   int iu = askap::nint(uScaled);
		   int fracu=askap::nint(itsOverSample*(double(iu)-uScaled));
		   if (fracu<0) {
                       iu+=1;
                       fracu += itsOverSample;
		   } else if (fracu>=itsOverSample) {
                       iu-=1;
                       fracu -= itsOverSample;
		   }
		   ASKAPCHECK(fracu>-1, "Fractional offset in u is negative, uScaled="<<uScaled<<" iu="<<iu<<" oversample="<<itsOverSample<<" fracu="<<fracu);
		   ASKAPCHECK(fracu<itsOverSample,
				   "Fractional offset in u exceeds oversampling, uScaled="<<uScaled<<" iu="<<iu<<" oversample="<<itsOverSample<<" fracu="<<fracu);
		   iu+=itsShape(0)/2;
		   
		   double vScaled=frequencyList[chan]*outUVW(i)(1)/(casa::C::c *itsUVCellSize(1));
		   int iv = askap::nint(vScaled);
		   int fracv=askap::nint(itsOverSample*(double(iv)-vScaled));
		   if (fracv<0) {
                       iv+=1;
                       fracv += itsOverSample;
		   } else if (fracv>=itsOverSample) {
                       iv-=1;
                       fracv -= itsOverSample;
		   }
		   ASKAPCHECK(fracv>-1, "Fractional offset in v is negative, vScaled="<<vScaled<<" iv="<<iv<<" oversample="<<itsOverSample<<" fracv="<<fracv);
		   ASKAPCHECK(fracv<itsOverSample,
				   "Fractional offset in v exceeds oversampling, vScaled="<<vScaled<<" iv="<<iv<<" oversample="<<itsOverSample<<" fracv="<<fracv);
		   iv+=itsShape(1)/2;
		   
		   // Calculate the delay phasor
		   const double phase=2.0f*casa::C::pi*frequencyList[chan]*delay(i)/(casa::C::c);
			          
		   const casa::Complex phasor(cos(phase), sin(phase));
		   
		   bool allPolGood=true;
		   for (uint pol=0; pol<nPol; ++pol) {
			   if (acc.flag()(i, chan, pol))
				   allPolGood=false;
		   }
  
           /*
           // temporary for debugging
           if (allPolGood && !itsFreqMapper.isMapped(chan)) {
              ASKAPLOG_INFO_STR(logger, "Channel "<<chan<<" is not mapped to the image cube");       
           }
           */
           
		   // Ensure that we only use unflagged data, incomplete polarisation vectors are 
		   // ignored
		   // @todo Be more careful about matching polarizations
		   if (allPolGood && itsFreqMapper.isMapped(chan)) {
		     // obtain which channel of the image this accessor channel is mapped to
		     const int imageChan = itsFreqMapper(chan);

             // number of polarisation planes in the grid
			 const casa::uInt nImagePols = (shape().nelements()<=2) ? 1 : shape()[2];
			 
			 // a buffer for the visibility vector in the polarisation frame used for the grid
			 casa::Vector<casa::Complex> imagePolFrameVis(nImagePols);
			 if (!isPSFGridder()) {
			     // both forward and reverse are covered, isPSFGridder returns false for the forward gridder
			     imagePolFrameVis = gridPolConv(acc.visibility().yzPlane(i).row(chan));			     
			 }
		     
		     // Now loop over all image polarizations
		     for (uint pol=0; pol<nImagePols; ++pol) {
		          // Lookup the portion of grid to be
			      // used for this row, polarisation and channel
			      const int gInd=gIndex(i, pol, chan);
			      ASKAPCHECK(gInd>-1,"Index into image grid is less than zero");
                  ASKAPCHECK(gInd<int(itsGrid.size()), "Index into image grid exceeds number of planes");
			   			   
                  /// Make a slicer to extract just this plane
                  const casa::IPosition ipStart(4, 0, 0, pol, imageChan);
                  const casa::Slicer slicer(ipStart, onePlane4D);
			   
                  // Lookup the convolution function to be
                  // used for this row, polarisation and channel
                  // cIndex gives the index for this row, polarization and channel. On top of
                  // that, we need to adjust for the oversampling since each oversampled
                  // plane is kept as a separate matrix.
                  const int cInd=fracu+itsOverSample*(fracv+itsOverSample*cIndex(i, pol, chan));
			      ASKAPCHECK(cInd>-1,"Index into convolution functions is less than zero");
			      ASKAPCHECK(cInd<int(itsConvFunc.size()),
					   "Index into convolution functions exceeds number of planes");
			   
			      casa::Matrix<casa::Complex> & convFunc(itsConvFunc[cInd]);
			   
			      casa::Array<casa::Complex> aGrid(itsGrid[gInd](slicer));
			      casa::Matrix<casa::Complex> grid(aGrid.nonDegenerate());
			   
			      /// Need to check if this point lies on the grid (taking into 
			      /// account the support)
			      if (((iu-itsSupport)>0)&&((iv-itsSupport)>0)&&((iu
							   +itsSupport) <itsShape(0))&&((iv+itsSupport)
							   <itsShape(1))) {
	                  if (forward) {
                          casa::Complex cVis(imagePolFrameVis[pol]);
                          GridKernel::degrid(cVis, convFunc, grid, iu, iv, itsSupport);
                          itsSamplesDegridded+=1.0;
                          itsNumberDegridded+=double((2*itsSupport+1)*(2*itsSupport+1));
                          if (itsVisWeight) {
                              cVis *= itsVisWeight->getWeight(i,frequencyList[chan],pol);
                          }
                          imagePolFrameVis[pol] += cVis*phasor;
                      } else {
                          if (!isPSFGridder()) {
                              /// Gridding visibility data onto grid
                              casa::Complex rVis = phasor*conj(imagePolFrameVis[pol]);
                              if (itsVisWeight) {
                                  rVis *= itsVisWeight->getWeight(i,frequencyList[chan],pol);
                              }
				   
                              GridKernel::grid(grid, convFunc, rVis, iu, iv, itsSupport);
			           
                              itsSamplesGridded+=1.0;
                              itsNumberGridded+=double((2*itsSupport+1)*(2*itsSupport+1));
			       
                              ASKAPCHECK(itsSumWeights.nelements()>0, "Sum of weights not yet initialised");
                              ASKAPCHECK(cIndex(i,pol,chan) < int(itsSumWeights.shape()(0)),
					 "Index " << cIndex(i,pol,chan) << " greater than allowed " << int(itsSumWeights.shape()(0)));
                              ASKAPDEBUGASSERT(pol < uint(itsSumWeights.shape()(1)));
                              ASKAPDEBUGASSERT(imageChan < int(itsSumWeights.shape()(2)));
				
                              itsSumWeights(cIndex(i,pol,chan), pol, imageChan)+=1.0;
                          }
                          /// Grid PSF?
                          if (isPSFGridder() && (itsUseAllDataForPSF || ((itsFeedUsedForPSF == acc.feed1()(i)) &&
				                 (itsPointingUsedForPSF.separation(acc.dishPointing1()(i))<1e-6)))) {
                              casa::Complex uVis(1.,0.);
                              if (itsVisWeight) {
                                  uVis *= itsVisWeight->getWeight(i,frequencyList[chan],pol);
                              }
                              GridKernel::grid(grid, convFunc, uVis, iu, iv, itsSupport);
                      
                              itsSamplesGridded+=1.0;
                              itsNumberGridded+=double((2*itsSupport+1)*(2*itsSupport+1));
			       
                              ASKAPCHECK(itsSumWeights.nelements()>0, "Sum of weights not yet initialised");
                              ASKAPCHECK(cIndex(i,pol,chan) < int(itsSumWeights.shape()(0)), "Index " << cIndex(i,pol,chan) << " greater than allowed " << int(itsSumWeights.shape()(0)));
                              ASKAPDEBUGASSERT(pol < uint(itsSumWeights.shape()(1)));
                              ASKAPDEBUGASSERT(imageChan < int(itsSumWeights.shape()(2)));
				
                              itsSumWeights(cIndex(i,pol,chan), pol, imageChan)+=1.0;
                      
                          } // end if psf needs to be done
			         } // end if forward (else case, reverse operation)
			      } // end of on-grid if statement
		     }//end of pol loop
		     // need to write back the result for degridding
		     if (forward) {
			     acc.rwVisibility().yzPlane(i).row(chan) = degridPolConv(imagePolFrameVis);
			 }		     
		   } else { 
		       if (!forward) {
		           itsVectorsFlagged+=1;
		       } 
		   } // end of if (allPolGood), else statement
	   }//end of chan loop
   }//end of i loop
   if (forward) {
	   itsTimeDegridded+=timer.real();
   } else {
	   itsTimeGridded+=timer.real();
   }
}

/// @brief correct visibilities, if necessary
/// @details This method is intended for on-the-fly correction of visibilities (i.e. 
/// facet-based correction needed for LOFAR). This method does nothing in this class, but
/// can be overridden in the derived classes to plug some effect in. The same method is 
/// used for both gridding and degridding, with the forward parameter used to distinguish
/// between these two operations. A non-const accessor has to be modified in situ, if a
/// correction is required. A buffer for read-only visibilities is created on-demand when
/// rwVisibility method of the accessor is called for the first time.
void TableVisGridder::correctVisibilities(IDataAccessor &, bool) {}

/// @brief Degrid the visibility data.
/// @param[in] acc non-const data accessor to work with  
void TableVisGridder::degrid(IDataAccessor& acc) {
	generic(acc, true);
	correctVisibilities(acc, true);
}

/// @brief Grid the visibility data.
/// @param acc const data accessor to work with
/// @note a non-const adapter is created behind the scene. If no on-the-fly visibility 
/// correction is performed, this adapter is equivalent to the original const data accessor
void TableVisGridder::grid(IConstDataAccessor& acc) {
    OnDemandBufferDataAccessor bufAcc(acc);
	correctVisibilities(bufAcc, false);
	generic(bufAcc, false);
}

/// @brief obtain the centre of the image
/// @details This method extracts RA and DEC axes from itsAxes and
/// forms a direction measure corresponding to the middle of each axis.
/// @return direction measure corresponding to the image centre
casa::MVDirection TableVisGridder::getImageCentre() const
{
   ASKAPCHECK(itsAxes.hasDirection(),"Direction axis is missing. axes="<<itsAxes);
   casa::MDirection out;
   casa::Vector<casa::Double> centrePixel(2);
   ASKAPDEBUGASSERT(itsShape.nelements()>=2);
   ASKAPDEBUGASSERT(itsPaddingFactor>0);
   for (size_t dim=0; dim<2; ++dim) {
        centrePixel[dim] = double(itsShape[dim])/2./double(itsPaddingFactor);
   }
   ASKAPCHECK(itsAxes.directionAxis().toWorld(out, centrePixel), 
        "Unable to obtain world coordinates for the centre of the image. Something is wrong with the coordinate system");
   return out.getValue();
}

/// @brief obtain the tangent point
/// @details For faceting all images should be constructed for the same tangent
/// point. This method extracts the tangent point (reference position) from the
/// coordinate system.
/// @return direction measure corresponding to the tangent point
casa::MVDirection TableVisGridder::getTangentPoint() const
{
   ASKAPCHECK(itsAxes.hasDirection(),"Direction axis is missing. axes="<<itsAxes);
   const casa::Vector<casa::Double> refVal(itsAxes.directionAxis().referenceValue());
   ASKAPDEBUGASSERT(refVal.nelements() == 2);
   const casa::Quantum<double> refLon(refVal[0], "rad");
   const casa::Quantum<double> refLat(refVal[1], "rad");
   const casa::MVDirection out(refLon, refLat);
   return out;
}

/// @brief Conversion helper function
/// @details Copies in to out expanding double into complex values and
/// padding appropriately if necessary (itsPaddingFactor is more than 1)
/// @param[out] out complex output array
/// @param[in] in double input array
/// @param[in] padding padding factor
void TableVisGridder::toComplex(casa::Array<casa::Complex>& out,
		const casa::Array<double>& in, const int padding) {
	casa::IPosition outShape(in.shape());
    // effectively its in.shape
	const int nx=outShape(0);
	const int ny=outShape(1);

	ASKAPDEBUGASSERT(outShape.nelements()>=2);
	outShape(0) *= padding;
	outShape(1) *= padding;
	out.resize(outShape);
	
	const int xOffset = (outShape(0) - in.shape()(0))/2;
	const int yOffset = (outShape(1) - in.shape()(1))/2;


	casa::ReadOnlyArrayIterator<double> inIt(in, 2);
	casa::ArrayIterator<casa::Complex> outIt(out, 2);
	while (!inIt.pastEnd()&&!outIt.pastEnd()) {
		casa::Matrix<double> inMat(inIt.array());
		casa::Matrix<casa::Complex> outMat(outIt.array());
		for (int iy=0; iy<ny; iy++) {
			for (int ix=0; ix<nx; ix++) {
				outMat(ix + xOffset, iy + yOffset)=casa::Complex(float(inMat(ix,iy)));
			}
		}
		inIt.next();
		outIt.next();
	}
}

/// @brief Conversion helper function
/// @details Copies real part of in into double array and
/// extracting an inner rectangle if necessary (itsPaddingFactor is more than 1)
/// @param[out] out real output array
/// @param[in] in complex input array
/// @param[in] padding padding factor      
void TableVisGridder::toDouble(casa::Array<double>& out,
		const casa::Array<casa::Complex>& in, const int padding) {
	casa::IPosition outShape(in.shape());
	ASKAPDEBUGASSERT(outShape.nelements()>=2);
	outShape(0) /= padding;
	outShape(1) /= padding;
	out.resize(outShape);
	const int nx=outShape(0);
	const int ny=outShape(1);
	
	const int xOffset = (in.shape()(0) - outShape(0))/2;
	const int yOffset = (in.shape()(1) - outShape(1))/2;
	
	casa::ReadOnlyArrayIterator<casa::Complex> inIt(in, 2);
	casa::ArrayIterator<double> outIt(out, 2);
	while (!inIt.pastEnd()&&!outIt.pastEnd()) {
		casa::Matrix<casa::Complex> inMat(inIt.array());
		casa::Matrix<double> outMat(outIt.array());
		for (int iy=0; iy<ny; ++iy) {
			for (int ix=0; ix<nx; ++ix) {
				outMat(ix, iy)=double(casa::real(inMat(ix + xOffset,iy + yOffset)));
			}
		}
		inIt.next();
		outIt.next();
	}
}

/// @brief set up itsStokes using the information from itsAxes and itsShape
void TableVisGridder::initStokes()
{
   const int nPol = itsShape.nelements()>=3 ? itsShape[2] : 1;
   if (itsAxes.has("STOKES")) {
       itsStokes = itsAxes.stokesAxis();
   } else {
       itsStokes.resize(1);
       itsStokes[0] = casa::Stokes::I;
   }
   ASKAPCHECK(int(itsStokes.nelements()) == nPol, "Stokes axis is not consistent with the shape of the grid. There are "<<
              nPol<<" planes in the grid and "<<itsStokes.nelements()<<
              " polarisation descriptors defined by the STOKES axis");
}


void TableVisGridder::initialiseGrid(const scimath::Axes& axes,
		const casa::IPosition& shape, const bool dopsf) {
	itsShape=shape;
	ASKAPDEBUGASSERT(shape.nelements()>=2);
	itsShape(0) *= itsPaddingFactor;
	itsShape(1) *= itsPaddingFactor;

	initialiseCellSize(axes);
	
	initStokes();
		
	configureForPSF(dopsf);

	/// We only need one grid
	itsGrid.resize(1);
	itsGrid[0].resize(itsShape);
	itsGrid[0].set(0.0);
	if (isPSFGridder()) {
		// for a proper PSF calculation
		initRepresentativeFieldAndFeed();
	}
	
	initialiseSumOfWeights();
    ASKAPCHECK(itsSumWeights.nelements()>0, "Sum of weights not yet initialised");
    initialiseFreqMapping();
    ASKAPLOG_INFO_STR(logger, "Gridding is set up with tangent centre "<<
             printDirection(getTangentPoint())<<" and image centre "<<
             printDirection(getImageCentre())); 
}

/// @brief helper method to set up cell size
/// @details Similar action is required to calculate uv-cell size for gridding and degridding.
/// Moreover, derived gridders may override initialiseGrid and initialiseDegrid and we don't want
/// to duplicate the code up there. This method calculates uv-cell size for both ra and dec axes
/// using coordinate information provided. This method also assigns passed axes parameter to itsAxes.
/// @param[in] axes coordinate system (ra and dec axes are used).
void TableVisGridder::initialiseCellSize(const scimath::Axes& axes)
{
    itsAxes=axes;
    ASKAPCHECK(itsAxes.hasDirection(), "Direction axis is missing. itsAxes:"<<itsAxes);
    casa::Vector<casa::Double> increments = itsAxes.directionAxis().increment();
    ASKAPCHECK(increments.nelements() == 2, "Expect 2 elements in the increment vector, you have "<<increments);
    itsUVCellSize.resize(2);
    ASKAPDEBUGASSERT(itsShape.nelements()>=2);
    for (size_t dim = 0; dim<2; ++dim) {
         itsUVCellSize[dim] = 1./(increments[dim]*double(itsShape[dim]));
    }
}


/// @brief initialise sum of weights
/// @details We keep track the number of times each convolution function is used per
/// channel and polarisation (sum of weights). This method is made virtual to be able
/// to do gridder specific initialisation without overriding initialiseGrid.
/// This method accepts no parameters as itsShape, itsNWPlanes, etc should have already
/// been initialised by the time this method is called.
void TableVisGridder::initialiseSumOfWeights()
{
  itsSumWeights.resize(1,itsShape.nelements()>=3 ? itsShape(2) : 1, 
                         itsShape.nelements()>=4 ? itsShape(3) : 1);
  itsSumWeights.set(0.0);
}


/// @brief a helper method to initialize gridding of the PSF
/// @details The PSF is calculated using the data for a
/// representative field/feed only. By default, the first encountered
/// feed/field is chosen. If the same gridder is reused for another
/// sequence of data points a new representative feed/field have to be
/// found. This is done by resetting the cache in initialiseGrid. However,
/// the latter method can be overridden in the derived classes. To avoid
/// a duplication of the code, this helper method resets the representative
/// feed/field cache. It is called from initialiseGrid.
void TableVisGridder::initRepresentativeFieldAndFeed()
{
  itsFirstGriddedVis = true;

  /*
  // temporary code for debuggig
  std::cout<<"TableVisGridder::initRepresentativeFieldAndFeed"<<std::endl;
  itsFirstGriddedVis = false;
  casa::Quantity ra(0.,"rad"), dec(0.,"rad");
  casa::MVAngle::read(ra,"13:32:22.48");
  casa::MVAngle::read(dec,"-042.16.56.93");
  itsPointingUsedForPSF = casa::MVDirection(ra,dec);
  ASKAPLOG_INFO_STR(logger, "Field override for PSF, will use "<<printDirection(itsPointingUsedForPSF));
  itsFeedUsedForPSF = 0;
  // end of temporary code
  */
}

/// This is the default implementation
void TableVisGridder::finaliseGrid(casa::Array<double>& out) {
	ASKAPDEBUGASSERT(itsGrid.size() > 0);
	// buffer for result as doubles
	casa::Array<double> dBuffer(itsGrid[0].shape());
	ASKAPDEBUGASSERT(dBuffer.shape().nelements()>=2);

    /// Loop over all grids Fourier transforming and accumulating
	for (unsigned int i=0; i<itsGrid.size(); i++) {
		casa::Array<casa::Complex> scratch(itsGrid[i].copy());
		fft2d(scratch, false);
		if (i==0) {
			toDouble(dBuffer, scratch);
		} else {
			casa::Array<double> work(dBuffer.shape());
			toDouble(work, scratch);
			dBuffer+=work;
		}
	}
	// Now we can do the convolution correction
	correctConvolution(dBuffer);
	dBuffer*=double(dBuffer.shape()(0))*double(dBuffer.shape()(1));
	out = scimath::PaddingUtils::extract(dBuffer,itsPaddingFactor);
}


/// This is the default implementation
void TableVisGridder::finaliseWeights(casa::Array<double>& out) {
	ASKAPDEBUGASSERT(itsShape.nelements() >= 4);

	int nPol=itsShape(2);
	int nChan=itsShape(3);

	ASKAPCHECK(itsSumWeights.nelements()>0, "Sum of weights not yet initialised");
	int nZ=itsSumWeights.shape()(0);

	for (int chan=0; chan<nChan; chan++) {
		for (int pol=0; pol<nPol; pol++) {
			double sumwt=0.0;
			for (int iz=0; iz<nZ; iz++) {
			  //			  float sumConvFunc=real(casa::sum(casa::abs(itsConvFunc[iz])));
			  //			  ASKAPLOG_INFO_STR(logger, "Sum of conv func " << sumConvFunc);
				sumwt+=itsSumWeights(iz, pol, chan);
			}
			ASKAPDEBUGASSERT(out.shape().nelements() == 4);
			casa::IPosition ipStart(4, 0, 0, pol, chan);
			casa::IPosition onePlane(4, out.shape()(0), out.shape()(1), 1, 1);
			casa::Slicer slicer(ipStart, onePlane);
			out(slicer).set(sumwt);
		}
	}
}

void TableVisGridder::initialiseDegrid(const scimath::Axes& axes,
		const casa::Array<double>& in) {
    configureForPSF(false);
	itsShape = scimath::PaddingUtils::paddedShape(in.shape(),itsPaddingFactor);

	initialiseCellSize(axes);
    initStokes();

    initialiseFreqMapping();
 
	/// We only need one grid
	itsGrid.resize(1);
	itsGrid[0].resize(itsShape);

	if (casa::max(casa::abs(in))>0.0) {
		itsModelIsEmpty=false;
		casa::Array<double> scratch(itsShape);
		scimath::PaddingUtils::extract(scratch, itsPaddingFactor) = in;
		correctConvolution(scratch);
		toComplex(itsGrid[0], scratch);
		fft2d(itsGrid[0], true);
	} else {
		ASKAPLOG_INFO_STR(logger, "No need to degrid: model is empty");
		itsModelIsEmpty=true;
		itsGrid[0].set(casa::Complex(0.0));
	}
}

/// @brief helper method to initialise frequency mapping
/// @details Derived gridders may override initialiseGrid and initialiseDegrid. Howerver, 
/// they still need to be able to initialise frequency axis mapping (between accessor channels
/// and image cube), which is handled by a private member class. This method initialises the 
/// mapper using the content of itsShape and itsAxes, which should be set prior to calling this
/// method.
void TableVisGridder::initialiseFreqMapping()
{
  if (itsAxes.has("FREQUENCY") && itsShape.nelements()>=4) {
      itsFreqMapper.setupImage(itsAxes, itsShape(3));
  } else {
      ASKAPLOG_INFO_STR(logger, "Forced to use single spectral plane gridding (either "
                                "FREQUENCY axis or the number of channels are missing");
      itsFreqMapper.setupSinglePlaneGridding();
  }
}

/// This is the default implementation
void TableVisGridder::finaliseDegrid() {
	/// Nothing to do
}

// This ShPtr should get deep-copied during cloning.
void TableVisGridder::initVisWeights(IVisWeights::ShPtr viswt)
{
	itsVisWeight = viswt;
}

// Customize for the specific type of Visibility weight.
// Input string is whatever is after "image" => "image.i.0.xxx" gives ".i.0.xxx "
void TableVisGridder::customiseForContext(casa::String context)
{

	// RVU : Set up model dependant gridder behaviour
	//       For MFS, gridders for each Taylor term need different VisWeights.
	//  parse the 'context' string, and generate the "order" parameter.
  ASKAPLOG_INFO_STR(logger, "Customising gridder for context " << context);
	/*
	char corder[2];
	corder[0] = *(context.data()+3); // read the fourth character to get the order of the Taylor coefficient.
	corder[1] = '\n';
	int order = atoi(corder);
	//	ASKAPLOG_INFO_STR(logger, "Customising gridder for context " << context
	//			  << " corder " << corder << " order" << order);
	if(order <0 || order >9) order = 0;
	*/
	// MV: more general version
	const ImageParamsHelper iph(context);
	const int order = iph.isTaylorTerm() ? iph.order() : 1;
	
	if(itsVisWeight) {
		itsVisWeight->setParameters(order);
    }
}



/// This is the default implementation
int TableVisGridder::cIndex(int row, int pol, int chan) {
	return 0;
}

/// This is the default implementation
int TableVisGridder::gIndex(int row, int pol, int chan) {
	return 0;
}

}

}
