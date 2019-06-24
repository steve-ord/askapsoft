/// @file tmssummary.cc
///
/// @brief inspect a measurement set using various MSSummary functions
///
/// @copyright (c) 2015 CSIRO
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
/// @author Daniel Mitchell <daniel.mitchell@csiro.au>

//# Includes
#include <string>
#include <casacore/casa/aips.h>
#include <casacore/casa/Exceptions/Error.h>
#include <casacore/casa/Logging/LogIO.h>
#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/ms/MSOper/MSSummary.h>
#include <casacore/ms/MSOper/MSLister.h>
#include <askap/askap/AskapError.h>

void usage(void) {
  std::cerr << "Usage: " << "mslist [--verbose] MS_filename" << std::endl <<
               "  --brief               brief listing (default with verbose)" << std::endl <<
               "  --full                more extensive listing" << std::endl <<
               "  --what                what what observed? (fields, times, etc.)" << std::endl <<
               "  --how                 how was it observed? (antennas, frequencies, etc.)" << std::endl <<
               "  --tables              list tables in measurement set" << std::endl <<
               "  --data                display correlation data" << std::endl <<
               "  --verbose             verbose output" <<
               std::endl;
  std::cerr << "Data selection parameters used with --data. Standard CASA options." << std::endl <<
               "  --datacolumn=str      " << std::endl <<
               "  --field=str           " << std::endl <<
               "  --spw=str             default=all. e.g. --spw='0:5~10'" << std::endl <<
               "  --antenna=str         default=all. e.g. --antenna='3&&4;1'" << std::endl <<
               "  --timerange=str       default=all. e.g. --timerange='<2014/09/14/10:48:50.0'" << std::endl <<
               "  --correlation=str     default=all. e.g. --correlation=XX" << std::endl <<
               "  --scan=str            default=all. e.g. --scan=9" << std::endl <<
               //"  --feed=str            " << std::endl <<
               //"  --array=str           " << std::endl <<
               "  --uvrange=str         " << std::endl <<
               //"  --average=str         " << std::endl <<
               //"  --showflags=bool      1 or 0. default=0" << std::endl <<
               //"  --msselect=str        " << std::endl <<
               "  --pagerows=int        default=50" << std::endl <<
               "  --listfile=str        default=stdout" << std::endl <<
               std::endl;
}

casacore::String passMsParameter(int argc, const char** argv)
{
  for(int i=1; i<argc; i++) {
      std::string arg = argv[i];
      if(arg.find("-")!=0) {
          return arg;
      }
  }
  return "";
}

casacore::Bool passFlagParameter(int argc, const char** argv, const char* par)
{
  for(int i=1; i<argc; i++) {
      if(!std::strcmp(argv[i], par)) return casacore::True;
  }
  return casacore::False;
}

template <typename T>
T passFlaggedParameter(int argc, const char** argv, const char* par, T def)
{
  T val = def;
  for(int i=1; i<argc; i++) {
      std::string arg = argv[i];
      if(arg.find(par)==0) {
          casacore::Int pos = arg.find("=")+1;
          val = arg.substr(pos,arg.size()-pos);
          break;
      }
  }
  return val;
}

template<>
casacore::Long passFlaggedParameter(int argc, const char** argv, const char* par, casacore::Long def)
{
  casacore::Long val = def;
  for(int i=1; i<argc; i++) {
      std::string arg = argv[i];
      if(arg.find(par)==0) {
          casacore::Int pos = arg.find("=")+1;
          val = atoi(arg.substr(pos,arg.size()-pos).c_str());
          break;
      }
  }
  return val;
}

template<>
casacore::Bool passFlaggedParameter(int argc, const char** argv, const char* par, casacore::Bool def)
{
  casacore::Bool val = def;
  for(int i=1; i<argc; i++) {
      std::string arg = argv[i];
      if(arg.find(par)==0) {
          casacore::Int pos = arg.find("=")+1;
          casacore::Int val = atoi(arg.substr(pos,arg.size()-pos).c_str());
          switch(val) {
            case 0:
              return casacore::False;
              break;
            case 1:
              return casacore::True;
              break;
            default:
              ASKAPTHROW(askap::AskapError, "Unknown bool (use 0 or 1): " << arg);
              break;
          }
      }
  }
  return val;
}

