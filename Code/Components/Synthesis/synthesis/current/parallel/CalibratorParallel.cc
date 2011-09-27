/// @file
///
/// CalibratorParallel: Support for parallel applications using the measurement 
/// equation classes. This code applies to calibration. I expect that this part
/// will be redesigned in the future for a better separation of the algorithm
/// from the parallel framework middleware. Current version is basically an
/// adapted ImagerParallel clas
///
/// Performs calibration on a data source. Can run in serial or 
/// parallel (MPI) mode.
///
/// The data are accessed from the DataSource. This is and will probably remain
/// disk based. The images are kept purely in memory until the end.
///
/// Control parameters are passed in from a LOFAR ParameterSet file.
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
///

// Include own header file first
#include <parallel/CalibratorParallel.h>

// System includes
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// logging stuff
#include <askap_synthesis.h>
#include <askap/AskapLogging.h>
ASKAP_LOGGER(logger, ".parallel");

// own includes
#include <askap/AskapError.h>
#include <askap/AskapUtil.h>

#include <dataaccess/TableDataSource.h>
#include <dataaccess/ParsetInterface.h>
#include <dataaccess/TimeChunkIteratorAdapter.h>

#include <fitting/LinearSolver.h>
#include <fitting/GenericNormalEquations.h>
#include <fitting/Params.h>

#include <measurementequation/ImageFFTEquation.h>
#include <measurementequation/SynthesisParamsHelper.h>
#include <measurementequation/MEParsetInterface.h>
#include <measurementequation/CalibrationME.h>
#include <measurementequation/PreAvgCalMEBase.h>
#include <measurementequation/ComponentEquation.h>
#include <measurementequation/NoXPolGain.h>
#include <measurementequation/LeakageTerm.h>
#include <measurementequation/Product.h>
#include <measurementequation/ImagingEquationAdapter.h>
#include <gridding/VisGridderFactory.h>
#include <mwcommon/AskapParallel.h>
#include <Common/ParameterSet.h>
#include <calibaccess/CalParamNameHelper.h>
#include <calibaccess/CalibAccessFactory.h>


// casa includes
#include <casa/aips.h>
#include <casa/OS/Timer.h>

using namespace askap;
using namespace askap::scimath;
using namespace askap::synthesis;
using namespace askap::accessors;
using namespace askap::mwcommon;

/// @brief Constructor from ParameterSet
/// @details The parset is used to construct the internal state. We could
/// also support construction from a python dictionary (for example).
/// The command line inputs are needed solely for MPI - currently no
/// application specific information is passed on the command line.
/// @param[in] parset ParameterSet for inputs
CalibratorParallel::CalibratorParallel(askap::mwcommon::AskapParallel& comms,
        const LOFAR::ParameterSet& parset) :
      MEParallelApp(comms,parset), 
      itsPerfectModel(new scimath::Params()), itsSolveGains(false), itsSolveLeakage(false),
      itsSolutionInterval(-1.)
{  
  const std::string what2solve = parset.getString("solve","gains");
  if (what2solve.find("gains") != std::string::npos) {
      ASKAPLOG_INFO_STR(logger, "Gains will be solved for (solve='"<<what2solve<<"')");
      itsSolveGains = true;
  }
  if (what2solve.find("leakages") != std::string::npos) {
      ASKAPLOG_INFO_STR(logger, "Leakages will be solved for (solve='"<<what2solve<<"')");
      itsSolveLeakage = true;
  }
  ASKAPCHECK(itsSolveGains || itsSolveLeakage, 
      "Nothing to solve! Either gains or leakages (or both) have to be solved for, you specified solve='"<<
      what2solve<<"'");
  
  init(parset);
  if (itsComms.isMaster()) {
                  
      /// Create the solver  
      itsSolver.reset(new LinearSolver);
      ASKAPCHECK(itsSolver, "Solver not defined correctly");
      itsRefGain = parset.getString("refgain","");
      
      // setup solution source (or sink to be exact, because we're writing the soltion here)
      itsSolutionSource = CalibAccessFactory::rwCalSolutionSource(parset);
      ASKAPASSERT(itsSolutionSource);
  }
  if (itsComms.isWorker()) {
      // load sky model, populate itsPerfectModel
      readModels();
      itsSolutionInterval = SynthesisParamsHelper::convertQuantity(parset.getString("interval","-1s"), "s");
      if (itsSolutionInterval < 0) {
          ASKAPLOG_INFO_STR(logger, "A single solution will be made for the whole duration of the dataset");
      } else {
          ASKAPLOG_INFO_STR(logger, "Solution will be made for each "<<itsSolutionInterval<<" seconds chunk of the dataset");
      }
  }
}

