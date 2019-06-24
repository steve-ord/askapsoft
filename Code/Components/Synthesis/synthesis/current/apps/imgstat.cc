///
/// @file : statistics of the image
///
/// This program is intended to be used in scripts (to extract statistics from an image)
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/images/Images/PagedImage.h>
#include <casacore/images/Images/SubImage.h>
#include <casacore/images/Images/ImageStatistics.h>
#include <CommandLineParser.h>
#include <askap/askap/AskapError.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/Coordinate.h>
#include <casacore/casa/Quanta/MVAngle.h>
#include <casacore/casa/Quanta/Quantum.h>
#include <casacore/measures/Measures/MDirection.h>
#include <casacore/casa/Quanta/MVDirection.h>


#include <stdexcept>
#include <iostream>

using namespace askap;

void printDirection(std::ostream &os,const casacore::MDirection &dir)  {
    double lngbuf=dir.getValue().getLong("deg").getValue();
    if (lngbuf<0) lngbuf+=360.;
    os<<(dir.getRefString()!="GALACTIC"?casacore::MVAngle::Format(casacore::MVAngle::TIME):
          casacore::MVAngle::Format(casacore::MVAngle::ANGLE))<<casacore::MVAngle(casacore::Quantity(lngbuf,"deg"))<<" "<<
          casacore::MVAngle(dir.getValue().getLat("deg"))<<
          " ("<<dir.getRefString()<<")";
}


// Main function
int main(int argc, const char** argv) { 
  try {
     cmdlineparser::Parser parser; // a command line parser
	 // command line parameters
	 cmdlineparser::FlagParameter doWtStats("-w");      
	 cmdlineparser::GenericParameter<std::string> imgfile;
	 parser.add(doWtStats,cmdlineparser::Parser::return_default);
	 parser.add(imgfile);

	 // I hope const_cast is temporary here
	 parser.process(argc, const_cast<char**> (argv));
     casacore::PagedImage<casacore::Float> img(imgfile.getValue());
     casacore::ImageStatistics<casacore::Float> imstat(img, casacore::False);
     float tmin,tmax;
     imstat.getFullMinMax(tmin,tmax);
     casacore::IPosition minPos,maxPos;
     imstat.getMinMaxPos(minPos,maxPos);
     casacore::Int direction_coordinate = img.coordinates().findCoordinate(casacore::Coordinate::DIRECTION);
     ASKAPASSERT(direction_coordinate>=0);
     ASKAPASSERT(maxPos.nelements()>=2);
     const casacore::DirectionCoordinate &dc = img.coordinates().directionCoordinate(direction_coordinate);
     casacore::Vector<casacore::Double> pixel(2);
     pixel(0)=casacore::Double(maxPos[0]);
     pixel(1)=casacore::Double(maxPos[1]);
     casacore::MDirection res;
     ASKAPASSERT(dc.toWorld(res,pixel));
     
     // print peak in the image and position of the peak 
     std::cout<<tmax<<" ";
     printDirection(std::cout,res);
     std::cout<<" # Max RA Dec (Epoch)"<<std::endl;
     
     std::cout<<std::setprecision(15)<<res.getValue().getLong("deg").getValue()<<" "<<
                res.getValue().getLat("deg").getValue()<<" # RA DEC"<<std::endl;
     
     casacore::Array<float> statBuf;
     if (imstat.getConvertedStatistic(statBuf,casacore::LatticeStatsBase::RMS)) {
         casacore::Vector<float> statVec(statBuf.reform(casacore::IPosition(1,statBuf.nelements())));
         ASKAPCHECK(statVec.nelements() == 1, "Expect exactly one element in the array returned by getConvertedStatistics; you have: "<<statVec);         
         std::cout<<statVec[0]<<" ";
     }
     if (imstat.getConvertedStatistic(statBuf,casacore::LatticeStatsBase::MEDIAN)) {
         casacore::Vector<float> statVec(statBuf.reform(casacore::IPosition(1,statBuf.nelements())));
         ASKAPCHECK(statVec.nelements() == 1, "Expect exactly one element in the array returned by getConvertedStatistics; you have: "<<statVec);         
         std::cout<<statVec[0]<<" # RMS MEDIAN"<<std::endl;
     }
     if (doWtStats.defined()) {
         // making a slice to get inner quarter
         const casacore::IPosition shape = img.shape();
         ASKAPCHECK(shape.nelements() >= 2, "Need 2D images for the '-w' option");
         casacore::IPosition blc(shape.nelements(),0);
         casacore::IPosition trc(shape);
         for (size_t dim = 0; dim<trc.nelements(); ++dim) {
              --trc[dim];
              if (dim>=2) {
                  trc[dim] = 0;
              } 
         }
         blc[0] = shape[0]/4;
         blc[1] = shape[1]/4;
         trc[0] = 3*blc[0];
         trc[1] = 3*blc[1];
         ASKAPCHECK(blc[0]>=0 && blc[1]>=0, "BLC is negative: "<<blc<<", shape="<<shape);
         ASKAPCHECK(trc[1]<shape[1] && trc[0]<shape[0], "TRC extends beyond the edge: "<<trc<<", shape="<<shape<<" blc="<<blc);
         casacore::Slicer slc(blc,trc,casacore::IPosition(shape.nelements(),1),casacore::Slicer::endIsLast);
         casacore::SubImage<casacore::Float> si(img,slc,casacore::AxesSpecifier(casacore::True));
         casacore::ImageStatistics<casacore::Float> imStatWt(si, casacore::False);
         imStatWt.getFullMinMax(tmin,tmax);
         std::cout<<tmax<<" "<<tmin<<" # MAX MIN in the inner quarter"<<std::endl; 
     }     
  }
  ///==============================================================================
  catch (const cmdlineparser::XParser &ex) {
	 std::cerr << "Usage: " << argv[0] << " [-w] imagefile"
			<< std::endl<<
			"  -w print min/max of the inner quarter (useful for weights analysis)"<<std::endl;
  }

  catch (const askap::AskapError& x) {
     std::cerr << "Askap error in " << argv[0] << ": " << x.what()
        << std::endl;
     exit(1);
  } 
  catch (const std::exception& x) {
	 std::cerr << "Unexpected exception in " << argv[0] << ": " << x.what()
			<< std::endl;
	 exit(1);
  }
  exit(0);  
}
