/// @file DataConverterTest.h
///
/// DataConverterTest: Tests of the DataConverter class(es)
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
#ifndef I_DATA_CONVERTER_TEST_IMPL_H
#define I_DATA_CONVERTER_TEST_IMPL_H

#include<iostream>
#include<stdexcept>
using namespace std;

// boost includes
#include <boost/shared_ptr.hpp>

// CASA includes
#include <casacore/measures/Measures/MEpoch.h>
#include <casacore/measures/Measures/MDirection.h>
#include <casacore/measures/Measures/MPosition.h>
#include <casacore/measures/Measures/MeasConvert.h>
#include <casacore/measures/Measures/MeasFrame.h>
#include <casacore/measures/Measures/MCEpoch.h>
#include <casacore/measures/Measures/MCDirection.h>
#include <casacore/casa/Quanta/MVPosition.h>

// own includes
#include <dataaccess/BasicDataConverter.h>

// cppunit includes
#include <cppunit/extensions/HelperMacros.h>

namespace askap {

namespace accessors {

class DataConverterTest : public CppUnit::TestFixture {

   CPPUNIT_TEST_SUITE(DataConverterTest);
   CPPUNIT_TEST(testEpochConversion);
   CPPUNIT_TEST(testDirectionConversion);
   CPPUNIT_TEST_EXCEPTION(testMissingFrame,std::exception);
   CPPUNIT_TEST(testFrequencyConversion);
   CPPUNIT_TEST(testVelocityConversion);
   CPPUNIT_TEST_EXCEPTION(testMissingRestFrequency1,std::exception);
   CPPUNIT_TEST_EXCEPTION(testMissingRestFrequency2,std::exception);
   CPPUNIT_TEST(testVelToFreq);
   CPPUNIT_TEST(testFreqToVel);
   CPPUNIT_TEST(testEpochToMeasures);
   CPPUNIT_TEST_SUITE_END();
public:
   void setUp()
   {
     itsConverter.reset(new BasicDataConverter);
   }
   void tearDown()
   {
     itsConverter.reset();
   }
   /// test Epoch conversion
   void testEpochConversion()
   { 
    casacore::MEpoch refEpoch=casacore::MEpoch(casacore::MVEpoch(casacore::Quantity(50257.29,"d")),
                            casacore::MEpoch::Ref(casacore::MEpoch::UTC));
    itsConverter->setEpochFrame(refEpoch,"s");
    CPPUNIT_ASSERT(fabs(itsConverter->epoch(refEpoch))<1e-7);
    // adjust it one day forward and see whether the epoch is converted
    // to seconds
    casacore::MEpoch newEpoch=casacore::MEpoch(casacore::MVEpoch(casacore::Quantity(50258.29,"d")),
                            casacore::MEpoch::Ref(casacore::MEpoch::UTC));
    CPPUNIT_ASSERT(fabs(itsConverter->epoch(newEpoch)-86400)<1e-7);

    // convert it to another frame and check again
    casacore::MEpoch gmstEpoch=casacore::MEpoch::Convert(newEpoch,
                         casacore::MEpoch::Ref(casacore::MEpoch::GMST))(newEpoch);    
    CPPUNIT_ASSERT(fabs(itsConverter->epoch(gmstEpoch)-86400)<1e-7);
 
    
    // convert it to LMST (which requires a position) and check again
    const casacore::MeasFrame someFrame=getSomeFrame(WHERE_ONLY);
    // preserve only converted MVEpoch and instantiate MEpoch from scratch
    // in order to clean out the position and test converter properly
    casacore::MEpoch lmstEpoch=casacore::MEpoch(casacore::MEpoch::Convert(newEpoch,
                  casacore::MEpoch::Ref(casacore::MEpoch::LMST,someFrame))(newEpoch).getValue(),
		  casacore::MEpoch::LMST);    
    itsConverter->setMeasFrame(someFrame);
    CPPUNIT_ASSERT(fabs(itsConverter->epoch(lmstEpoch)-86400)<1e-7);
    
   }

   void testMissingFrame()
   {
     casacore::MDirection theSun(casacore::MDirection::SUN);
     itsConverter->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));
     casacore::MVDirection result;
     itsConverter->direction(theSun,result);
   }

