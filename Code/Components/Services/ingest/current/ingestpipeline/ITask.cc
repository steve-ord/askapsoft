/// @file ITask.cc
///
/// @copyright (c) 2010 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "ingestpipeline/ITask.h"

askap::cp::ingest::ITask::~ITask()
{
}

std::string askap::cp::ingest::ITask::getName(void) const
{
    return itsName;
}

void askap::cp::ingest::ITask::setName(const std::string& name)
{
    itsName = name;
}

/// @brief should this task be executed for inactive ranks?
/// @details If a particular rank is inactive, process method is
/// not called unless this method returns true. Possible use cases:
///   - Splitting the datastream expanding parallelism, i.e
///     inactive rank(s) become active after this task.
///   - Need for collective operations
/// @return true, if process method should be called even if
/// this rank is inactive (i.e. uninitialised chunk pointer
/// will be passed to process method).
/// @note default action is to return false, i.e. process method
/// is not called for inactive tasks.
bool askap::cp::ingest::ITask::isAlwaysActive() const
{
  return false;
}
        
