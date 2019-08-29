#include <assert.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "ups_communicator.h"

using namespace std;

// Constructor for UpsCommunicator
UpsCommunicator::UpsCommunicator(unsigned int n, Warehouse *houses)
    : Communicator(n, houses, UPS_PORT, "Ups") {}

bool UpsCommunicator::setup_world(long id) {
    // This method doesn't need id
    worldid = id;
    // assert(id == INVALID_ID);
    // Fill in handshake code if needed
    return true;
}
