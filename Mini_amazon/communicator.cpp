#include <arpa/inet.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

#include "amazon_ups.pb.h"
#include "communicator.h"
#include "world_amazon.pb.h"

using namespace std;

/////////////////////////////////
/// Public methods start here
/////////////////////////////////

// Constructor for Communicator
Communicator::Communicator(unsigned int n, Warehouse *houses, int port_num,
                           const char *type_str)
    : port(port_num), type(type_str) {
    sock_fd = INVALID_FD;
    warehouse_number = n;
    warehouses = houses;
    worldid = INVALID_ID;
}

// Check whether Communicator is connected
bool Communicator::is_connect() {
    return (sock_fd != INVALID_FD && worldid != INVALID_ID);
}

long Communicator::get_worldid() { return worldid; }

void Communicator::disconnect() {
    if (sock_fd != INVALID_FD) {
        close(sock_fd);
    }
    sock_fd = INVALID_FD;
    worldid = INVALID_ID;
}

// Connect to target hostname with given worldid
bool Communicator::connect(const char *hostname, long id) {
    // Check current connection status first
    if (this->is_connect()) {
        this->fail_connect("Disconnect current connection first");
        return false;
    }

    // Connect socket with server
    if (!this->setup_sock(hostname)) {
        this->fail_connect("Setup socket failed");
        return false;
    }

    // Connect to Communicator with shackhands
    if (!this->setup_world(id)) {
        this->fail_connect("Setup world failed");
        return false;
    }

    cerr << type << ": Connect Success!" << endl;

    return true;
}

/////////////////////////////////
/// Private methods start here
/////////////////////////////////

void Communicator::fail_connect(const char *err_msg) {
    cerr << type << " Err:  " << err_msg << endl;
    this->disconnect();
}

bool Communicator::setup_sock(const char *hostname) {
    //struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    struct hostent *hptr;

    // Create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        this->fail_connect("Socket creation error");
        return false;
    }

    memset(&serv_addr, '\x00', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert hostname to IPv4/IPv6 address
    if ((hptr = gethostbyname(hostname)) == NULL) {
        this->fail_connect("gethostbyname error for host");
        return false;
    }
    memcpy(&serv_addr.sin_addr, hptr->h_addr_list[0], hptr->h_length);

    // Connect to server
    if (::connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
        0) {
        this->fail_connect("Connect failed");
        return false;
    }
    return true;
}
