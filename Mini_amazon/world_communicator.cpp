#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "world_amazon.pb.h"
#include "world_communicator.h"

using namespace std;

// Constructor for World
WorldCommunicator::WorldCommunicator(unsigned int n, Warehouse *houses)
    : Communicator(n, houses, WORLD_PORT, "World") {}

bool WorldCommunicator::setup_world(long id) {
    // Check sock connect statue
    if (sock_fd == INVALID_FD) {
        this->fail_connect("Setup sock fisrt");
        return false;
    }

    // New a connect retuest
    AConnect *connect_request = new AConnect;
    connect_request->set_isamazon(true);

    // Add warehouses into request
    for (int i = 0; i < warehouse_number; i++) {
        AInitWarehouse *house_request = connect_request->add_initwh();
        house_request->set_id(warehouses[i].get_id());
        house_request->set_x(warehouses[i].get_x());
        house_request->set_y(warehouses[i].get_y());
    }

    // Sepecific id if need
    if (id != INVALID_ID) {
        connect_request->set_worldid(id);
    }

    // Check whether connect_request is valid
    if (!connect_request->IsInitialized()) {
        this->fail_connect("Unknown Error");
        return false;
    }
#ifdef DEBUG
    cerr << "DEBUG: Send WorldID: " << connect_request->worldid() << endl;
    cerr << "DEBUG: Send Initwh size: " << connect_request->initwh_size()
         << endl;
    cerr << "DEBUG: Send IsAmazon: " << connect_request->isamazon() << endl;
#endif

    // Send request to server
    if (!sendMesgTo<AConnect>(*connect_request, sock_fd)) {
        this->fail_connect("Cannot send AConnect request");
        return false;
    }

    // Receive respond
    AConnected *connect_response = new AConnected;
    if (!recvMesgFrom(*connect_response, sock_fd)) {
        this->fail_connect("Cannot recv AConnected response");
        return false;
    }

    if (strncmp("connected!", connect_response->result().c_str(),
                strlen("connected!"))) {
        this->fail_connect("Recv failed response");
#ifdef DEBUG
        cerr << "DEBUG: " << connect_response->result().c_str() << endl;
#endif
        return false;
    }

    if (id != INVALID_ID && id != connect_response->worldid()) {
        this->fail_connect("Inconstant worldid");
        return false;
    }
    worldid = connect_response->worldid();

#ifdef DEBUG
    cerr << "DEBUG: Recv WorldID: " << worldid << endl;
#endif

    return true;
}
