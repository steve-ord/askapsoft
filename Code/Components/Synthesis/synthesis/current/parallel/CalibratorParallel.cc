/// @file
///
/// CalibratorParallel: Support for parallel applications using the measurement
/// equation classes. This code applies to calibration. I expect that this part
/// will be redesigned in the future for a better separation of the algorithm
/// from the parallel framework middleware. Current version is basically an
/// adapted ImagerParallel class
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
/// @author Vitaliy Ogarko <vogarko@gmail.com>
///

#ifdef HAVE_MPI
#include <mpi.h>
#endif

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
#include <measurementequation/NoXPolFreqDependentGain.h>
#include <measurementequation/NoXPolBeamIndependentGain.h>
#include <measurementequation/LeakageTerm.h>
#include <measurementequation/Product.h>
#include <measurementequation/ImagingEquationAdapter.h>
#include <gridding/VisGridderFactory.h>
#include <askapparallel/AskapParallel.h>
#include <Common/ParameterSet.h>
#include <calibaccess/CalParamNameHelper.h>
#include <calibaccess/CalibAccessFactory.h>

#ifdef USE_CAL_SERVICE
#include <calibaccess/ServiceCalSolutionSourceStub.h>
#include <calserviceaccessor/ServiceCalSolutionSource.h>
#include <calserviceaccessor/ServiceCalSolutionAccessor.h>
#endif


// casa includes
#include <casacore/casa/aips.h>
#include <casacore/casa/OS/Timer.h>

#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>

using namespace askap;
using namespace askap::scimath;
using namespace askap::synthesis;
using namespace askap::accessors;
using namespace askap::askapparallel;

