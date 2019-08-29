#ifndef __MESSAGEQUEUE__
#define __MESSAGEQUEUE__

#include <deque>
#include <iostream>
#include <mutex>

using namespace std;

template <class T>
class message_queue {
    /////////////////////////////////
    /// Private methods start here
    /////////////////////////////////
   private:
    deque<T> dq;
    mutable mutex m;
    // The postion of message
    int pos;
    int dq_size;
    int next_send;
    // The size of message_queue

    /////////////////////////////////
    /// Public members start here
    /////////////////////////////////
   public:
    // Constructor
    message_queue() : dq(), m(), pos(0), dq_size(0), next_send(0){};
    // Check existence of message and its position
    int where(T value);
    // Check if message_queue is emoty
    bool if_empty();
    // push to back of queue
    void pushback(T value);
    // pop front and then pushback
    bool popfront(T& value);
    // Remove acked message
    T front();
    // Send next message
    T send_next();
    int get_dq_size();
    int get_next_send();

    // Destructor
    ~message_queue(){};
};

template <class T>
int message_queue<T>::get_dq_size() {
    lock_guard<mutex> lock(m);
    return dq_size;
}
template <class T>
int message_queue<T>::get_next_send() {
    lock_guard<mutex> lock(m);
    return next_send;
}

template <class T>
int message_queue<T>::where(T value) {
    lock_guard<mutex> lock(m);
    for (int i = 0; i < dq.size(); i++) {
        if (dq[i] == value) {
            pos = i;
            return pos;
        }
    }
    return -1;
}

template <class T>
bool message_queue<T>::if_empty() {
    lock_guard<mutex> lock(m);
    if (dq_size == next_send)
        return true;
    else
        return false;
}

template <class T>
void message_queue<T>::pushback(T value) {
    lock_guard<mutex> lock(m);
    dq.push_back(value);
    dq_size++;
}

template <class T>
T message_queue<T>::send_next() {
    lock_guard<mutex> lock(m);
    if (next_send != dq_size) {
        next_send++;
        return dq[next_send - 1];
    }
    return dq[dq_size - 1];
}

template <class T>
bool message_queue<T>::popfront(T& value) {
    lock_guard<mutex> lock(m);
    value = dq.front();
    dq.pop_front();
    dq_size--;
    if (next_send > 0) next_send--;
    return true;
}

template <class T>
T message_queue<T>::front() {
    lock_guard<mutex> lock(m);
    return dq.front();
}

#endif
