#ifndef __AMAZONSERVER__H_
#define __AMAZONSERVER__H_

#include "communicator.h"

// Server(w_send_queue, w_recv_queue, ups_send_queue, worldworker)
class AmazonServer {
    /////////////////////////////////
    /// Private members start here
    /////////////////////////////////
   private:
    // target's port number
    message_queue<ACommands> send_world_queue;
    message_queue<ACommands> recv_world_queue;
    message_queue<AUCommands> send_ups_queue;

    WorldWorker world_worker;

    /////////////////////////////////
    /// Private members start here
    /////////////////////////////////
   public:
    AmazonServer();
    ~AmazonServer();
};

#endif
