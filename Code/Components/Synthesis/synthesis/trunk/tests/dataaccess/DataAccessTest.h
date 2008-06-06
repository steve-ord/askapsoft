/// @file DataAccessTestImpl
///
/// DataAccessTest: Test of the Data Access layer. Due to the lack of
///                 real implementation all tests are actually compilation
///                 tests. The methods using the interface are gathered in
///                 the separate class DataAccessTestImpl. They will be
///                 called from here when the DataSource implementation
///                 is ready.
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
#ifndef I_DATA_ACCESS_TEST_H
#define I_DATA_ACCESS_TEST_H

#include <stdexcept>
#include <boost/shared_ptr.hpp>

#include "DataAccessTestImpl.h"

#include <cppunit/extensions/HelperMacros.h>

namespace askap {
namespace synthesis {

class DataAccessTest : public CppUnit::TestFixture,
                       protected DataAccessTestImpl		       
{
    CPPUNIT_TEST_SUITE(DataAccessTest);
    CPPUNIT_TEST(testAccess);
    CPPUNIT_TEST_SUITE_END();
public:
    void setUp()
    {
      // do nothing at this stage as we only test compilation
    }
    void tearDown()
    {
      // do nothing at this stage as we only test compilation
    }
    void testAccess()
    {
      // do nothing at this stage as we only test compilation
    }
};
 
} // namespace synthesis
} // namespace askap

#endif // #ifndef I_DATA_ACCESS_TEST_H
