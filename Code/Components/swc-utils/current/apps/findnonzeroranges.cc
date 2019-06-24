/// @file
/// @brief an utility to extract channel ranges with non-zero signal (after some thresholding)
/// @details It is intended to be used for debugging of the frequency mapping
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


#include <dataaccess/TableDataSource.h>
#include <askap_accessors.h>
#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapUtil.h>
ASKAP_LOGGER(logger, "");

#include <askap/askap/AskapError.h>
#include <dataaccess/SharedIter.h>

#include <dataaccess/TableManager.h>
#include <dataaccess/IDataConverterImpl.h>
#include <swcorrelator/BasicMonitor.h>

// casa
#include <casacore/measures/Measures/MFrequency.h>
#include <casacore/tables/Tables/Table.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/casa/Arrays/MatrixMath.h>



// std
#include <stdexcept>
#include <iostream>
#include <vector>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;

using namespace askap;
using namespace askap::accessors;

casacore::Matrix<casacore::Complex> flagOutliers(const casacore::Matrix<casacore::Complex> &in) {
  //return in;
  casacore::Matrix<casacore::Complex> result(in);
  for (casacore::uInt row=0;row<result.nrow(); ++row) {
       for (casacore::uInt col=0; col<result.ncolumn(); ++col) {
            if (casacore::abs(result(row,col))>2e6) {
                result(row,col) = 0.;
            }
       }
  }
  return result;
}