/// @brief Constructor from ParameterSet
/// @details The parset is used to construct the internal state. We could
/// also support construction from a python dictionary (for example).
/// The command line inputs are needed solely for MPI - currently no
/// application specific information is passed on the command line.
/// @param[in] parset ParameterSet for inputs
CalibratorParallel::CalibratorParallel(askap::askapparallel::AskapParallel& comms,
        const LOFAR::ParameterSet& parset) :
      MEParallelApp(comms,parset),
      itsPerfectModel(new scimath::Params()), itsSolveGains(false), itsSolveLeakage(false),
      itsSolveBandpass(false), itsChannelsPerWorker(0), itsStartChan(0),
      itsBeamIndependentGains(false), itsNormaliseGains(false), itsSolutionInterval(-1.),
      itsMaxNAntForPreAvg(0u), itsMaxNBeamForPreAvg(0u), itsMaxNChanForPreAvg(1u), itsSolutionID(-1), itsSolutionIDValid(false),
      itsMatrixIsParallel(false), itsMajorLoopIterationNumber(0)
{
  const std::string what2solve = parset.getString("solve","gains");
  if (what2solve.find("gains") != std::string::npos) {
      ASKAPLOG_INFO_STR(logger, "Gains will be solved for (solve='"<<what2solve<<"')");
      itsSolveGains = true;
      if (what2solve.find("antennagains") != std::string::npos) {
          ASKAPLOG_INFO_STR(logger, "Same gain values are assumed for all beams (i.e. antenna-based)");
          itsBeamIndependentGains = true;
      }
      itsNormaliseGains = parset.getBool("normalisegains",false);
      if (itsNormaliseGains) {
          ASKAPLOG_INFO_STR(logger,
              "Newly found gains will be normalised to have amplitudes of unity at output");
      }
  }
  if (what2solve.find("leakages") != std::string::npos) {
      ASKAPLOG_INFO_STR(logger, "Leakages will be solved for (solve='"<<what2solve<<"')");
      itsSolveLeakage = true;
  }

  if (what2solve.find("bandpass") != std::string::npos) {
      ASKAPLOG_INFO_STR(logger, "Bandpass will be solved for (solve='"<<what2solve<<"')");
      itsSolveBandpass = true;
      ASKAPCHECK(!itsSolveGains && !itsSolveLeakage,
         "Combination of frequency-dependent and frequency-independent effects is not supported at the moment for simplicity");
  }

  ASKAPCHECK(itsSolveGains || itsSolveLeakage || itsSolveBandpass,
      "Nothing to solve! Either gains or leakages (or both) or bandpass have to be solved for, you specified solve='"<<
      what2solve<<"'");

  init(parset);

  if (parset.getString("solver", "") == "LSQR"
      && parset.getString("solver.LSQR.parallelMatrix", "") == "true") {
      ASKAPCHECK(itsComms.isParallel(), "Parallel matrix scheme is supported only in the parallel mode!");
      ASKAPCHECK(itsSolveBandpass, "Parallel matrix scheme is supported only for bandpass solutions!");
      itsMatrixIsParallel = true;
  }

  if ((itsComms.isMaster() && !itsMatrixIsParallel)
      || (itsComms.isWorker() && itsMatrixIsParallel)) {
      // Create the solver.
      itsSolver.reset(new LinearSolver);
      ASKAPCHECK(itsSolver, "Solver not defined correctly");

      // Set solver parameters.
      const std::string solverType = parset.getString("solver", "SVD");
      itsSolver->setAlgorithm(solverType);

      if (solverType == "LSQR") {
          std::map<std::string, std::string> solverParams = CalibratorParallel::getLSQRSolverParameters(parset);
          if (itsSolveBandpass) {
              solverParams["nChan"] = parset.getString("nChan");
          }
          itsSolver->setParameters(solverParams);
      }
  }

  if (itsMatrixIsParallel) {
      ASKAPDEBUGASSERT(itsComms.nGroups() == 1);
      ASKAPDEBUGASSERT((itsComms.rank() > 0) == itsComms.isWorker());
#ifdef HAVE_MPI
      // Create an MPI communicator with workers only, i.e., master rank excluded (needed for the LSQR solver).
      MPI_Comm newComm;
      int rank = itsComms.rank();
      int color = (int)(rank > 0);
      MPI_Comm_split(MPI_COMM_WORLD, color, rank, &newComm);

      if (itsComms.isWorker()) {
          void *workersComm = (void *)&newComm;
          boost::shared_ptr<LinearSolver> linearSolver = boost::dynamic_pointer_cast<LinearSolver>(itsSolver);
          linearSolver->SetWorkersCommunicator(workersComm);
      }
#endif
  }

  if (itsComms.isMaster()) {
      if (parset.isDefined("refantenna") && parset.isDefined("refgain")) {
          ASKAPLOG_WARN_STR(logger,"refantenna and refgain are both defined. refantenna will be used.");
      }
      if (parset.isDefined("refantenna")) {
          const int refAntenna = parset.getInt32("refantenna",-1);
          const int refBeam = 0;
          itsRefGainXX = accessors::CalParamNameHelper::paramName(refAntenna, refBeam, casa::Stokes::XX);
          itsRefGainYY = accessors::CalParamNameHelper::paramName(refAntenna, refBeam, casa::Stokes::YY);
      } else if (parset.isDefined("refgain")) {
          const string refGain = parset.getString("refgain");
          itsRefGainXX = refGain;
          itsRefGainYY = refGain;
      } else {
          itsRefGainXX = "";
          itsRefGainYY = "";
      }

      // setup solution source (or sink to be exact, because we're writing the solution here)
      itsSolutionSource = CalibAccessFactory::rwCalSolutionSource(parset);

      // Now we have to decide whether we are a Service or a Table.

      ASKAPASSERT(itsSolutionSource);

      const std::string calAccType = parset.getString("calibaccess","parset");
#ifdef USE_CAL_SERVICE
      if (calAccType == "service") {
        itsSolutionSource.reset(new ServiceCalSolutionSource(parset));
        ASKAPLOG_INFO_STR(logger,"Obtaining calibration information from service source");
        itsSolutionIDValid = true;

        // get the string vector of solutions we are going to solve for ....
        const vector<string> willSolve = parset.getStringVector("solve", false);
        string leakages("leakages");
        string gains("gains");
        string bandpass("bandpass");

        boost::shared_ptr<ServiceCalSolutionSource> src = boost::dynamic_pointer_cast<ServiceCalSolutionSource>(itsSolutionSource);
        for (size_t sol = 0; sol < willSolve.size(); ++sol) {
          ASKAPLOG_INFO_STR(logger,"Will solve for :" << willSolve[sol]);
          if (gains.compare(willSolve[sol]) == 0) {
            src->solveGains();
            ASKAPLOG_INFO_STR(logger,"Solving for and will push gains");
          }
          if (leakages.compare(willSolve[sol]) == 0) {
            ASKAPLOG_INFO_STR(logger,"Solving for and will push leakages");
            src->solveLeakages();
          }

          if (bandpass.compare(willSolve[sol]) == 0) {
              ASKAPLOG_INFO_STR(logger,"Solving for and will push bandpass");
              src->solveBandpass();
          }

        }

      }
#endif 

  }
  if (itsComms.isWorker()) {

      // Todo: replace these with a single parameter: Channels(chanperworker,chunk) and move to init
      const int chunkSize = parset.getInt32("chanperworker",0);
      ASKAPCHECK(chunkSize >= 0, "Number of channels per worker cannot be negative, you have "<<chunkSize);
      itsChannelsPerWorker = static_cast<casa::uInt>(chunkSize);
      if (itsChannelsPerWorker > 0) {
          const int chunk = parset.getInt32("chunk");
          ASKAPCHECK(chunk >= 0, "Chunk number is supposed to be non-negative, you have "<<chunk);
          itsStartChan = itsChannelsPerWorker * chunk;
          ASKAPLOG_INFO_STR(logger, "This worker at rank = "<<itsComms.rank()<<" will process "<<itsChannelsPerWorker<<
                                    " spectral channels starting from "<<itsStartChan<<" (chunk="<<chunk<<")");
      }
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

/// @brief helper method to update maximal expected numbers for pre-averaging
/// @details This method updates global maxima based on the current local values. It is handy to
/// avoid code dublication as well as for the case if we ever decide to trim the pre-aveaging buffer
/// to exclude fixed parameters.
/// @param[in] nAnt currently expected number of antennas in the buffer
/// @param[in] nBeam currently expected number of beams in the buffer
/// @param[in] nChan currently expected number of frequency channels in the buffer
void CalibratorParallel::updatePreAvgBufferEstimates(const casa::uInt nAnt, const casa::uInt nBeam, const casa::uInt nChan)
{
   if (itsMaxNAntForPreAvg < nAnt) {
       itsMaxNAntForPreAvg = nAnt;
   }
   if (itsMaxNBeamForPreAvg < nBeam) {
       itsMaxNBeamForPreAvg = nBeam;
   }
   if (itsMaxNChanForPreAvg < nChan) {
       itsMaxNChanForPreAvg = nChan;
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
      const casa::uInt nAnt = parset.getInt32("nAnt",36);
      const casa::uInt nBeam = parset.getInt32("nBeam",1);
      if (itsSolveGains) {
          ASKAPLOG_INFO_STR(logger, "Initialise gains (unknowns) for "<<nAnt<<" antennas and "<<nBeam<<" beam(s).");
          if (itsBeamIndependentGains) {
              ASKAPCHECK(nBeam == 1, "Number of beams should be set to 1 for beam-independent case");
          }
          for (casa::uInt ant = 0; ant<nAnt; ++ant) {
               for (casa::uInt beam = 0; beam<nBeam; ++beam) {
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::XX), casa::Complex(1.,0.));
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::YY), casa::Complex(1.,0.));
                    /*
                    // temporary hack to fix some gains
                    if ((ant == 0) || (ant == 3)) {
                         itsModel->fix(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::XX));
                         itsModel->fix(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::YY));
                    }
                    //
                    */
               }
          }
          // technically could've done it outside the if-statement but this way it reflects the intention to keep track
          // which parameters are added to the model
          updatePreAvgBufferEstimates(nAnt, nBeam);
      }
      if (itsSolveLeakage) {
          ASKAPLOG_INFO_STR(logger, "Initialise leakages (unknowns) for "<<nAnt<<" antennas and "<<nBeam<<" beam(s).");
          for (casa::uInt ant = 0; ant<nAnt; ++ant) {
               for (casa::uInt beam = 0; beam<nBeam; ++beam) {
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::XY),casa::Complex(0.,0.));
                    itsModel->add(accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::YX),casa::Complex(0.,0.));
               }
          }
          // technically could've done it outside the if-statement but this way it reflects the intention to keep track
          // which parameters are added to the model
          updatePreAvgBufferEstimates(nAnt, nBeam);
      }
      if (itsSolveBandpass) {
          const casa::uInt nChan = parset.getInt32("nChan",304);
          ASKAPLOG_INFO_STR(logger, "Initialise bandpass (unknowns) for "<<nAnt<<" antennas, "<<nBeam<<" beam(s) and "<<
                                   nChan<<" spectral channels");
          for (casa::uInt ant = 0; ant<nAnt; ++ant) {
               for (casa::uInt beam = 0; beam<nBeam; ++beam) {
                    const std::string xxParName = accessors::CalParamNameHelper::bpPrefix() + accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::XX);
                    const std::string yyParName = accessors::CalParamNameHelper::bpPrefix() + accessors::CalParamNameHelper::paramName(ant, beam, casa::Stokes::YY);
                    for (casa::uInt chan = 0; chan<nChan; ++chan) {
                         itsModel->add(accessors::CalParamNameHelper::addChannelInfo(xxParName, chan), casa::Complex(1.,0.));
                         itsModel->add(accessors::CalParamNameHelper::addChannelInfo(yyParName, chan), casa::Complex(1.,0.));
                    }
               }
          }
          updatePreAvgBufferEstimates(nAnt, nBeam, nChan);
      }
  }
  if (itsComms.isWorker()) {
      // a greater reuse of the measurement equation could probably be achieved
      // at this stage we cache just the "perfect" ME, but recreate calibration ME.
      itsEquation.reset();

      const casa::uInt nChan = parset.getInt32("chanperworker",0);
      if (nChan > 0) {
          const casa::uInt nAnt = parset.getInt32("nAnt",36);
          const casa::uInt nBeam = parset.getInt32("nBeam",1);
          updatePreAvgBufferEstimates(nAnt, nBeam, nChan);
      }
  }
  itsMajorLoopIterationNumber = 0;
}

