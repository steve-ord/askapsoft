/// @file
/// $brief Tests of the polarisation frame converter
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
#ifndef POL_CONVERTER_TEST_H
#define POL_CONVERTER_TEST_H

// System includes
#include <cmath>

// Cppunit includes
#include <cppunit/extensions/HelperMacros.h>

// ASKAPsoft includes
#include "askap/scimath/utils/PolConverter.h"
#include "askap/askap/AskapError.h"

namespace askap {

namespace scimath {

class PolConverterTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PolConverterTest);
  CPPUNIT_TEST(dimensionTest);
  CPPUNIT_TEST(stokesIonlyTest);
  CPPUNIT_TEST_EXCEPTION(dimensionExceptionTest, askap::CheckError);
  CPPUNIT_TEST(stokesEnumTest);
  CPPUNIT_TEST(stringConversionTest);
  CPPUNIT_TEST(linear2stokesTest);
  CPPUNIT_TEST(circular2stokesTest);
  CPPUNIT_TEST(sparseTransformTest);
  CPPUNIT_TEST(canonicOrderTest);
  CPPUNIT_TEST_SUITE_END();
public:
  void dimensionTest() {
     casacore::Vector<casacore::Stokes::StokesTypes> in(4);
     in[0] = casacore::Stokes::XX;
     in[1] = casacore::Stokes::XY;
     in[2] = casacore::Stokes::YX;
     in[3] = casacore::Stokes::YY;
     casacore::Vector<casacore::Stokes::StokesTypes> out(2);
     out[0] = casacore::Stokes::I;
     out[1] = casacore::Stokes::Q;
     
     PolConverter pc(in,out);
     CPPUNIT_ASSERT(pc.nInputDim() == 4);
     CPPUNIT_ASSERT(pc.nOutputDim() == 2);     
     casacore::Vector<casacore::Complex> inVec(in.nelements(), casacore::Complex(0,-1.));
     casacore::Vector<casacore::Complex> outVec = pc(inVec);
     CPPUNIT_ASSERT(outVec.nelements() == out.nelements());
     CPPUNIT_ASSERT(abs(outVec[0]-casacore::Complex(0.,-2.))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[1]-casacore::Complex(0.,0.))<1e-5);
     // check noise
     casacore::Vector<casacore::Complex> noise = pc.noise(casacore::Vector<casacore::Complex>(in.nelements(), casacore::Complex(1.,1.)));
     CPPUNIT_ASSERT(noise.nelements() == out.nelements());
     CPPUNIT_ASSERT(abs(noise[0]-casacore::Complex(sqrt(2.),sqrt(2.)))<1e-5);
     CPPUNIT_ASSERT(abs(noise[1]-casacore::Complex(sqrt(2.),sqrt(2.)))<1e-5);
     
     
     // ignore missing polarisations in pc2
     PolConverter pc2(out,in,false);
     CPPUNIT_ASSERT(pc2.nInputDim() == 2);
     CPPUNIT_ASSERT(pc2.nOutputDim() == 4);     
     casacore::Vector<casacore::Complex> inVec2(out.nelements(), casacore::Complex(0,-1.));
     casacore::Vector<casacore::Complex> outVec2 = pc2(inVec2);
     CPPUNIT_ASSERT(outVec2.nelements() == in.nelements());
     CPPUNIT_ASSERT(abs(outVec2[0]-casacore::Complex(0.,-1.))<1e-5);
     CPPUNIT_ASSERT(abs(outVec2[1]-casacore::Complex(0.,0.))<1e-5);
     CPPUNIT_ASSERT(abs(outVec2[2]-casacore::Complex(0.,0.))<1e-5);
     CPPUNIT_ASSERT(abs(outVec2[3]-casacore::Complex(0.,0.))<1e-5);     
     // check noise
     noise.assign(pc2.noise(casacore::Vector<casacore::Complex>(out.nelements(), casacore::Complex(1.,1.))));
     CPPUNIT_ASSERT(noise.nelements() == in.nelements());
     CPPUNIT_ASSERT(abs(noise[0]-casacore::Complex(1./sqrt(2.),1./sqrt(2.)))<1e-5);
     CPPUNIT_ASSERT(abs(noise[1]-casacore::Complex(0.,0.))<1e-5);
     CPPUNIT_ASSERT(abs(noise[2]-casacore::Complex(0.,0.))<1e-5);
     CPPUNIT_ASSERT(abs(noise[3]-casacore::Complex(1./sqrt(2.),1./sqrt(2.)))<1e-5);
  }
  
  void dimensionExceptionTest() {
     casacore::Vector<casacore::Stokes::StokesTypes> in(2);
     in[0] = casacore::Stokes::I;
     in[1] = casacore::Stokes::Q;

     casacore::Vector<casacore::Stokes::StokesTypes> out(4);
     out[0] = casacore::Stokes::XX;
     out[1] = casacore::Stokes::XY;
     out[2] = casacore::Stokes::YX;
     out[3] = casacore::Stokes::YY;
     
     // don't ignore missing polarisations here (default third argument), this should cause an
     // exception in the constructor     
     PolConverter pc(in,out);
     CPPUNIT_ASSERT(pc.nInputDim() == 2);
     CPPUNIT_ASSERT(pc.nOutputDim() == 4);          
     casacore::Vector<casacore::Complex> inVec(in.nelements(), casacore::Complex(0,-1.));
     pc(inVec);  
  }
  
  void stokesIonlyTest() {
     casacore::Vector<casacore::Stokes::StokesTypes> in(1);
     in[0] = casacore::Stokes::I;

     casacore::Vector<casacore::Stokes::StokesTypes> out(2);
     out[0] = casacore::Stokes::XX;
     out[1] = casacore::Stokes::YY;

     PolConverter pc(in,out,false);
     CPPUNIT_ASSERT(pc.nInputDim() == 1);
     CPPUNIT_ASSERT(pc.nOutputDim() == 2);          
     casacore::Vector<casacore::Complex> inVec(in.nelements(), casacore::Complex(0,-1.));
     casacore::Vector<casacore::Complex> outVec = pc(inVec);  
     CPPUNIT_ASSERT(abs(outVec[0]-casacore::Complex(0,-0.5))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[1]-casacore::Complex(0,-0.5))<1e-5);          
     // check noise
     casacore::Vector<casacore::Complex> noise = pc.noise(casacore::Vector<casacore::Complex>(in.nelements(), casacore::Complex(1.,1.)));
     CPPUNIT_ASSERT(noise.nelements() == out.nelements());
     CPPUNIT_ASSERT(abs(noise[0]-casacore::Complex(0.5,0.5))<1e-5);
     CPPUNIT_ASSERT(abs(noise[1]-casacore::Complex(0.5,0.5))<1e-5);
     
     PolConverter pc2(out,in);
     CPPUNIT_ASSERT(pc2.nInputDim() == 2);
     CPPUNIT_ASSERT(pc2.nOutputDim() == 1);          
     inVec.resize(2);
     inVec.set(casacore::Complex(1.,0));
     outVec.resize(1);
     outVec = pc2(inVec);
     CPPUNIT_ASSERT(abs(outVec[0]-casacore::Complex(2.,0.))<1e-5);          
     // check noise
     noise.assign(pc2.noise(casacore::Vector<casacore::Complex>(out.nelements(), casacore::Complex(1.,1.))));
     CPPUNIT_ASSERT(noise.nelements() == in.nelements());
     CPPUNIT_ASSERT(abs(noise[0]-casacore::Complex(sqrt(2.),sqrt(2.)))<1e-5);
  }
  
  void linear2stokesTest() {
     casacore::Vector<casacore::Stokes::StokesTypes> in(4);
     in[0] = casacore::Stokes::XX;
     in[1] = casacore::Stokes::XY;
     in[2] = casacore::Stokes::YX;
     in[3] = casacore::Stokes::YY;
     casacore::Vector<casacore::Stokes::StokesTypes> out(4);
     out[0] = casacore::Stokes::I;
     out[1] = casacore::Stokes::Q;
     out[2] = casacore::Stokes::U;
     out[3] = casacore::Stokes::V;
     
     PolConverter pc(in,out);
     CPPUNIT_ASSERT(pc.nInputDim() == 4);
     CPPUNIT_ASSERT(pc.nOutputDim() == 4);     
     casacore::Vector<casacore::Complex> inVec(in.nelements());
     inVec[0]=casacore::Complex(0.1,0.2);
     inVec[1]=casacore::Complex(0.3,0.4);
     inVec[2]=casacore::Complex(0.5,0.6);
     inVec[3]=casacore::Complex(0.7,0.8);     
     casacore::Vector<casacore::Complex> outVec = pc(inVec);
     CPPUNIT_ASSERT(outVec.nelements() == out.nelements());
     CPPUNIT_ASSERT(abs(outVec[0]-casacore::Complex(0.8,1))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[1]-casacore::Complex(-0.6,-0.6))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[2]-casacore::Complex(0.8,1.0))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[3]-casacore::Complex(-0.2,0.2))<1e-5);
     // check noise
     casacore::Vector<casacore::Complex> noise = pc.noise(casacore::Vector<casacore::Complex>(in.nelements(),casacore::Complex(1.,1.)));
     CPPUNIT_ASSERT(noise.nelements() == out.nelements());
     for (casacore::uInt dim = 0; dim<noise.nelements(); ++dim) {
          CPPUNIT_ASSERT(abs(noise[dim]-casacore::Complex(sqrt(2.),sqrt(2.)))<1e-5);
     }
     // more realistic case of (slightly) different noise in orthogonal polarisation products
     casacore::Vector<casacore::Complex> inNoise(in.nelements());
     inNoise[0] = casacore::Complex(0.009,0.009);
     inNoise[3] = casacore::Complex(0.011,0.011);
     const float crossPolNoise = sqrt(casacore::real(inNoise[0])*casacore::real(inNoise[3]));
     inNoise[1] = inNoise[2] = casacore::Complex(crossPolNoise, crossPolNoise);
     noise.assign(pc.noise(inNoise));
     CPPUNIT_ASSERT(noise.nelements() == out.nelements());
     for (casacore::uInt dim = 0; dim<noise.nelements(); ++dim) {
          CPPUNIT_ASSERT(fabsf(casacore::real(noise[dim])-casacore::imag(noise[dim]))<1e-5);
          // 202 == 9*9+11*11, 198 = 2*9*11
          const float targetVal = 0.001*(dim<2 ? sqrt(202.) : sqrt(198.));
          CPPUNIT_ASSERT(abs(noise[dim]-casacore::Complex(targetVal,targetVal))<1e-5);
     }
     
     PolConverter pcReverse(out,in);
     CPPUNIT_ASSERT(pcReverse.nInputDim() == 4);
     CPPUNIT_ASSERT(pcReverse.nOutputDim() == 4);          
     casacore::Vector<casacore::Complex> newInVec = pcReverse(outVec);
     CPPUNIT_ASSERT(newInVec.nelements() == inVec.nelements());
     for (size_t pol = 0; pol<inVec.nelements(); ++pol) {
          CPPUNIT_ASSERT(abs(inVec[pol] - newInVec[pol])<1e-5);
     }     
     // verify noise
     casacore::Vector<casacore::Complex> outNoise = pcReverse.noise(noise);
     CPPUNIT_ASSERT(outNoise.nelements() == in.nelements());
     CPPUNIT_ASSERT(outNoise.nelements() == inNoise.nelements());
     
     for (casacore::uInt dim=0; dim<outNoise.nelements(); ++dim) {
          const float targetVal = (dim % 3 == 0) ? 
                      sqrt(casacore::square(casacore::real(noise[0])) + casacore::square(casacore::real(noise[1]))) / 2. :
                      sqrt(casacore::square(casacore::real(noise[2])) + casacore::square(casacore::real(noise[3]))) / 2.;
          CPPUNIT_ASSERT(abs(outNoise[dim] - casacore::Complex(targetVal,targetVal))<1e-5);
     }     
  }

  void circular2stokesTest() {
     casacore::Vector<casacore::Stokes::StokesTypes> in(4);
     in[0] = casacore::Stokes::RR;
     in[1] = casacore::Stokes::RL;
     in[2] = casacore::Stokes::LR;
     in[3] = casacore::Stokes::LL;
     casacore::Vector<casacore::Stokes::StokesTypes> out(4);
     out[0] = casacore::Stokes::I;
     out[1] = casacore::Stokes::Q;
     out[2] = casacore::Stokes::U;
     out[3] = casacore::Stokes::V;
     
     PolConverter pc(in,out);
     CPPUNIT_ASSERT(pc.nInputDim() == 4);
     CPPUNIT_ASSERT(pc.nOutputDim() == 4);     
     casacore::Vector<casacore::Complex> inVec(in.nelements());
     inVec[0]=casacore::Complex(0.1,0.2);
     inVec[1]=casacore::Complex(0.3,0.4);
     inVec[2]=casacore::Complex(0.5,0.6);
     inVec[3]=casacore::Complex(0.7,0.8);     
     casacore::Vector<casacore::Complex> outVec = pc(inVec);
     CPPUNIT_ASSERT(outVec.nelements() == out.nelements());
     CPPUNIT_ASSERT(abs(outVec[0]-casacore::Complex(0.8,1))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[1]-casacore::Complex(-0.2,0.2))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[2]-casacore::Complex(-0.6,-0.6))<1e-5);
     CPPUNIT_ASSERT(abs(outVec[3]-casacore::Complex(0.8,1.0))<1e-5);

     // check noise
     casacore::Vector<casacore::Complex> noise = pc.noise(casacore::Vector<casacore::Complex>(in.nelements(),casacore::Complex(1.,1.)));
     CPPUNIT_ASSERT(noise.nelements() == out.nelements());
     for (casacore::uInt dim = 0; dim<noise.nelements(); ++dim) {
          CPPUNIT_ASSERT(abs(noise[dim]-casacore::Complex(sqrt(2.),sqrt(2.)))<1e-5);
     }
     // more realistic case of (slightly) different noise in orthogonal polarisation products
     casacore::Vector<casacore::Complex> inNoise(in.nelements());
     inNoise[0] = casacore::Complex(0.009,0.009);
     inNoise[3] = casacore::Complex(0.011,0.011);
     const float crossPolNoise = sqrt(casacore::real(inNoise[0])*casacore::real(inNoise[3]));
     inNoise[1] = inNoise[2] = casacore::Complex(crossPolNoise, crossPolNoise);
     noise.assign(pc.noise(inNoise));
     CPPUNIT_ASSERT(noise.nelements() == out.nelements());
     for (casacore::uInt dim = 0; dim<noise.nelements(); ++dim) {
          CPPUNIT_ASSERT(fabsf(casacore::real(noise[dim])-casacore::imag(noise[dim]))<1e-5);
          // 202 == 9*9+11*11, 198 = 2*9*11
          const float targetVal = 0.001*(dim % 2 == 0 ? sqrt(202.) : sqrt(198.));
          CPPUNIT_ASSERT(abs(noise[dim]-casacore::Complex(targetVal,targetVal))<1e-5);
     }
     
     PolConverter pcReverse(out,in);
     CPPUNIT_ASSERT(pcReverse.nInputDim() == 4);
     CPPUNIT_ASSERT(pcReverse.nOutputDim() == 4);          
     casacore::Vector<casacore::Complex> newInVec = pcReverse(outVec);
     CPPUNIT_ASSERT(newInVec.nelements() == inVec.nelements());
     for (size_t pol = 0; pol<inVec.nelements(); ++pol) {
          CPPUNIT_ASSERT(abs(inVec[pol] - newInVec[pol])<1e-5);
     }     

     // verify noise
     casacore::Vector<casacore::Complex> outNoise = pcReverse.noise(noise);
     CPPUNIT_ASSERT(outNoise.nelements() == in.nelements());
     CPPUNIT_ASSERT(outNoise.nelements() == inNoise.nelements());
     
     for (casacore::uInt dim=0; dim<outNoise.nelements(); ++dim) {
          const float targetVal = (dim % 3 == 0) ? 
                      sqrt(casacore::square(casacore::real(noise[0])) + casacore::square(casacore::real(noise[2]))) / 2. :
                      sqrt(casacore::square(casacore::real(noise[1])) + casacore::square(casacore::real(noise[3]))) / 2.;
          CPPUNIT_ASSERT(abs(outNoise[dim] - casacore::Complex(targetVal,targetVal))<1e-5);
     }     
  }
  
  void stokesEnumTest() {
     // our code relies on a particular order of the stokes parameters in the enum defined in casacore.
     // The following code tests that enums components corresponding to the same polarisation
     // frame are following each other and the order is preserved.
     
     // I,Q,U,V
     CPPUNIT_ASSERT(int(casacore::Stokes::Q)-int(casacore::Stokes::I) == 1);
     CPPUNIT_ASSERT(int(casacore::Stokes::U)-int(casacore::Stokes::I) == 2);
     CPPUNIT_ASSERT(int(casacore::Stokes::V)-int(casacore::Stokes::I) == 3);
     
     // XX,XY,YX,YY
     CPPUNIT_ASSERT(int(casacore::Stokes::XY)-int(casacore::Stokes::XX) == 1);
     CPPUNIT_ASSERT(int(casacore::Stokes::YX)-int(casacore::Stokes::XX) == 2);
     CPPUNIT_ASSERT(int(casacore::Stokes::YY)-int(casacore::Stokes::XX) == 3);
     
     // RR,RL,LR,LL
     CPPUNIT_ASSERT(int(casacore::Stokes::RL)-int(casacore::Stokes::RR) == 1);
     CPPUNIT_ASSERT(int(casacore::Stokes::LR)-int(casacore::Stokes::RR) == 2);
     CPPUNIT_ASSERT(int(casacore::Stokes::LL)-int(casacore::Stokes::RR) == 3);
     
     // mixed products
     CPPUNIT_ASSERT(int(casacore::Stokes::RY)-int(casacore::Stokes::RX) == 1);
     CPPUNIT_ASSERT(int(casacore::Stokes::LX)-int(casacore::Stokes::RX) == 2);
     CPPUNIT_ASSERT(int(casacore::Stokes::LY)-int(casacore::Stokes::RX) == 3);
     CPPUNIT_ASSERT(int(casacore::Stokes::XR)-int(casacore::Stokes::RX) == 4);
     CPPUNIT_ASSERT(int(casacore::Stokes::XL)-int(casacore::Stokes::RX) == 5);
     CPPUNIT_ASSERT(int(casacore::Stokes::YR)-int(casacore::Stokes::RX) == 6);
     CPPUNIT_ASSERT(int(casacore::Stokes::YL)-int(casacore::Stokes::RX) == 7);
     
  }  
  void stringConversionTest() {
     CPPUNIT_ASSERT(PolConverter::equal(PolConverter::fromString("xx,yy,xy,yx"),
                    PolConverter::fromString("xxyyxyyx")));
     CPPUNIT_ASSERT(PolConverter::equal(PolConverter::fromString("xyi,qu"),
                    PolConverter::fromString("xy i q u")));
     casacore::Vector<casacore::Stokes::StokesTypes> frame = PolConverter::fromString("xy i q RR");
     CPPUNIT_ASSERT(frame.nelements() == 4);
     CPPUNIT_ASSERT(frame[0] == casacore::Stokes::XY);
     CPPUNIT_ASSERT(frame[1] == casacore::Stokes::I);
     CPPUNIT_ASSERT(frame[2] == casacore::Stokes::Q);
     CPPUNIT_ASSERT(frame[3] == casacore::Stokes::RR);
     std::vector<std::string> frameStr = PolConverter::toString(frame);
     CPPUNIT_ASSERT(frameStr.size() == 4);
     CPPUNIT_ASSERT(frameStr[0] == "XY");
     CPPUNIT_ASSERT(frameStr[1] == "I");
     CPPUNIT_ASSERT(frameStr[2] == "Q");
     CPPUNIT_ASSERT(frameStr[3] == "RR");     
  }
  
  void sparseTransformTest() {
     casacore::Vector<casacore::Stokes::StokesTypes> in(4);
     in[0] = casacore::Stokes::I;
     in[1] = casacore::Stokes::Q;
     in[2] = casacore::Stokes::U;
     in[3] = casacore::Stokes::V;
     casacore::Vector<casacore::Stokes::StokesTypes> out(4);
     out[0] = casacore::Stokes::XX;
     out[1] = casacore::Stokes::XY;
     out[2] = casacore::Stokes::YX;
     out[3] = casacore::Stokes::YY;
     
     PolConverter pc(in,out);
     CPPUNIT_ASSERT(pc.nInputDim() == 4);
     CPPUNIT_ASSERT(pc.nOutputDim() == 4);
     CPPUNIT_ASSERT(pc.equal(in, pc.inputPolFrame()));
     CPPUNIT_ASSERT(pc.equal(out, pc.outputPolFrame()));          
 
     std::map<casacore::Stokes::StokesTypes, casacore::Complex> tfm = pc.getSparseTransform(casacore::Stokes::I);
     CPPUNIT_ASSERT_EQUAL(size_t(2), tfm.size());
     CPPUNIT_ASSERT(tfm.find(casacore::Stokes::XX) != tfm.end());
     CPPUNIT_ASSERT(tfm.find(casacore::Stokes::YY) != tfm.end());
     CPPUNIT_ASSERT(tfm.find(casacore::Stokes::XY) == tfm.end());
     CPPUNIT_ASSERT(tfm.find(casacore::Stokes::YX) == tfm.end());
     CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5,real(tfm[casacore::Stokes::XX]),1e-5);
     CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5,real(tfm[casacore::Stokes::YY]),1e-5);
     CPPUNIT_ASSERT_DOUBLES_EQUAL(0.,imag(tfm[casacore::Stokes::XX]),1e-5);
     CPPUNIT_ASSERT_DOUBLES_EQUAL(0.,imag(tfm[casacore::Stokes::YY]),1e-5);     
  }
  
  void canonicOrderTest() {  
     const casacore::Vector<casacore::Stokes::StokesTypes> stokes = PolConverter::canonicStokes();
     CPPUNIT_ASSERT(PolConverter::isStokes(stokes));
     const casacore::Vector<casacore::Stokes::StokesTypes> linear = PolConverter::canonicLinear();
     CPPUNIT_ASSERT(PolConverter::isLinear(linear));
     const casacore::Vector<casacore::Stokes::StokesTypes> circular = PolConverter::canonicCircular();
     CPPUNIT_ASSERT(PolConverter::isCircular(circular));
     CPPUNIT_ASSERT_EQUAL(size_t(4), stokes.nelements());
     CPPUNIT_ASSERT_EQUAL(size_t(4), linear.nelements());
     CPPUNIT_ASSERT_EQUAL(size_t(4), circular.nelements());
     for (casacore::uInt elem = 0; elem < stokes.nelements(); ++elem) {
          CPPUNIT_ASSERT_EQUAL(elem, PolConverter::getIndex(stokes[elem]));
          CPPUNIT_ASSERT_EQUAL(elem, PolConverter::getIndex(linear[elem]));
          CPPUNIT_ASSERT_EQUAL(elem, PolConverter::getIndex(circular[elem]));
     }
  }
  
  
  
  
  
};

} // namespace scimath

} // namespace askap

#endif // #ifndef POL_CONVERTER_TEST_H