   /// test Direction conversion
   void testDirectionConversion()
   {
    
    const casacore::MVDirection direction(casacore::Quantity(30.,"deg"),
                                  casacore::Quantity(-50.,"deg"));
    casacore::MDirection j2000Dir(direction, casacore::MDirection::J2000);
    casacore::MDirection galDir(casacore::MDirection::Convert(j2000Dir,
                           casacore::MDirection::GALACTIC)(j2000Dir));

    itsConverter->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));
    casacore::MVDirection result;
    itsConverter->direction(galDir,result);
    CPPUNIT_ASSERT(result.separation(direction)<1e-7);

    // convert direction to Az/El (which requires time and position)
    // and check again
    const casacore::MeasFrame someFrame=getSomeFrame(WHERE_AND_WHEN);
    casacore::MDirection azelDir(casacore::MDirection::Convert(galDir,
                           casacore::MDirection::Ref(casacore::MDirection::AZEL,
			   someFrame))(galDir));
    itsConverter->setMeasFrame(someFrame);		    
    itsConverter->direction(azelDir,result);
    CPPUNIT_ASSERT(result.separation(direction)<1e-7);
   }

   /// test Frequency conversion
   void testFrequencyConversion()
   {
     const casacore::MVFrequency freq(casacore::Quantity(1420,"MHz"));
     casacore::MFrequency lsrkFreq(freq, casacore::MFrequency::LSRK);
     itsConverter->setFrequencyFrame(casacore::MFrequency::Ref(
                      casacore::MFrequency::LSRK),"GHz");
		      
     CPPUNIT_ASSERT(itsConverter->isVoid(casacore::MFrequency::Ref(
                      casacore::MFrequency::LSRK),"GHz"));

     CPPUNIT_ASSERT(fabs(itsConverter->frequency(lsrkFreq)-1.42)<1e-7);

     // the same with topocentric (i.e. sky frequency) to LSRK conversion
     const casacore::MeasFrame someFrame=getSomeFrame();
     
     casacore::MFrequency topoFreq(casacore::MFrequency::Convert(lsrkFreq,
                           casacore::MFrequency::Ref(casacore::MFrequency::TOPO,
			   someFrame))(lsrkFreq));
     itsConverter->setMeasFrame(someFrame);
     
     CPPUNIT_ASSERT(fabs(itsConverter->frequency(topoFreq)-1.42)<1e-5);
   }

   /// test Velocity conversion
   void testVelocityConversion()
   {
     const casacore::MVRadialVelocity vel(casacore::Quantity(-1000,"m/s"));
     casacore::MRadialVelocity lsrkVel(vel, casacore::MRadialVelocity::LSRK);
     itsConverter->setVelocityFrame(casacore::MRadialVelocity::Ref(
                      casacore::MRadialVelocity::LSRK),"km/s");
     
     CPPUNIT_ASSERT(fabs(itsConverter->velocity(lsrkVel)+1)<1e-7);

     // the same with topocentric (i.e. sky frequency) to LSRK conversion
     const casacore::MeasFrame someFrame=getSomeFrame();

     casacore::MRadialVelocity topoVel(casacore::MRadialVelocity::Convert(lsrkVel,
                           casacore::MRadialVelocity::Ref(casacore::MRadialVelocity::TOPO,
			   someFrame))(lsrkVel));
     itsConverter->setMeasFrame(someFrame);

     CPPUNIT_ASSERT(fabs(itsConverter->velocity(topoVel)+1)<1e-7);
   }

   /// test missing rest frequency for velocity to frequency conversion
   void testMissingRestFrequency1() {     
     casacore::MRadialVelocity lsrkVel((casacore::MVRadialVelocity(casacore::Quantity(
                                    -1000, "m/s"))),
				    casacore::MRadialVelocity::LSRK);
     itsConverter->setFrequencyFrame(casacore::MFrequency::Ref(
                                     casacore::MFrequency::LSRK),"GHz");
     const casacore::Double buf=itsConverter->frequency(lsrkVel);
     // to keep the compiler happy that this variable is used
     ASKAPASSERT(true || buf==0);     
   }
   
   /// test missing rest frequency for frequency to velocity conversion
   void testMissingRestFrequency2() {
     casacore::MFrequency lsrkFreq((casacore::MVFrequency(casacore::Quantity(
                                    1.4, "GHz"))),
				    casacore::MFrequency::LSRK);
     itsConverter->setVelocityFrame(casacore::MRadialVelocity::Ref(
                                    casacore::MRadialVelocity::LSRK),"km/s");
     const casacore::Double buf=itsConverter->velocity(lsrkFreq);     
     // to keep the compiler happy that this variable is used
     ASKAPASSERT(true || buf==0);     
   }

   /// test velocity to frequency conversion
   void testVelToFreq() {     
     casacore::MRadialVelocity lsrkVel((casacore::MVRadialVelocity(casacore::Quantity(
                                    -10, "km/s"))),
				    casacore::MRadialVelocity::LSRK);
     itsConverter->setFrequencyFrame(casacore::MFrequency::Ref(
                                     casacore::MFrequency::TOPO),"MHz");
     itsConverter->setRestFrequency(casacore::MVFrequency(casacore::Quantity(
                                    1420.405752, "MHz")));
				    
     const casacore::MeasFrame someFrame=getSomeFrame(FULL);     

     itsConverter->setMeasFrame(someFrame);     
          
     CPPUNIT_ASSERT(fabs(itsConverter->frequency(lsrkVel)-1420.464418)<1e-5);
   }

   /// test frequency to velocity conversion
   void testFreqToVel() {     
     casacore::MFrequency topoFreq((casacore::MVFrequency(casacore::Quantity(
                                    1420464418, "Hz"))),
				    casacore::MFrequency::TOPO);
     itsConverter->setVelocityFrame(casacore::MRadialVelocity::Ref(
                                     casacore::MRadialVelocity::LSRK),"km/s");
     itsConverter->setRestFrequency(casacore::MVFrequency(casacore::Quantity(
                                    1420.405752, "MHz")));
				    
     const casacore::MeasFrame someFrame=getSomeFrame(FULL);     

     itsConverter->setMeasFrame(someFrame);
     
     CPPUNIT_ASSERT(fabs(itsConverter->velocity(topoFreq)+10)<1e-4);
   }

   /// test reverse "conversions" to measures for epoch
   void testEpochToMeasures() {
     casacore::MEpoch refEpoch=casacore::MEpoch(casacore::MVEpoch(casacore::Quantity(54257.29,"d")),
                            casacore::MEpoch::Ref(casacore::MEpoch::UTC));
     itsConverter->setEpochFrame(refEpoch,"d");
     casacore::MEpoch newEpoch=casacore::MEpoch(casacore::MVEpoch(casacore::Quantity(54258.29,"d")),
                            casacore::MEpoch::Ref(casacore::MEpoch::UTC));
     const casacore::Double asDouble=itsConverter->epoch(newEpoch);
     const casacore::MVEpoch asMVEpoch(casacore::Quantity(asDouble,"d"));
     
     CPPUNIT_ASSERT(fabs(itsConverter->epoch(
              itsConverter->epochMeasure(asDouble))-1.)<1e-7);	      
     CPPUNIT_ASSERT(fabs(itsConverter->epoch(
              itsConverter->epochMeasure(asMVEpoch))-1.)<1e-7);     
   }
   
   