std::map<std::string, std::string> CalibratorParallel::getLSQRSolverParameters(const LOFAR::ParameterSet& parset) {
    std::map<std::string, std::string> params;
    if (parset.isDefined("solver.LSQR.alpha")) params["alpha"] = parset.getString("solver.LSQR.alpha");
    if (parset.isDefined("solver.LSQR.norm"))  params["norm"] = parset.getString("solver.LSQR.norm");
    if (parset.isDefined("solver.LSQR.niter")) params["niter"] = parset.getString("solver.LSQR.niter");
    if (parset.isDefined("solver.LSQR.rmin"))  params["rmin"] = parset.getString("solver.LSQR.rmin");
    if (parset.isDefined("solver.LSQR.verbose")) params["verbose"] = parset.getString("solver.LSQR.verbose");
    if (parset.isDefined("solver.LSQR.parallelMatrix")) params["parallelMatrix"] = parset.getString("solver.LSQR.parallelMatrix");

    // Smoothing constraints parameters.
    if (parset.isDefined("solver.LSQR.smoothing")) params["smoothing"] = parset.getString("solver.LSQR.smoothing");
    if (parset.isDefined("solver.LSQR.smoothing.MinWeight")) params["smoothingMinWeight"] = parset.getString("solver.LSQR.smoothing.MinWeight");
    if (parset.isDefined("solver.LSQR.smoothing.MaxWeight")) params["smoothingMaxWeight"] = parset.getString("solver.LSQR.smoothing.MaxWeight");
    if (parset.isDefined("solver.LSQR.smoothing.nsteps")) params["smoothingNsteps"] = parset.getString("solver.LSQR.smoothing.nsteps");

    return params;
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
          if (itsChannelsPerWorker > 0) {
              ASKAPLOG_INFO_STR(logger, "Setting up selector for "<<itsChannelsPerWorker<<" channels starting from "<<itsStartChan);
              sel->chooseChannels(itsChannelsPerWorker,itsStartChan);
          }
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
      } else {
          ASKAPLOG_INFO_STR(logger, "Reusing iterator adapter (this is a subsequent solution interval)");
      }
      ASKAPDEBUGASSERT(itsIteratorAdapter);
      IDataSharedIter it(itsIteratorAdapter);

      ASKAPCHECK(itsModel, "Initial assumption of parameters is not defined");

      if (!itsPerfectME) {
          ASKAPLOG_INFO_STR(logger, "Constructing measurement equation corresponding to the uncorrupted model");
          ASKAPCHECK(itsPerfectModel, "Uncorrupted model not defined");
          if (SynthesisParamsHelper::hasImage(itsPerfectModel)) {
              ASKAPCHECK(!SynthesisParamsHelper::hasComponent(itsPerfectModel),
                         "Image + component case has not yet been implemented");
              // have to create an image-specific equation
              boost::shared_ptr<ImagingEquationAdapter> ieAdapter(new ImagingEquationAdapter);
              ASKAPCHECK(gridder(), "Gridder not defined");
              ieAdapter->assign<ImageFFTEquation>(*itsPerfectModel, gridder());
              itsPerfectME = ieAdapter;
          } else {
              // model is a number of components, don't need an adapter here

              // it doesn't matter which iterator is passed below. It is not used
              boost::shared_ptr<ComponentEquation>
                  compEq(new ComponentEquation(*itsPerfectModel,it));
              itsPerfectME = compEq;
          }
      }
      // now we could've used class data members directly instead of passing them to createCalibrationME
      createCalibrationME(it,itsPerfectME);
      ASKAPCHECK(itsEquation, "Equation is not defined");
  } else {
      ASKAPLOG_INFO_STR(logger, "Reusing measurement equation" );
      // we need to update the model held by measurement equation
      // because it has been cloned at construction
      ASKAPCHECK(itsEquation, "Equation is not defined");
      ASKAPCHECK(itsModel, "Model is not defined");
      // set helper parameter controlling which part of bandpass is solved for (ignored in all other cases)
      setChannelOffsetInModel();
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
   ASKAPDEBUGASSERT(perfectME);
   // temporary logic while preaveraging is being debugged for polarisation
   // calibration
   //const bool doPreAveraging = false;
   const bool doPreAveraging = true;
   //const bool doPreAveraging = !itsSolveLeakage;
  if (!doPreAveraging)  {

   ASKAPCHECK(itsSolutionInterval < 0, "Time-dependent solutions are supported only with pre-averaging, you have interval = "<<
              itsSolutionInterval<<" seconds");
   ASKAPCHECK(!itsSolveBandpass, "Bandpass solution is only supported for pre-averaging at the moment (for simplicity)");

   // the old code without pre-averaging
   if (itsSolveGains && !itsSolveLeakage) {
       if (itsBeamIndependentGains) {
          itsEquation.reset(new CalibrationME<NoXPolBeamIndependentGain>(*itsModel,dsi,perfectME));
       } else {
          itsEquation.reset(new CalibrationME<NoXPolGain>(*itsModel,dsi,perfectME));
       }
   } else if (itsSolveLeakage && !itsSolveGains) {
       itsEquation.reset(new CalibrationME<LeakageTerm>(*itsModel,dsi,perfectME));
   } else if (itsSolveLeakage && itsSolveGains) {
       if (itsBeamIndependentGains) {
           itsEquation.reset(new CalibrationME<Product<NoXPolBeamIndependentGain,LeakageTerm> >(*itsModel,dsi,perfectME));
       } else {
           itsEquation.reset(new CalibrationME<Product<NoXPolGain,LeakageTerm> >(*itsModel,dsi,perfectME));
       }
   } else {
       ASKAPTHROW(AskapError, "Unsupported combination of itsSolveGains and itsSolveLeakage. This shouldn't happen. Verify solve parameter");
   }

  } else {

   // code with pre-averaging
   // it is handy to have a shared pointer to the base type because it is
   // not templated
   boost::shared_ptr<PreAvgCalMEBase> preAvgME;
   if (itsSolveGains && !itsSolveLeakage) {
       if (itsBeamIndependentGains) {
          preAvgME.reset(new CalibrationME<NoXPolBeamIndependentGain, PreAvgCalMEBase>());
       } else {
          preAvgME.reset(new CalibrationME<NoXPolGain, PreAvgCalMEBase>());
       }
   } else if (itsSolveLeakage && !itsSolveGains) {
       preAvgME.reset(new CalibrationME<LeakageTerm, PreAvgCalMEBase>());
   } else if (itsSolveLeakage && itsSolveGains) {
       if (itsBeamIndependentGains) {
          preAvgME.reset(new CalibrationME<Product<NoXPolBeamIndependentGain,LeakageTerm>, PreAvgCalMEBase>());
       } else {
          preAvgME.reset(new CalibrationME<Product<NoXPolGain,LeakageTerm>, PreAvgCalMEBase>());
       }
   } else if (itsSolveBandpass) {
       preAvgME.reset(new CalibrationME<NoXPolFreqDependentGain, PreAvgCalMEBase>());
   } else {
       ASKAPTHROW(AskapError, "Unsupported combination of itsSolveGains and itsSolveLeakage. This shouldn't happen. Verify solve parameter");
   }
   ASKAPDEBUGASSERT(preAvgME);
   // without the following lines of code, the buffer initialisation will be performed based on the first sighted
   // data chunk. This is however problematic if the shape of data may change from iteration to iteration
   ASKAPDEBUGASSERT(itsMaxNAntForPreAvg > 0);
   ASKAPDEBUGASSERT(itsMaxNBeamForPreAvg > 0);
   ASKAPDEBUGASSERT(itsMaxNChanForPreAvg > 0);
   if ((itsMaxNChanForPreAvg > 1) && !preAvgME->isFrequencyDependent()) {
       ASKAPLOG_WARN_STR(logger, "Pre-averaging calibration buffer size estimate ("<<itsMaxNChanForPreAvg<<
              " channels) doesn't seem to be aligned with the frequency dependence property of selected calibration effects");
   }
   preAvgME->initialise(itsMaxNAntForPreAvg, itsMaxNBeamForPreAvg, itsMaxNChanForPreAvg);

   // this is just an optimisation, should work without this line
   preAvgME->beamIndependent(itsBeamIndependentGains);
   preAvgME->accumulate(dsi,perfectME);
   itsEquation = preAvgME;

   // set helper parameter controlling which part of bandpass is solved for (ignored in all other cases)
   setChannelOffsetInModel();

   // this is just because we bypass setting the model for the first major cycle
   // in the case without pre-averaging
   itsEquation->setParameters(*itsModel);
   // set the next chunk flag, if necessary (time-dependent solution is supported only with pre-averaging)
   setNextChunkFlag(nextChunk());
  }
}