/// @brief initalise measurement equation and model
/// @details This method is indended to be called if this object is reused to
/// get more than one solution. It initialises the model and normal equations.
/// It is called from constructor, so if only one solution is required the constructor
/// is sufficient.
/// @param[in] parset ParameterSet for inputs
void CalibratorParallel::init(const LOFAR::ParameterSet& parset)
{
  if (itsComms.isMaster()) {
      ASKAPDEBUGASSERT(itsModel); // should be initialized in SynParallel
      itsModel->reset();

      // initial assumption of the parameters
      const casa::uInt nAnt = parset.getInt32("nAnt",36); // 28  
      const casa::uInt nBeam = parset.getInt32("nBeam",1); 
      if (itsSolveGains) {
          ASKAPLOG_INFO_STR(logger, "Initialise gains (unknowns) for "<<nAnt<<" antennas and "<<nBeam<<" beam(s).");
          for (casa::uInt ant = 0; ant<nAnt; ++ant) {
               for (casa::uInt beam = 0; beam<nBeam; ++beam) {
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::XX), casa::Complex(1.,0.));
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::YY),casa::Complex(1.,0.));
               }
          }
      }
      if (itsSolveLeakage) {
          ASKAPLOG_INFO_STR(logger, "Initialise leakages (unknowns) for "<<nAnt<<" antennas and "<<nBeam<<" beam(s).");
          for (casa::uInt ant = 0; ant<nAnt; ++ant) {
               for (casa::uInt beam = 0; beam<nBeam; ++beam) {
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::XY),casa::Complex(0.,0.));
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::YX),casa::Complex(0.,0.));
               }
          }
      }
  }
  if (itsComms.isWorker()) {
      // a greater reuse of the measurement equation could probably be achieved as the sky model didn't change
      itsEquation.reset();
      if (itsIteratorAdapter) {
          // this initialisation is done before resuming the iteration
          ASKAPDEBUGASSERT(itsIteratorAdapter->moreDataAvailable());
          ASKAPDEBUGASSERT(!itsIteratorAdapter->hasMore());
          itsIteratorAdapter->resume();
      }
  }
}


void CalibratorParallel::calcOne(const std::string& ms, bool discard)
{
  casa::Timer timer;
  timer.mark();
  ASKAPLOG_INFO_STR(logger, "Calculating normal equations for " << ms );
  // First time around we need to generate the equation 
  if ((!itsEquation) || discard) {
      ASKAPLOG_INFO_STR(logger, "Creating measurement equation" );
      if (!itsIteratorAdapter) {
          ASKAPLOG_INFO_STR(logger, "Creating iterator over data" );
          TableDataSource ds(ms, TableDataSource::DEFAULT, dataColumn());
          ds.configureUVWMachineCache(uvwMachineCacheSize(),uvwMachineCacheTolerance());      
          IDataSelectorPtr sel=ds.createSelector();
          sel << parset();
          IDataConverterPtr conv=ds.createConverter();
          conv->setFrequencyFrame(getFreqRefFrame(), "Hz");
          conv->setDirectionFrame(casa::MDirection::Ref(casa::MDirection::J2000));
          // ensure that time is counted in seconds since 0 MJD
          conv->setEpochFrame();
          //IDataSharedIter it=ds.createIterator(sel, conv);
          itsIteratorAdapter.reset(new accessors::TimeChunkIteratorAdapter(ds.createIterator(sel, conv), itsSolutionInterval));
          if (itsSolutionInterval >= 0) {
              ASKAPLOG_INFO_STR(logger, "Iterator has been created, solution interval = "<<itsSolutionInterval<<" s");
          } else {
              ASKAPLOG_INFO_STR(logger, "Iterator has been created, infinite solution interval");
          }
      }
      ASKAPDEBUGASSERT(itsIteratorAdapter);
      IDataSharedIter it(itsIteratorAdapter);
      
      ASKAPCHECK(itsPerfectModel, "Uncorrupted model not defined");
      ASKAPCHECK(itsModel, "Initial assumption of parameters is not defined");
      ASKAPCHECK(gridder(), "Gridder not defined");
      if (SynthesisParamsHelper::hasImage(itsPerfectModel)) {
         ASKAPCHECK(!SynthesisParamsHelper::hasComponent(itsPerfectModel),
                 "Image + component case has not yet been implemented");
         // have to create an image-specific equation        
         boost::shared_ptr<ImagingEquationAdapter> ieAdapter(new ImagingEquationAdapter);
         ieAdapter->assign<ImageFFTEquation>(*itsPerfectModel, gridder());
         createCalibrationME(it,ieAdapter);
      } else {
         // model is a number of components, don't need an adapter here
         
         // it doesn't matter which iterator is passed below. It is not used
         boost::shared_ptr<ComponentEquation> 
                  compEq(new ComponentEquation(*itsPerfectModel,it));
         createCalibrationME(it,compEq);         
      }
      ASKAPCHECK(itsEquation, "Equation is not defined");
  } else {
      ASKAPLOG_INFO_STR(logger, "Reusing measurement equation" );
      // we need to update the model held by measurement equation 
      // because it has been cloned at construction
      ASKAPCHECK(itsEquation, "Equation is not defined");
      ASKAPCHECK(itsModel, "Model is not defined");
      itsEquation->setParameters(*itsModel);
  }
  ASKAPCHECK(itsNe, "NormalEquations are not defined");
  itsEquation->calcEquations(*itsNe);
  ASKAPLOG_INFO_STR(logger, "Calculated normal equations for "<< ms << " in "<< timer.real()
                     << " seconds ");
}