int main(int argc, const char* argv[])
{

  try {

    if (argc < 2) {
        usage();
        ASKAPTHROW(askap::AskapError, "mslist requires a MS file");
    }

    // pass command line parameters
    casacore::Bool brief = passFlagParameter(argc, argv, "--brief");
    casacore::Bool full = passFlagParameter(argc, argv, "--full");
    casacore::Bool what = passFlagParameter(argc, argv, "--what");
    casacore::Bool how = passFlagParameter(argc, argv, "--how");
    casacore::Bool tables = passFlagParameter(argc, argv, "--tables");
    casacore::Bool data = passFlagParameter(argc, argv, "--data");
    casacore::Bool verbose = passFlagParameter(argc, argv, "--verbose");
    // pass parameters for MSLister
    casacore::String datacolumn  = passFlaggedParameter<casacore::String>(argc, argv, "--datacolumn=","");
    casacore::String field       = passFlaggedParameter<casacore::String>(argc, argv, "--field=","");
    casacore::String spw         = passFlaggedParameter<casacore::String>(argc, argv, "--spw=","");
    casacore::String antenna     = passFlaggedParameter<casacore::String>(argc, argv, "--antenna=","");
    casacore::String timerange   = passFlaggedParameter<casacore::String>(argc, argv, "--timerange=","");
    casacore::String correlation = passFlaggedParameter<casacore::String>(argc, argv, "--correlation=","");
    casacore::String scan        = passFlaggedParameter<casacore::String>(argc, argv, "--scan=","");
    //casacore::String feed        = passFlaggedParameter<casacore::String>(argc, argv, "--feed=","");
    //casacore::String array       = passFlaggedParameter<casacore::String>(argc, argv, "--array=","");
    casacore::String uvrange     = passFlaggedParameter<casacore::String>(argc, argv, "--uvrange=","");
    //casacore::String average     = passFlaggedParameter<casacore::String>(argc, argv, "--average=","");
    //casacore::Bool   showflags   = passFlaggedParameter<casacore::Bool>(argc, argv, "--showflags=",casacore::True);
    //casacore::String msselect    = passFlaggedParameter<casacore::String>(argc, argv, "--msselect=","");
    casacore::Long   pagerows    = passFlaggedParameter<casacore::Long>(argc, argv, "--pagerows=",50);
    casacore::String listfile    = passFlaggedParameter<casacore::String>(argc, argv, "--listfile=","");

    casacore::String msfile = passMsParameter(argc, argv);

    // these parameters do not seem to make a difference, so leave them out
    casacore::String feed        = "";
    casacore::String array       = "";
    casacore::String average     = "";
    casacore::Bool showflags     = casacore::False;
    casacore::String msselect    = "";

    // This now needed as the interface to list has changed
    casacore::String observation = "";

    // set default behaviour to brief
    if (!brief && !full && !what && !how && !tables && !data) {
        brief = casacore::True;
        verbose = casacore::True;
    }

    casacore::LogSink globalSink(casacore::LogMessage::NORMAL);
    casacore::LogIO os(globalSink);

    casacore::MeasurementSet ms(msfile, casacore::Table::Old);
    casacore::MSSummary mss(ms);

    if (full || verbose) {
        if (verbose) mss.listTitle(os);
    }

    if (brief) {
        mss.listMain(os, verbose);
    }
    else {

        if ( full ) {
            mss.listWhere(os, verbose);
        }
        if ( full || what ) {
            mss.listWhat(os, verbose);
        }
        if ( full || how ) {
            mss.listHow(os, verbose);
        } 
        if ( full ) {
            if (!ms.source().isNull()) {
                mss.listSource(os,verbose);
            }
            if (!ms.sysCal().isNull()) {
                mss.listSysCal(os,verbose);
            }
            if (!ms.weather().isNull()) {
                mss.listWeather(os,verbose);
            }
            mss.listHistory(os);
        }
        if ( tables ) {
            mss.listTables(os,verbose);
        }

    }

    if (data) {

        casacore::MSLister msl(ms, os);

        std::cout << std::endl;

        //msl.setPage(width=120, height=20);
        //msl.setFormat(ndec=2);
        //msl.setPrecision( precTime=1, precUVDist=0, Int precAmpl=3, precPhase=1, precWeight=0 );

        //use !verbose to restrict some data output? miriad: "a maximum of 6 channels is printed"

        casacore::String options = "";

        msl.list(options, datacolumn, field, spw,
                 antenna, timerange, correlation,
                 scan, feed, array,observation,uvrange,
                 average, showflags, msselect,
                 pagerows, listfile);

    }

  }
  catch (const askap::AskapError& e) {
    std::cerr << "Askap error in " << argv[0] << ": " << e.what() << std::endl;
    exit(1);
  }
  catch (const std::exception& e) {
     std::cerr << "Unexpected exception in " << argv[0] << ": " << e.what()
            << std::endl;
    exit(1);
  }

  return 0;
}

