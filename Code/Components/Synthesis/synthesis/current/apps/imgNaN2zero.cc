/// @file imgNaN2zero.cc
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

#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/images/Images/PagedImage.h>
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
using namespace std;

void printDirection(ostream &os,const casacore::MDirection &dir)  {
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
     // command line parameter
	 cmdlineparser::GenericParameter<std::string> imgfile;
	 parser.add(imgfile);

	 // I hope const_cast is temporary here
	 parser.process(argc, const_cast<char**> (argv));
     casacore::PagedImage<casacore::Float> img(imgfile.getValue());
     const casacore::IPosition shape = img.shape();
     ASKAPASSERT(shape.nelements()>=2);
     casacore::IPosition curpos(shape.nelements(),0);
     casacore::uInt nElements = shape.product();
     casacore::uInt countNaNs = 0;
     for (casacore::uInt i=0;i<nElements;++i) {
          float val = img(curpos);
          if (std::isnan(val)) {
              val = 0;
              ++countNaNs;
          }
          img.putAt(val,curpos);
          for (int dim=0;dim<int(shape.nelements());++dim) {
               ++curpos[dim];
               if (curpos[dim]<shape[dim]) {
                   break;
               }
               curpos[dim] = 0;
          }
     }
     std::cout<<"Replaced "<<countNaNs<<" NaNs"<<std::endl;
  }
  ///==============================================================================
  catch (const cmdlineparser::XParser &ex) {
	 std::cerr << "Usage: " << argv[0] << " imagefile"
			<< std::endl;
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