/// @brief create measurement equation
/// @details This method initialises itsEquation with shared pointer to a proper type.
/// It uses internal flags to create a correct type (i.e. polarisation calibration or
/// just antenna-based gains). Parameters are passed directly to the constructor of 
/// CalibrationME template.
/// @param[in] dsi data shared iterator 
/// @param[in] perfectME uncorrupted measurement equation
void CalibratorParallel::createCalibrationME(const IDataSharedIter &dsi, 
                const boost::shared_ptr<IMeasurementEquation const> &perfectME)
{
   ASKAPDEBUGASSERT(itsModel);
   // temporary logic while preaveraging is being debugged for polarisation 
   // calibration
   //const bool doPreAveraging = false;
   const bool doPreAveraging = true;
   //const bool doPreAveraging = !itsSolveLeakage;
  if (!doPreAveraging)  {
   
   ASKAPCHECK(itsSolutionInterval < 0, "Time-dependent solutions are supported only with pre-averaging, you have interval = "<<
              itsSolutionInterval<<" seconds");
              
   // the old code without pre-averaging
   if (itsSolveGains && !itsSolveLeakage) {
       itsEquation.reset(new CalibrationME<NoXPolGain>(*itsModel,dsi,perfectME));           
   } else if (itsSolveLeakage && !itsSolveGains) {
       itsEquation.reset(new CalibrationME<LeakageTerm>(*itsModel,dsi,perfectME));           
   } else if (itsSolveLeakage && itsSolveGains) {
       itsEquation.reset(new CalibrationME<Product<NoXPolGain,LeakageTerm> >(*itsModel,dsi,perfectME));           
   } else {
       ASKAPTHROW(AskapError, "Unsupported combination of itsSolveGains and itsSolveLeakage. This shouldn't happen. Verify solve parameter");       
   }
 
  } else {
   
   // code with pre-averaging
   // it is handy to have a shared pointer to the base type because it is
   // not templated
   boost::shared_ptr<PreAvgCalMEBase> preAvgME;
   if (itsSolveGains && !itsSolveLeakage) {
       preAvgME.reset(new CalibrationME<NoXPolGain, PreAvgCalMEBase>());           
   } else if (itsSolveLeakage && !itsSolveGains) {
       preAvgME.reset(new CalibrationME<LeakageTerm, PreAvgCalMEBase>());           
   } else if (itsSolveLeakage && itsSolveGains) {
       preAvgME.reset(new CalibrationME<Product<NoXPolGain,LeakageTerm>, PreAvgCalMEBase>());       
   } else {
       ASKAPTHROW(AskapError, "Unsupported combination of itsSolveGains and itsSolveLeakage. This shouldn't happen. Verify solve parameter");       
   }
   ASKAPDEBUGASSERT(preAvgME);
   preAvgME->accumulate(dsi,perfectME);
   itsEquation = preAvgME;
   // this is just because we bypass setting the model for the first major cycle
   // in the case without pre-averaging
   itsEquation->setParameters(*itsModel);
   // set the next chunk flag, if necessary (time-dependent solution is supported only with pre-averaging)
   setNextChunkFlag(nextChunk());
  }
}

