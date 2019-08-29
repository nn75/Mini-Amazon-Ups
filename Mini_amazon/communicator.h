#ifndef _COMMUNICATOR_H
#define _COMMUNICATOR_H

#define INVALID_FD -1
#define INVALID_ID -1

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "warehouse.h"

// this is adpated from code that a Google engineer posted online
template <typename T>
bool sendMesgTo(const T &message, int out_fd) {
    google::protobuf::io::FileOutputStream out(out_fd);
    {  // extra scope: make output go away before out->Flush()
        // We create a new coded stream for each message.
        // Don’t worry, this is fast.
        google::protobuf::io::CodedOutputStream output(
            &out);  // Write the size.
        const int size = message.ByteSize();
        output.WriteVarint32(size);
        uint8_t *buffer = output.GetDirectBufferForNBytesAndAdvance(size);
        if (buffer != NULL) {
            // Optimization: The message fits in one buffer, so use the faster
            // direct-to-array serialization path.
            message.SerializeWithCachedSizesToArray(buffer);
        } else {
            // Slightly-slower path when the message is multiple buffers.
            message.SerializeWithCachedSizes(&output);
            if (output.HadError()) {
                return false;
            }
        }
    }
    out.Flush();
    return true;
}

// this is adpated from code that a Google engineer posted online
template <typename T>
bool recvMesgFrom(T &message, int in_fd) {
    google::protobuf::io::FileInputStream in(in_fd);
    google::protobuf::io::CodedInputStream input(&in);
    uint32_t size;
    if (!input.ReadVarint32(&size)) {
        return false;
    }
    // Tell the stream not to read beyond that size.
    google::protobuf::io::CodedInputStream::Limit limit =
        input.PushLimit(size);  // Parse the message.
    if (!message.MergeFromCodedStream(&input)) {
        return false;
    }
    if (!input.ConsumedEntireMessage()) {
        return false;
    }
    // Release the limit.
    input.PopLimit(limit);
    return true;
}

class Communicator {
    /////////////////////////////////
    /// Public members start here
    /////////////////////////////////
   protected:
    // target's port number
    const int port;
    // communicator type
    const char *type;
    // Connected sock_fd, -1 means failed
    int sock_fd;
    // The number of warehouses
    unsigned int warehouse_number;
    Warehouse *warehouses;
    // world id, for test
    long worldid;

    // Connect sock with server
    bool setup_sock(const char *hostname);
    // Change state into failed connection
    void fail_connect(const char *err_msg);

    // *Pure Virtual Function*
    // Hand shake with server
    virtual bool setup_world(long id) = 0;

    /////////////////////////////////
    /// Public members start here
    /////////////////////////////////
   public:
    // Constructor
    Communicator(unsigned int n, Warehouse *houses, int port_num,
                 const char *type_str);
    // Disconnect from server
    void disconnect();
    // Connect to server
    bool connect(const char *hostname, long id = INVALID_ID);
    // Check whether world is connect
    bool is_connect();
    // Get worldid
    long get_worldid();

    // Method for send protobuf message
    template <typename T>
    bool send_msg(const T &message) {
        if (!this->is_connect()) {
            return false;
        }
        return sendMesgTo(message, sock_fd);
    }

    // Method for recv protobuf message
    template <typename T>
    bool recv_msg(T &message) {
        if (!this->is_connect()) {
            return false;
        }
        return recvMesgFrom(message, sock_fd);
    }
};

#endif
