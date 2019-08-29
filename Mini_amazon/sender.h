#ifndef _SENDER_H
#define _SENDER_H

#include <unistd.h>
#include <thread>

#include "communicator.h"
#include "message_queue.h"

template <class T>
class Sender {
    /////////////////////////////////
    /// Private members start here
    /////////////////////////////////
   private:
    Communicator* communicator;
    // message_queue for sending
    message_queue<pair<long int, T>>& sender_queue;
    // Thread for sending to world
    thread sender_thread;

    /////////////////////////////////
    /// Public members start here
    /////////////////////////////////
   public:
    // Constructor
    Sender<T>(Communicator* c, message_queue<pair<long int, T>>& s_q)
        : communicator(c), sender_queue(s_q) {
        sender_thread = thread(&Sender<T>::start_sending, this);
    };
    // Start receving from web
    void start_sending();
};

template <class T>
void Sender<T>::start_sending() {
    cout << "start sender thread success" << endl;
    while (1) {
        if (sender_queue.get_next_send() == sender_queue.get_dq_size()) {
            usleep(100000);
            continue;
        }
        cout << sender_queue.get_next_send() << "== " << sender_queue.get_dq_size() << endl;
        T message = sender_queue.send_next().second;
        //cout << "pair first:" << sender_queue.send_next().first << endl;
        if (!communicator->send_msg(message)) {
            break;
        } else {
            cout << "Send one" << endl;
        }
    }
}

#endif