/// Calculate the normal equations for a given measurement set
void CalibratorParallel::calcNE()
{
  // Now we need to recreate the normal equations
  
  // we need to preserve at least one metadata item which is a flag whether we have more
  // data available. This flag is filled at the first iteration and would be overwritten by reset
  // unless we do something. The following code looks ugly, but it is probably the easiest way to
  // achieve what we want
  scimath::Params tempMetadata;
  if (itsNe) {
      boost::shared_ptr<scimath::GenericNormalEquations> gne = boost::dynamic_pointer_cast<scimath::GenericNormalEquations>(itsNe);
      if (gne) {
          tempMetadata = gne->metadata();
      }
  }
  boost::shared_ptr<scimath::GenericNormalEquations> gne(new GenericNormalEquations);
  gne->metadata() = tempMetadata;
  itsNe = gne;

  if (itsComms.isWorker()) {
        
      ASKAPDEBUGASSERT(itsNe);

      if (itsComms.isParallel()) {
          calcOne(measurementSets()[itsComms.rank()-1],false);
          sendNE();
      } else {
          ASKAPCHECK(itsSolver, "Solver not defined correctly");
          // just throw exception for now, although we could've maintained a map of dataset names/iterators
          // to allow reuse of the right iterator
          ASKAPCHECK((itsSolutionInterval < 0) || (measurementSets().size() < 2), 
              "The code currently doesn't support time-dependent solutions for a number of measurement sets in the serial mode");
          //
          itsSolver->init();
          for (size_t iMs=0; iMs<measurementSets().size(); ++iMs) {
            calcOne(measurementSets()[iMs],false);
            itsSolver->addNormalEquations(*itsNe);
          }
      }
  }
}

void CalibratorParallel::solveNE()
{ 
  if (itsComms.isMaster()) {
      // Receive the normal equations
      if (itsComms.isParallel()) {
          receiveNE();
      }
        
      ASKAPLOG_INFO_STR(logger, "Solving normal equations");
      casa::Timer timer;
      timer.mark();
      Quality q;
      ASKAPDEBUGASSERT(itsSolver);
      itsSolver->setAlgorithm("SVD");     
      itsSolver->solveNormalEquations(*itsModel,q);
      ASKAPLOG_INFO_STR(logger, "Solved normal equations in "<< timer.real() << " seconds ");
      ASKAPLOG_INFO_STR(logger, "Solution quality: "<<q);
      if (itsRefGain != "") {
          ASKAPLOG_INFO_STR(logger, "Rotating phases to have that of "<<itsRefGain<<" equal to 0");
          rotatePhases();
      }
  }
}

/// @brief helper method to rotate all phases
/// @details This method rotates the phases of all gains in itsModel
/// to have the phase of itsRefGain exactly 0. This operation does
/// not seem to be necessary for SVD solvers, however it simplifies
/// "human eye" analysis of the results (otherwise the phase degeneracy
/// would make the solution different from the simulated gains).
/// @note The method throws exception if itsRefGain is not among
/// the parameters of itsModel
void CalibratorParallel::rotatePhases()
{
  ASKAPDEBUGASSERT(itsComms.isMaster());
  ASKAPDEBUGASSERT(itsModel);
  ASKAPCHECK(itsModel->has(itsRefGain), "phase rotation to `"<<itsRefGain<<
              "` is impossible because this parameter is not present in the model");
  
  const casa::Complex refPhaseTerm = casa::polar(1.f,
                -arg(itsModel->complexValue(itsRefGain)));
                       
  std::vector<std::string> names(itsModel->freeNames());
  for (std::vector<std::string>::const_iterator it=names.begin();
               it!=names.end();++it)  {
       const std::string parname = *it;
       if (parname.find("gain") != std::string::npos) {                    
           itsModel->update(parname,
                 itsModel->complexValue(parname)*refPhaseTerm);                                 
       } 
  }             
}

/// @brief helper method to extract solution time from NE.
/// @details To be able to time tag the calibration solutions we add
/// start and stop times extracted from the dataset as metadata to normal
/// equations. It allows us to send these times to the master, which
/// ultimately writes the calibration solution. Otherwise, these times 
/// could only be obtained in workers who deal with the actual data.
/// @return solution time (seconds since 0 MJD)
/// @note if no start/stop time metadata are present in the normal equations
/// this method returns 0.
double CalibratorParallel::solutionTime() const
{
  // use the earliest time corresponding to the data used to make this calibration solution
  // to tag the solution. A request for any latest time than this would automatically 
  // extract this solution as most recent.
  ASKAPASSERT(itsNe);
  
  boost::shared_ptr<scimath::GenericNormalEquations> gne = boost::dynamic_pointer_cast<scimath::GenericNormalEquations>(itsNe);
  if (gne) {
      scimath::Params& metadata = gne->metadata();
      if (metadata.has("min_time")) {
          return metadata.scalarValue("min_time");
      }
  }
  return 0.;
}