protected:

   /// a type of the frame requested
   enum FrameType {
      WHERE_ONLY,
      WHERE_AND_WHEN,
      FULL
   };
   /// an auxiliary static method to construct an arbitrary frame, where
   /// the conversion is performed.
   /// @param type a FrameType enum describing which aspects of the frame
   ///             must be set
   static casacore::MeasFrame getSomeFrame(FrameType type = FULL) {
     const casacore::MPosition where((casacore::MVPosition(casacore::Quantity(267.,"m"),
                                   casacore::Quantity(149.549,"deg"),
		                   casacore::Quantity(-30.2644,"deg"))),
				   casacore::MPosition::WGS84);

     if (type==WHERE_ONLY) {
         return casacore::MeasFrame(where);
     }

     casacore::MEpoch when=casacore::MEpoch(casacore::MVEpoch(casacore::Quantity(54255.29,"d")),
                            casacore::MEpoch::Ref(casacore::MEpoch::UTC));

     if (type==WHERE_AND_WHEN) {
         return casacore::MeasFrame(where,when);
     }
    
     casacore::MDirection what((casacore::MVDirection(casacore::Quantity(30.,"deg"),
                                 casacore::Quantity(-50.,"deg"))),
				 casacore::MDirection::J2000);

     return casacore::MeasFrame(where,when,what);
   }
private:
   boost::shared_ptr<BasicDataConverter> itsConverter;
};

} // namespace accessors
} // namespace askap

#endif // #ifndef I_DATA_CONVERTER_TEST_IMPL_H
