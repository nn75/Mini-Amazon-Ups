#ifndef _WEBPROCESSOR_H
#define _WEBPROCESSOR_H

#define INVALID_FD -1
#define INVALID_WEB_FD -1

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <thread>
#include <vector>
#include "amazon_ups.pb.h"
#include "message_queue.h"
#include "world_amazon.pb.h"

using namespace std;

class WebProcessor {
    /////////////////////////////////
    /// Private members start here
    /////////////////////////////////
   private:
    int sock_fd;

    int web_client_fd;
    // communicator type
    const char* type;
    // message queue to send web info to world
    message_queue<pair<long int, ACommands> >& send_world_queue;
    // message queue to send web info to ups
    message_queue<pair<long int, AUCommands> >& send_ups_queue;
    // world seqnum
    long int& world_seqnum;
    // ups seqnum
    long int& ups_seqnum;
    // lock the sequm;
    mutex& mtx;
    // Thread for receving from web
    thread web_recevier_thread;

    // Connect sock with web client
    bool setup_sock();
    // Change state into failed connection
    void fail_connect(const char* err_msg);

    /////////////////////////////////
    /// Public members start here
    /////////////////////////////////
   public:
    // Constructor
    WebProcessor(message_queue<pair<long int, ACommands> >& s_w_q,
                 message_queue<pair<long int, AUCommands> >& s_u_q,
                 long int& wnum, long int& unum, mutex& mt);
    // Disconnect from web
    void disconnect();
    // Connect web
    bool connect();
    // Start receving from web
    void start_recv();
    // Parse info received from web
    void get_buy_info();

    vector<string> split(const string& str, const string& sep);
};

#endif