/// @brief helper method to set next chunk flag
/// @details In the current design, iteration over the data is done by workers.
/// However, maser needs to make the decision whether more iterations are required,
/// i.e. whether a new chunk of the data is available. We carry this information from
/// worker to maser with the normal equations using metadata. This method encodes the
/// given value of the flag in the normal equations class. 
/// @note Nothing is done if the normal equations object does not support metadata.
/// An exception is thrown if this method is called from the maser. We could've join
/// this method and nextChunk, but it would require making this method non-const
/// @param[in] flag flag value to set
void CalibratorParallel::setNextChunkFlag(const bool flag)
{
  ASKAPCHECK(itsComms.isWorker(), "setNextChunkFlag is supposed to be used in workers");
  if (itsNe) {  
      const boost::shared_ptr<scimath::GenericNormalEquations> gne = boost::dynamic_pointer_cast<scimath::GenericNormalEquations>(itsNe);
      if (gne) {
          scimath::Params& metadata = gne->metadata();
          const double encodedVal = flag ? 1. : -1.;
          const std::string parName = "next_chunk_flag";
          if (metadata.has(parName)) {
              metadata.update(parName, encodedVal);
          } else {
              metadata.add(parName, encodedVal);
          }
      }
  }
}

/// @brief helper method to extract next chunk flag
/// @details This method is a reverse operation to that of setNextChunkFlag. It
/// extracts the flag from the metadata attached to the normal equations and 
/// returns it. 
/// @note false is returned if no appropriate metadata element is found or the normal
/// equations object does not support metadata. 
/// @return true, if the flag is set
bool CalibratorParallel::extractNextChunkFlag() const
{
  if (itsNe) {  
      const boost::shared_ptr<scimath::GenericNormalEquations> gne = boost::dynamic_pointer_cast<scimath::GenericNormalEquations>(itsNe);
      if (gne) {
          const scimath::Params& metadata = gne->metadata();
          const std::string parName = "next_chunk_flag";
          if (metadata.has(parName)) {
              const double encodedVal = metadata.scalarValue(parName);
              return encodedVal > 0;
          } 
      }
  }
  return false;
}


/// @brief initialise the class to iterate over next portion of data
/// @details This method signals to the iterator adapter to switch to the
/// next chunk of data. It also checks whether more data are available.
/// @note an exception is thrown if the iterator adapter is not initialised
/// @return true, if more data chunks are available
bool CalibratorParallel::nextChunk() const
{
  ASKAPCHECK(itsIteratorAdapter, "Iterator adapter is not defined in nextChunk!");
  const bool result = itsIteratorAdapter->moreDataAvailable();
  if (result) {
      itsIteratorAdapter->resume();
  }
  return result;
}


/// @brief Write the results (runs in the solver)
/// @details The solution (calibration parameters) is written into 
/// an external file in the parset file format.
/// @param[in] postfix a string to be added to the file name
void CalibratorParallel::writeModel(const std::string &postfix)
{
  if (itsComms.isMaster()) {
      ASKAPLOG_INFO_STR(logger, "Writing results of the calibration");
      ASKAPCHECK(postfix == "", "postfix parameter is not supposed to be used in the calibration code");

      ASKAPCHECK(itsSolutionSource, "Solution source has to be defined by this stage");

      const long solutionID = itsSolutionSource->newSolutionID(solutionTime());
      boost::shared_ptr<ICalSolutionAccessor> solAcc = itsSolutionSource->rwSolution(solutionID);
      ASKAPASSERT(solAcc);
      
      ASKAPDEBUGASSERT(itsModel);
      std::vector<std::string> parlist = itsModel->names();
      for (std::vector<std::string>::const_iterator it = parlist.begin(); 
           it != parlist.end(); ++it) {
           const casa::Complex val = itsModel->complexValue(*it);
           const std::pair<accessors::JonesIndex, casa::Stokes::StokesTypes> paramType = 
                 accessors::CalParamNameHelper::parseParam(*it);
           solAcc->setJonesElement(paramType.first, paramType.second, val);
      }
  }
}