/// @brief helper method to update channel offset
/// @details To be able to process a subset of channels we specify the offset
/// in the model. However, this offset needs to be reset per worker in the
/// parallel case for the correct operation. This method encapsulates the required
/// code of setting the channel offset to the value of itsStartChan
void CalibratorParallel::setChannelOffsetInModel() const {
   if (itsModel->has("chan_offset")) {
       itsModel->update("chan_offset",double(itsStartChan));
   } else {
       itsModel->add("chan_offset",double(itsStartChan));
   }
   itsModel->fix("chan_offset");
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
          calcOne(measurementSets()[itsComms.rank()-1], false);
          if (!itsMatrixIsParallel) {
              sendNE();
          } else {
              ASKAPCHECK(itsSolver, "Solver not defined correctly");
              itsSolver->init();
              itsSolver->addNormalEquations(*itsNe);
          }
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
  itsMajorLoopIterationNumber++;

  // Passing major loop iteration number to the solver.
  if (itsSolver) {
      boost::shared_ptr<LinearSolver> linearSolver = boost::dynamic_pointer_cast<LinearSolver>(itsSolver);
      if (linearSolver) {
          linearSolver->SetMajorLoopIterationNumber(itsMajorLoopIterationNumber);
      }
  }

  if (!itsMatrixIsParallel && itsComms.isMaster()) {
      // Receive the normal equations
      if (itsComms.isParallel()) {
          receiveNE();
      }

      ASKAPLOG_INFO_STR(logger, "Solving normal equations");
      casa::Timer timer;
      timer.mark();
      Quality q;
      ASKAPDEBUGASSERT(itsSolver);

      itsSolver->solveNormalEquations(*itsModel,q);

      ASKAPLOG_INFO_STR(logger, "Solved normal equations in "<< timer.real() << " seconds ");
      ASKAPLOG_INFO_STR(logger, "Solution quality: "<<q);
  }

  if (itsMatrixIsParallel) {
      ASKAPDEBUGASSERT(itsComms.nGroups() == 1);
      ASKAPDEBUGASSERT((itsComms.rank() > 0) == itsComms.isWorker());

      if (itsComms.isWorker()) {
          ASKAPLOG_INFO_STR(logger, "Solving normal equations");
          casa::Timer timer;
          timer.mark();
          Quality q;
          ASKAPDEBUGASSERT(itsSolver);

          // TODO: Perhaps we could overload parametersToBroadcast() to send only local parts of the full model to workers,
          //       but when the broadcast is performed (in ccalibrator.cc) the equation is not yet built,
          //       so no direct access to the list of local parameters.
          //       In principle, we could build that local parameters list separately, at the time we initialize the model in init().
          // Creating the local model corresponding to the local normal equation.
          scimath::Params localModel;
          std::vector<std::string> namesEq(itsSolver->normalEquations().unknowns());
          for (std::vector<std::string>::const_iterator it = namesEq.begin();
               it != namesEq.end(); ++it) {
              const std::string parname = *it;
              localModel.add(parname, itsModel->value(parname));
          }

          itsSolver->solveNormalEquations(localModel, q);

          ASKAPLOG_INFO_STR(logger, "Solved normal equations in "<< timer.real() << " seconds ");
          ASKAPLOG_INFO_STR(logger, "Solution quality: "<<q);

          sendModelToMaster(localModel);
      }
      if (itsComms.isMaster()) {
          // Receive the local models from workers, and update the full model.
          for (int i = 0; i < itsComms.nProcs() - 1; ++i) {
              scimath::Params localModel;
              receiveModelOnMaster(localModel, i + 1);

              // Update the full model on master from local models on workers.
              std::vector<std::string> localNames(localModel.freeNames());
              for (std::vector<std::string>::const_iterator it = localNames.begin();
                   it != localNames.end(); ++it) {
                  const std::string parname = *it;
                  itsModel->update(parname, localModel.value(parname));
              }
          }
      }
  }
}

