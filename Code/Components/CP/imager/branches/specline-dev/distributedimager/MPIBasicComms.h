/// @file MPIBasicComms.h
///
/// @copyright (c) 2009 CSIRO
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

#ifndef ASKAP_CP_MPIBASICCOMMS_H
#define ASKAP_CP_MPIBASICCOMMS_H

// Include own header file first
#include "IBasicComms.h"

// System includes
#include <string>
#include <mpi.h>

// ASKAPsoft includes
#include <messages/IMessage.h>

namespace askap {
namespace cp {

class MPIBasicComms : public IBasicComms
{
public:
    MPIBasicComms(int argc, char *argv[]);
    virtual ~MPIBasicComms();

    virtual int getId(void);
    virtual int getNumNodes(void);
    virtual void abort(void);

    void sendMessage(const IMessage& msg, int dest);
    void receiveMessage(IMessage& msg, int source);
    void receiveMessageAnySrc(IMessage& msg);
    void receiveMessageAnySrc(IMessage& msg, int& actualSource);

    void sendMessageBroadcast(const IMessage& msg);
    void receiveMessageBroadcast(IMessage& msg, int root);

private:
    /// @brief MPI_Send a raw buffer to the specified destination
    /// process.
    ///
    /// @params[in] buf a pointer to the buffer to send.
    /// @params[in] size    the number of bytes to send.
    /// @params[in] dest    the id of the process to send to.
    /// @params[in] tag the MPI tag to be used in the communication.
    void send(const void* buf, size_t size, int dest, int tag);

    /// @brief MPI_Recv a raw buffer from the specified source process.
    ///
    /// @params[out] buf a pointer to the buffer to receive data into.
    /// @params[in] size    the number of bytes to receive.
    /// @params[in] source  the id of the process to receive from.
    /// @params[in] tag the MPI tag to be used in the communication.
    /// @params[out] status MPI_Status structure returned by the call to
    ///                     MPI_Recv()
    void receive(void* buf, size_t size, int source, int tag, MPI_Status& status);

    /// @brief MPI_Bcast a raw buffer.
    ///
    /// @params [in,out] buf    data buffer.
    /// @params [in] size       number of bytes to broadcast.
    /// @params [in] root       id of the root process.
    void broadcast(void* buf, size_t size, int root);

    // Check for error status and handle accordingly
    void checkError(const int error, const std::string location);

    // Add a byte offset to the  specified pointer, returning the result
    void* addOffset(const void *ptr, size_t offset);

    // Root for broadcasts
    static const int itsRoot = 0;

    // Specific MPI Communicator for this class
    MPI_Comm itsCommunicator;

    // No support for assignment
    MPIBasicComms& operator=(const MPIBasicComms& rhs);

    // No support for copy constructor
    MPIBasicComms(const MPIBasicComms& src);
};

};
};

#endif
