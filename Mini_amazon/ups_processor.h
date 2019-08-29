#ifndef __UPSPROCESSOR__H_
#define __UPSPROCESSOR__H_

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <iostream>
#include <thread>

#include "amazon_ups.pb.h"
#include "receiver.h"
#include "sender.h"
#include "ups_communicator.h"
#include "world_amazon.pb.h"

using namespace std;

class UpsProcessor {
    /////////////////////////////////
    /// Private members start here
    /////////////////////////////////
   private:
    // target's port number
    message_queue<pair<long int, ACommands>>& send_world_queue;
    message_queue<UACommands>& recv_ups_queue;
    message_queue<pair<long int, AUCommands>>& send_ups_queue;
    Sender<AUCommands> ups_sender;
    Receiver<UACommands> ups_receiver;
    long int& world_seqnum;
    long int& ups_seqnum;
    mutex& mtx;
    thread ups_thread;
    /////////////////////////////////
    /// Private members start here
    /////////////////////////////////
   public:
    UpsProcessor(message_queue<pair<long int, ACommands>>& mq1,
                 message_queue<UACommands>& mq2,
                 message_queue<pair<long int, AUCommands>>& mq3,
                 UpsCommunicator* ups_communicator, long int& wnum,
                 long int& unum, mutex& mt);
    ~UpsProcessor(){};
    void ups_command_process();
};

#endif