void CalibratorParallel::doPhaseReferencing()
{
    if (itsComms.isMaster()) {
        if (itsRefGainXX != "") {
            if (itsRefGainXX == itsRefGainYY) {
                ASKAPLOG_INFO_STR(logger, "Rotating phases to have that of "<<
                    itsRefGainXX<<" equal to 0");
            } else {
                ASKAPLOG_INFO_STR(logger, "Rotating XX phases to have that of "<<
                    itsRefGainXX<<" equal to 0 and YY phases to have that of "<<
                    itsRefGainYY<<" equal to 0");
            }
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
  // by default assume frequency-independent case
  std::vector<std::string> names(itsModel->freeNames());
  casa::Array<casa::Complex> refPhaseTerms;
  const casa::uInt refPols = 2;
  if (itsSolveBandpass) {
      // first find the required dimensionality
      casa::uInt maxChan = 0;
      for (std::vector<std::string>::const_iterator it=names.begin();
               it!=names.end();++it)  {
           const std::string parname = *it;
           if (parname.find(accessors::CalParamNameHelper::bpPrefix()) != std::string::npos) {
               const casa::uInt chan = accessors::CalParamNameHelper::extractChannelInfo(parname).first;
               if (chan > maxChan) {
                   maxChan = chan;
               }
           }
      }
      // build a vector of reference phase terms, one per channel
      refPhaseTerms.resize(casa::IPosition(2,maxChan+1,refPols));
      for (casa::uInt chan = 0; chan <= maxChan; ++chan) {
           const std::string xRefPar = accessors::CalParamNameHelper::bpPrefix() + accessors::CalParamNameHelper::addChannelInfo(itsRefGainXX, chan);
           const std::string yRefPar = accessors::CalParamNameHelper::bpPrefix() + accessors::CalParamNameHelper::addChannelInfo(itsRefGainYY, chan);
           ASKAPCHECK(itsModel->has(xRefPar), "phase rotation to `"<<xRefPar<<
              "` is impossible because this parameter is not present in the model, channel = "<<chan);
           ASKAPCHECK(itsModel->has(yRefPar), "phase rotation to `"<<yRefPar<<
              "` is impossible because this parameter is not present in the model, channel = "<<chan);
           refPhaseTerms(casa::IPosition(2,chan,0)) = casa::polar(1.f,-arg(itsModel->complexValue(xRefPar)));
           refPhaseTerms(casa::IPosition(2,chan,1)) = casa::polar(1.f,-arg(itsModel->complexValue(yRefPar)));
      }
  } else {
      ASKAPCHECK(itsModel->has(itsRefGainXX), "phase rotation to `"<<itsRefGainXX<<
              "` is impossible because this parameter is not present in the model");
      ASKAPCHECK(itsModel->has(itsRefGainYY), "phase rotation to `"<<itsRefGainYY<<
              "` is impossible because this parameter is not present in the model");
      refPhaseTerms.resize(casa::IPosition(2,1,refPols));
      refPhaseTerms(casa::IPosition(2,0,0)) = casa::polar(1.f,-arg(itsModel->complexValue(itsRefGainXX)));
      refPhaseTerms(casa::IPosition(2,0,1)) = casa::polar(1.f,-arg(itsModel->complexValue(itsRefGainYY)));
  }
  ASKAPDEBUGASSERT(refPhaseTerms.nelements() > 1);

  for (std::vector<std::string>::const_iterator it=names.begin();
               it!=names.end();++it)  {
       const std::string parname = *it;
       const casa::uInt index = itsSolveBandpass ?
           accessors::CalParamNameHelper::extractChannelInfo(parname).first : 0;
       if (parname.find("gain.g11") != std::string::npos) {
           itsModel->update(parname,
                 itsModel->complexValue(parname)*refPhaseTerms(casa::IPosition(2,index,0)));
       }
       else if (parname.find("gain.g22") != std::string::npos) {
           itsModel->update(parname,
                 itsModel->complexValue(parname)*refPhaseTerms(casa::IPosition(2,index,1)));
       }
       else if (parname.find("leakage.d12") != std::string::npos) {
           itsModel->update(parname,
                 itsModel->complexValue(parname)*
                 refPhaseTerms(casa::IPosition(2,index,1))*conj(refPhaseTerms(casa::IPosition(2,index,0))));
       }
       else if (parname.find("leakage.d21") != std::string::npos) {
           itsModel->update(parname,
                 itsModel->complexValue(parname)*
                 refPhaseTerms(casa::IPosition(2,index,0))*conj(refPhaseTerms(casa::IPosition(2,index,1))));
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
/// worker to master with the normal equations using metadata. This method encodes the
/// given value of the flag in the normal equations class.
/// @note Nothing is done if the normal equations object does not support metadata.
/// An exception is thrown if this method is called from the master. We could've join
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
bool CalibratorParallel::getNextChunkFlag() const
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

/// @brief Helper method to remove the next chunk flag
void CalibratorParallel::removeNextChunkFlag()
{
  if (itsNe) {
      const boost::shared_ptr<scimath::GenericNormalEquations> gne = boost::dynamic_pointer_cast<scimath::GenericNormalEquations>(itsNe);
      if (gne) {
          scimath::Params& metadata = gne->metadata();
          const std::string parName = "next_chunk_flag";
          if (metadata.has(parName)) {
              metadata.remove(parName);
          }
      }
  }
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
      ASKAPDEBUGASSERT(!itsIteratorAdapter->hasMore());
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
      if (!itsSolutionIDValid) {
          // obtain solution ID only once, the results can come in random order and the
          // accessor is responsible for aggregating all of them together. This is done based on this ID.
          //@todo Can probably get rid of this
          itsSolutionID = itsSolutionSource->newSolutionID(solutionTime());
          itsSolutionIDValid = true;
      }
      // if the solution time has changed - then you get a new SolutionID
      // this should spot subsequent gain cal solution intervals
      
      itsSolutionID = itsSolutionSource->newSolutionID(solutionTime());
      boost::shared_ptr<ICalSolutionAccessor> solAcc = itsSolutionSource->rwSolution(itsSolutionID);
      ASKAPASSERT(solAcc);

      ASKAPDEBUGASSERT(itsModel);
      std::vector<std::string> parlist = itsModel->freeNames();
      for (std::vector<std::string>::const_iterator it = parlist.begin();
           it != parlist.end(); ++it) {
           casa::Complex val = itsModel->complexValue(*it);
           if (itsSolveBandpass) {
               ASKAPCHECK(it->find(accessors::CalParamNameHelper::bpPrefix()) == 0,
                       "Expect parameter name starting from "<<accessors::CalParamNameHelper::bpPrefix()<<
                       " for the bandpass calibration, you have "<<*it);
               const std::pair<casa::uInt, std::string> parsedParam =
                       accessors::CalParamNameHelper::extractChannelInfo(*it);
               const std::pair<accessors::JonesIndex, casa::Stokes::StokesTypes> paramType =
                    accessors::CalParamNameHelper::parseParam(parsedParam.second);
               solAcc->setBandpassElement(paramType.first, paramType.second, parsedParam.first, val);
           } else {
               const std::pair<accessors::JonesIndex, casa::Stokes::StokesTypes> paramType =
                    accessors::CalParamNameHelper::parseParam(*it);
               if ( itsNormaliseGains ) {
                   if ( casa::fabs(val) > 0.0 &&
                        ((paramType.second == casa::Stokes::XX) ||
                         (paramType.second == casa::Stokes::YY)) ) {
                       val /= casa::fabs(val);
                   }
               }
               solAcc->setJonesElement(paramType.first, paramType.second, val);
           }
      }
  }
}

void CalibratorParallel::sendModelToMaster(const scimath::Params &model) const {
    ASKAPDEBUGASSERT(itsComms.isWorker());
    ASKAPDEBUGASSERT(itsComms.rank() > 0);

    LOFAR::BlobString bs;
    bs.resize(0);
    LOFAR::BlobOBufString bob(bs);
    LOFAR::BlobOStream out(bob);
    out.putStart("model", 1);
    out << model;
    out.putEnd();
    itsComms.sendBlob(bs, 0);
}

void CalibratorParallel::receiveModelOnMaster(scimath::Params &model, int rank) {
    ASKAPDEBUGASSERT(itsComms.isMaster());
    ASKAPDEBUGASSERT(itsComms.rank() == 0);

    LOFAR::BlobString bs;
    bs.resize(0);
    itsComms.receiveBlob(bs, rank);
    LOFAR::BlobIBufString bib(bs);
    LOFAR::BlobIStream in(bib);
    int version=in.getStart("model");
    ASKAPASSERT(version==1);
    in >> model;
    in.getEnd();
}