void process(const IConstDataSource &ds) {
  IDataSelectorPtr sel=ds.createSelector();
  sel->chooseFeed(0);
  sel->chooseCrossCorrelations();
  //sel->chooseAutoCorrelations();
  IDataConverterPtr conv=ds.createConverter();  
  conv->setFrequencyFrame(casacore::MFrequency::Ref(casacore::MFrequency::TOPO),"MHz");
  conv->setEpochFrame(casacore::MEpoch(casacore::Quantity(55913.0,"d"),
                      casacore::MEpoch::Ref(casacore::MEpoch::UTC)),"s");
  conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));
  
  casacore::uInt cycle = 0;
  
  std::ofstream os("chranges.dat");
  std::vector<casacore::uInt> starts;
  std::vector<casacore::uInt> stops;
                      
  casacore::Vector<double> freq;
  size_t counter = 0;
  size_t nGoodRows = 0;
  size_t nBadRows = 0;
  casacore::uInt nChan = 0;
  casacore::uInt nRow = 0;
  double startTime = 0;
  double stopTime = 0;
  
  casacore::Vector<casacore::uInt> ant1ids, ant2ids;
  
  casacore::uInt prevCycle = 0;  
  for (IConstDataSharedIter it=ds.createConstIterator(sel,conv);it!=it.end();++it) {
       ++cycle;
                
       if (nChan == 0) {
           nChan = it->nChannel();
           nRow = it->nRow();
           freq = it->frequency();
           ant1ids = it->antenna1();
           ant2ids = it->antenna2();
           startTime = it->time();
           std::cout<<"Baseline order is as follows: "<<std::endl;
           for (casacore::uInt row = 0; row<nRow; ++row) {
                std::cout<<"baseline (1-based) = "<<row+1<<" is "<<ant1ids[row]<<" - "<<ant2ids[row]<<std::endl; 
           }           
       } else { 
           ASKAPCHECK(nChan == it->nChannel(), 
                  "Number of channels seem to have been changed, previously "<<nChan<<" now "<<it->nChannel());
           if (nRow != it->nRow()) {
               std::cerr<<"Number of rows changed at time="<<(it->time() - startTime)/60.<<
                          " min, was "<<nRow<<" now "<<it->nRow()<<std::endl;
               continue;
           }
           ASKAPCHECK(nRow == it->nRow(), 
                  "Number of rows seem to have been changed, previously "<<nRow<<" now "<<it->nRow());
       }
       
       ASKAPASSERT(it->nPol() >= 1);
       ASKAPASSERT(it->nChannel() > 1);
       // check that the products come in consistent way across the interations
       for (casacore::uInt row = 0; row<nRow; ++row) {
            ASKAPCHECK(it->antenna1()[row] == ant1ids[row], "Inconsistent antenna 1 ids at row = "<<row);
            ASKAPCHECK(it->antenna2()[row] == ant2ids[row], "Inconsistent antenna 2 ids at row = "<<row);             
       }
       
       for (casacore::uInt row=0; row<nRow; ++row) {  
            casacore::Vector<casacore::Bool> flags = it->flag().xyPlane(0).row(row);
            bool flagged = false;
            for (casacore::uInt ch = 0; ch < flags.nelements(); ++ch) {
                 flagged |= flags[ch];
            }            
            
            casacore::Matrix<casacore::Complex> allChan = flagOutliers(it->visibility().xyPlane(0));
            casacore::Vector<casacore::Complex> measuredRow = allChan.row(row);
            
            
            // flagging based on the amplitude (to remove extreme outliers)
            //casacore::Complex currentAvgVis = casacore::sum(measuredRow) / float(it->nChannel());
            
            /*
            if ((casacore::abs(currentAvgVis) > 0.5) && (row % 3 == 2)) {
                flagged = true;
            } 
            */
            
            /*
            // optional flagging based on time-range
            if ((counter>1) && ((it->time() - startTime)/60.>1050.)) {
                flagged = true;
            }
            */
            
            /*
            // uncomment to store the actual amplitude time-series
            if ((counter>1) && (row % 3 == 0)) {
                os2<<counter<<" "<<(it->time() - startTime)/60.<<" "<<casacore::abs(currentAvgVis)<<std::endl;
            }
            */
            //
            
            if (flagged) {
               ++nBadRows;
            } else {
                ++nGoodRows;
                if ((ant1ids[row] != 0) || (ant2ids[row] != 1)) {
                    continue;
                }
                std::vector<casacore::uInt> newStarts;
                std::vector<casacore::uInt> newStops;
                int prevCh = -1;
                ASKAPDEBUGASSERT(measuredRow.nelements()>1);
                for (casacore::uInt ch=0; ch<measuredRow.nelements(); ++ch) {
                   if (casacore::abs(measuredRow[ch])>1e5) {
                       if (prevCh == -1) {
                           prevCh = int(ch);
                       }
                   } else {
                       if (prevCh != -1) {
                           newStarts.push_back(casacore::uInt(prevCh));
                           newStops.push_back(ch-1);
                           prevCh = -1;
                       }
                   }
                }
                if (prevCh != -1) {
                    newStarts.push_back(casacore::uInt(prevCh));
                    newStops.push_back(measuredRow.nelements()-1);
                }
                bool rangesChanged = ((newStarts.size() != starts.size()) || (newStops.size() != stops.size()));
                if (!rangesChanged) {
                    // check actual channel ranges
                    ASKAPDEBUGASSERT(starts.size() == stops.size());
                    for (size_t rng = 0; rng<starts.size(); ++rng) {
                         if ((starts[rng] != newStarts[rng]) || (stops[rng] != newStops[rng])) {
                             rangesChanged = true;
                             break;
                         }
                    }
                }
                
                if (rangesChanged) {
                    starts = newStarts;
                    stops = newStops;
                    if (starts.size() > 1) {
                        os<<cycle<<" "<<it->time()-startTime<<" "<<cycle-prevCycle;
                        for (size_t rng = 0; rng<starts.size(); ++rng) {
                             os<<" "<<starts[rng]<<" "<<stops[rng]<<" "<<stops[rng] - starts[rng] + 1;
                        }
                        os<<std::endl;
                    }
                    prevCycle = cycle;
                }
            }
       }
       if ((counter == 0) && (nGoodRows == 0)) {
           // all data are flagged, completely ignoring this iteration and consider the next one to be first
           nChan = 0;
           continue;
       }
       /*
       // optionally reset integration to provide multiple chunks integrated 
       if ((counter>1) && ((it->time() - startTime)/60. >= 29.9999999)) {
           counter = 0;
           nChan = 0;
       } 
       //
       */
      
       if (++counter == 1) {
           startTime = it->time();
       }
       stopTime = it->time() + 1; // 1s integration time is hardcoded
  }
 std::cout<<"Iterated over "<<counter<<" integration cycles, "<<nGoodRows<<" good and "<<nBadRows<<" bad rows, time span "<<(stopTime-startTime)/60.<<" minutues"<<std::endl;
}


int main(int argc, char **argv) {
  try {
     if (argc!=2) {
         cerr<<"Usage: "<<argv[0]<<" measurement_set"<<endl;
         return -2;
     }

     casacore::Timer timer;
     const std::string msName = argv[argc - 1];
     
     timer.mark();
     TableDataSource ds(msName,TableDataSource::MEMORY_BUFFERS);     
     std::cerr<<"Initialization: "<<timer.real()<<std::endl;
     timer.mark();
     process(ds);
     std::cerr<<"Job: "<<timer.real()<<std::endl;
     
  }
  catch(const AskapError &ce) {
     cerr<<"AskapError has been caught. "<<ce.what()<<endl;
     return -1;
  }
  catch(const std::exception &ex) {
     cerr<<"std::exception has been caught. "<<ex.what()<<endl;
     return -1;
  }
  catch(...) {
     cerr<<"An unexpected exception has been caught"<<endl;
     return -1;
  }
  return 0;
}
