#ifndef _RECEIVER_H
#define _RECEIVER_H

#include <thread>

#include "communicator.h"
#include "message_queue.h"

template <class T>
class Receiver {
    /////////////////////////////////
    /// Private members start here
    /////////////////////////////////
   private:
    Communicator* communicator;
    // message_queue for receving
    message_queue<T>& recv_queue;
    // message_queue<T> recv_queue;
    thread receiver_thread;

    /////////////////////////////////
    /// Public members start here
    /////////////////////////////////
   public:
    // Constructor
    Receiver<T>(Communicator* c, message_queue<T>& r_q)
        : communicator(c), recv_queue(r_q) {
        receiver_thread = thread(&Receiver<T>::start_receiving, this);
    };
    // Start receving from web
    void start_receiving();
};

template <class T>
void Receiver<T>::start_receiving() {
    cout << "start recevier thread success" << endl;
    while (1) {
        T message;
        if (!communicator->recv_msg(message)) {
            break;
        } else {
            cout << "Received one" << endl;
        }
        recv_queue.pushback(message);
    }
}
#endif
