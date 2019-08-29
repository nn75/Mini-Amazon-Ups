#include "ups_processor.h"
#include "database_interface.h"
#include <pqxx/pqxx>

using namespace pqxx;
using namespace std;
UpsProcessor::UpsProcessor(message_queue<pair<long int, ACommands>>& mq1,
                           message_queue<UACommands>& mq2,
                           message_queue<pair<long int, AUCommands>>& mq3,
                           UpsCommunicator* ups_communicator, long int& wnum,
                           long int& unum, mutex& mt)
    : send_world_queue(mq1),
      recv_ups_queue(mq2),
      send_ups_queue(mq3),

      ups_sender(ups_communicator, mq3),
      ups_receiver(ups_communicator, mq2),
      world_seqnum(wnum),
      ups_seqnum(unum),
      mtx(mt),
      ups_thread(thread(&UpsProcessor::ups_command_process, this)) {
    cout << "ups processor activate success" << endl;
}

void UpsProcessor::ups_command_process() {
    database_interface* dbi = new database_interface();
    int count = 0;
    while (1) {
        count++;
        usleep(10000);
        if(count == 1000){
            if (send_ups_queue.get_next_send() != 0){
                long int seq = send_ups_queue.front().first;
                pair<long int, AUCommands> temp;
                while(seq == -1) {
                    send_ups_queue.popfront(temp);
                    if (send_ups_queue.get_next_send() == 0) break;
                    seq = send_ups_queue.front().first;
                }
                if (send_ups_queue.get_next_send() != 0){
                    send_ups_queue.popfront(temp);
                    send_ups_queue.pushback(temp);
                }
            }
            count=0;
        }
        if (!recv_ups_queue.if_empty()) {
            count=0;
            UACommands tmp_msg;
            recv_ups_queue.popfront(tmp_msg);
            AUCommands ack_res;
            if (tmp_msg.acks_size() != 0) {
                for (int i = 0; i < tmp_msg.acks_size(); i++) {
                    long int ack_seq = tmp_msg.acks(i);
                    long int seq = send_ups_queue.front().first;
                    cout << "ack from ups:" << ack_seq << endl;
                    pair<long int, AUCommands> temp;
                    while (ack_seq != seq) {
                        if (send_ups_queue.get_next_send() == 0) break;
                        if (seq == -1) {
                            send_ups_queue.popfront(temp);
                            continue;
                        }
                        send_ups_queue.popfront(temp);
                        send_ups_queue.pushback(temp);
                        seq = send_ups_queue.front().first;
                    }
                    if (send_ups_queue.get_next_send() == 0) break;
                    send_ups_queue.popfront(temp);
                }
            }
            if (tmp_msg.arrived_size() != 0) {
                for (int i = 0; i < tmp_msg.arrived_size(); i++) {
                    int wh_num = tmp_msg.arrived(i).whid();
                    int truck_id = tmp_msg.arrived(i).truckid();
                    long int package_id = tmp_msg.arrived(i).packageid();
                    long int arrived_seq = tmp_msg.arrived(i).seqnum();

                    cout << endl;
                    cout << "UACommand Truck arrived:" << endl;
                    cout << "arrived wh_id:" << wh_num << endl;
                    cout << "arrived truck_id:" << truck_id<< endl;
                    cout << "arrived package_id:" << package_id << endl;
                    cout << "arrived arrived_seq :" << arrived_seq   << endl;
                    cout << endl;
                    
                    // add ack to ack ups
                    ack_res.add_acks(arrived_seq);
                    pair<long int, AUCommands> r_acks(-1, ack_res);
                    send_ups_queue.pushback(r_acks);

                    // database: If the ups said the truck is arrived, update
                    // the truck_id in database from -1 to given number
                    string update_truck =
                        "UPDATE orders_order SET truck_id = " +
                        to_string(truck_id) +
                        " WHERE tracking_number= " + to_string(package_id) +
                        ";";
                    dbi->run_query(update_truck);

                    // check status of order, if status == packed, send load
                    // message to world
                    string check_pack =
                        "SELECT * FROM orders_order WHERE tracking_number = " +
                        to_string(package_id) + " AND status = 'packed' ;";
                    vector<vector<string>> res_packed= dbi->run_query_with_results(check_pack);
                    if (res_packed.size() == 1) {
                        cout << "Truck arrived and packed" << endl;
                        ACommands world_load_msg;
                        APutOnTruck* put_on_truck = world_load_msg.add_load();
                        put_on_truck->set_whnum(atoi(res_packed[0][4].c_str()));
                        put_on_truck->set_truckid(atoi(res_packed[0][5].c_str()));
                        put_on_truck->set_shipid((long int)atoi(res_packed[0][0].c_str()));

                        mtx.lock();  //////lock
                        put_on_truck->set_seqnum(world_seqnum);
                        pair<long int, ACommands> world_load_pair(world_seqnum, world_load_msg);
                        world_seqnum++;
                        mtx.unlock();  /////unlock
                        send_world_queue.pushback(world_load_pair);

                        // After packed and truck arrived, update order status
                        // to loading
                        string update_to_loading =
                            "UPDATE orders_order SET status = 'loading' WHERE tracking_number= " + to_string(package_id) + ";";
                        dbi->run_query(update_to_loading);
                    }
                }
            }
            if (tmp_msg.finish_size() != 0) {
                for (int i = 0; i < tmp_msg.finish_size(); i++) {
                    long int package_id = tmp_msg.finish(i).packageid();
                    long int finish_seq = tmp_msg.finish(i).seqnum();

                    cout << endl;
                    cout << "UACommand Delivered finish:" << endl;
                    cout << "ups finish package_id" << package_id << endl;
                    cout << "ups finish finish_seq" << finish_seq << endl;
                    cout << endl;

                    // add ack to ack ups
                    ack_res.add_acks(finish_seq);
                    pair<long int, AUCommands> r_acks(-1, ack_res);
                    send_ups_queue.pushback(r_acks);

                    // database: If ups said
                    string update_delivered = "UPDATE orders_order SET status='delivered' WHERE tracking_number = " + to_string(package_id) + ";";
                    dbi->run_query(update_delivered);
                }
            }
        }  // not if_empty
    }      // While
}
