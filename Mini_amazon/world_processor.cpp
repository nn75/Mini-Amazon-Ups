#include "world_processor.h"
#include <pqxx/pqxx>
#include "database_interface.h"

using namespace pqxx;
using namespace std;

WorldProcessor::WorldProcessor(
    message_queue<pair<long int, ACommands> >& s_w_q,
    message_queue<AResponses>& r_w_q,
    message_queue<pair<long int, AUCommands> >& s_u_q,
    WorldCommunicator* world_communicator, long int& wnum, long int& unum,
    mutex& mt)
    : send_world_queue(s_w_q),
      recv_world_queue(r_w_q),
      send_ups_queue(s_u_q),

      world_sender(world_communicator, s_w_q),
      world_receiver(world_communicator, r_w_q),
      world_seqnum(wnum),
      ups_seqnum(unum),
      mtx(mt),
      world_thread(thread(&WorldProcessor::world_command_process, this)) {
    cout << "world processor activate success" << endl;
}

void WorldProcessor::world_command_process() {
    database_interface* dbi = new database_interface();
    int count = 0;
    while (1) {
        count++;
        usleep(10000);
        if(count == 1000){
            if (send_world_queue.get_next_send() != 0){
                long int seq = send_world_queue.front().first;
                pair<long int, ACommands> temp;
                while(seq == -1) {
                    send_world_queue.popfront(temp);
                    if (send_world_queue.get_next_send() == 0) break;
                    seq = send_world_queue.front().first;
                }
                if (send_world_queue.get_next_send() != 0){
                    send_world_queue.popfront(temp);
                    send_world_queue.pushback(temp);
                }
            }
            count=0;
        }
        if (!recv_world_queue.if_empty()) {
            count=0;
            AResponses tmp_msg;
            recv_world_queue.popfront(tmp_msg);
            ACommands ack_res;
            if (tmp_msg.acks_size() != 0) {
                for (int i = 0; i < tmp_msg.acks_size(); i++) {
                    long int ack_seq = tmp_msg.acks(i);
                    long int seq = send_world_queue.front().first;
                    cout << "ack from world:" << ack_seq << endl;
                    pair<long int, ACommands> temp;
                    while (ack_seq != seq) {
                        if (send_world_queue.get_next_send() == 0) break;
                        if (seq == -1) {
                            send_world_queue.popfront(temp);
                            continue;
                        }
                        send_world_queue.popfront(temp);
                        send_world_queue.pushback(temp);
                        seq = send_world_queue.front().first;
                    }
                    if (send_world_queue.get_next_send() == 0) break;
                    send_world_queue.popfront(temp);
                }
            }
            if (tmp_msg.loaded_size() != 0) {
                for (int i = 0; i < tmp_msg.loaded_size(); i++) {
                    long int ship_id = tmp_msg.loaded(i).shipid();
                    long int ship_seq = tmp_msg.loaded(i).seqnum();

                    ack_res.add_acks(ship_seq);
                    pair<long int, ACommands> r_acks(-1, ack_res);
                    send_world_queue.pushback(r_acks);

                    cout << endl;
                    cout << "AResponse ALoaded loaded:" << endl;
                    cout << "loaded ship_id:" << ship_id << endl;
                    cout << "loaded seqnum:" << ship_seq  << endl;
                    cout << endl;

                    // database: if world said order is loaded, update order status to "loaded"
                    string update_to_loaded ="UPDATE orders_order SET status = 'loaded' WHERE tracking_number= " + to_string(ship_id) + ";";
                    dbi->run_query(update_to_loaded); 

                    // ask ups to deliver
                    AUCommands ups_deliver_msg;
                    Deliver* ups_deliver = ups_deliver_msg.add_todeliver();
                    ups_deliver->set_packageid(ship_id);
                    mtx.lock();  //////lock
                    ups_deliver->set_seqnum(ups_seqnum);
                    cout << "Request ups to deliver, seqnum: " << ups_seqnum << endl;
                    pair<long int, AUCommands> ups_deliver_pair(ups_seqnum, ups_deliver_msg);
                    ups_seqnum++;
                    mtx.unlock();  /////unlock
                    send_ups_queue.pushback(ups_deliver_pair);

                    // database: If the todeliver message is sent to ups, update order status to "delivering"
                    string update_to_delivering ="UPDATE orders_order SET status = 'delivering' WHERE tracking_number= " + to_string(ship_id) + ";";
                    dbi->run_query(update_to_delivering); 
                }
            }
            if (tmp_msg.ready_size() != 0) {
                for (int i = 0; i < tmp_msg.ready_size(); i++) {
                    long int ship_id = tmp_msg.ready(i).shipid();
                    long int ship_seq = tmp_msg.ready(i).seqnum();

                    ack_res.add_acks(ship_seq);
                    pair<long int, ACommands> r_acks(-1, ack_res);
                    send_world_queue.pushback(r_acks);

                    cout << endl;                    
                    cout << "AResponse APacked ready:" << endl;
                    cout << "ready ship_id:" << ship_id << endl;
                    cout << "ready seqnum:" << ship_seq  << endl;
                    cout << endl;

                    // database: if world said the order is packed, update order status to "packed"
                    string update_to_packed =
                        "UPDATE orders_order SET status = 'packed' WHERE "
                        "tracking_number= " +
                        to_string(ship_id) + ";";
                    dbi->run_query(update_to_packed);

                    // check truck if truck_num != -1, send load message to world
                    string check_truck =
                        "SELECT * FROM orders_order WHERE tracking_number = " +
                        to_string(ship_id) + " AND truck_id != -1 ;";
                    vector<vector<string>> res = dbi->run_query_with_results(check_truck);
                    if (res.size() == 1) {
                        //If there's truck arrived
                        cout << "truck arrived" << endl;

                        ACommands world_load_msg;
                        APutOnTruck* put_on_truck = world_load_msg.add_load();
                        put_on_truck->set_whnum(atoi(res[0][4].c_str()));
                        put_on_truck->set_truckid(atoi(res[0][5].c_str()));
                        put_on_truck->set_shipid((long int)atoi(res[0][0].c_str()));
                        mtx.lock();  //////lock
                        put_on_truck->set_seqnum(world_seqnum);
                        pair<long int, ACommands> world_load_pair(world_seqnum, world_load_msg);
                        world_seqnum++;
                        mtx.unlock();  /////unlock
                        send_world_queue.pushback(world_load_pair);
                        // After packed and truck arrived, update order status to loading
                        string update_to_loading ="UPDATE orders_order SET status = 'loading' WHERE tracking_number= " + to_string(ship_id) + ";";
                        dbi->run_query(update_to_loading);
                    }else{
                        cout << "Truck haven't arrived" << endl;
                    }
                }
            }
            if (tmp_msg.arrived_size() != 0) {  // Process the response of purchursemore
                for (int i = 0; i < tmp_msg.arrived_size(); i++) {
                    // parse the arrived message;
                    int wh_num = tmp_msg.arrived(i).whnum();
                    long int product_id = tmp_msg.arrived(i).things(0).id();
                    string product_desciption =
                        tmp_msg.arrived(i).things(0).description();
                    int product_count = tmp_msg.arrived(i).things(0).count();
                    long int arrived_seqnum = tmp_msg.arrived(i).seqnum();

                    // add ack to ack response
                    ack_res.add_acks(arrived_seqnum);
                    pair<long int, ACommands> r_acks(-1, ack_res);
                    send_world_queue.pushback(r_acks);

                    cout << endl;
                    cout << "AResponse APurchaseMore arrived:" << endl;
                    cout << "arrived whnum:" << wh_num << endl;
                    cout << "arrived things -> id:" << product_id << endl;
                    cout << "arrived things -> description:"
                         << product_desciption << endl;
                    cout << "arrived things -> count:" << product_count << endl;
                    cout << "arrived seqnum:" << arrived_seqnum << endl;
                    cout << endl;

                    // database: If new product stock arrived at warehouse,
                    // update product table, increase its stock to 500
                    string increase_stock =
                        "UPDATE orders_product SET stock = stock + 500 WHERE product_id = " + to_string(product_id) +
                        " AND wh_id = " + to_string(wh_num) + ";";
                    dbi->run_query(increase_stock);

                    //Find the first stocking order meet prduct_id, count and wh_num, set its status to packing
                    string first_enough_order = "SELECT * FROM orders_order WHERE product_id = " + to_string(product_id) +
                        " AND wh_id = " + to_string(wh_num) + " AND amount = " + to_string(product_count - 500) + " LIMIT 1 ;";
                    vector<vector<string>> res_enough= dbi->run_query_with_results(first_enough_order);
                    int order_id = 0;
                    if(res_enough.size() == 1){
                        cout << "Buy one product enough in warehouse" << endl;
                        order_id = atoi(res_enough[0][0].c_str());
                        string stocking_to_packing = "UPDATE orders_order SET status = 'packing' WHERE tracking_number = "+to_string(order_id)+";";
                        dbi->run_query(stocking_to_packing);

                        //Minus stock
                        int new_product_count = product_count - 500;
                        string minus_stock = "UPDATE orders_product SET stock = stock-"+to_string(new_product_count)+" WHERE product_id = " + to_string(product_id) +
                        " AND wh_id = " + to_string(wh_num) + ";";
                        dbi->run_query(minus_stock);

                        //And send message pack message to world
                        ACommands topack;
                        APack *ap_topack = topack.add_topack();
                        ap_topack->set_whnum(wh_num);
                        AProduct *pd_topack = ap_topack->add_things();
                        pd_topack->set_id(product_id);
                        pd_topack->set_description(product_desciption);
                        pd_topack->set_count(product_count - 500);
                        ap_topack->set_shipid(order_id);
                        mtx.lock();  //////lock
                        ap_topack->set_seqnum(world_seqnum);
                        pair<long int, ACommands> topack_pair(world_seqnum, topack);
                        world_seqnum++;
                        mtx.unlock();  /////unlock
                        send_world_queue.pushback(topack_pair);
                        cout << "After buy more, stock enough, send topack to world" << endl;

                        //Get order_info address_x, address_y, ups_account,
                        string get_order_info = "SELECT * FROM orders_order WHERE tracking_number = "+ to_string(order_id) +";"; //status = 'packing' AND 
                        vector<vector<string>> res_info= dbi->run_query_with_results(get_order_info);
                        string ups_account = res_info[0][2];
                        int address_x = atoi(res_info[0][7].c_str());
                        int address_y = atoi(res_info[0][8].c_str());
                        
                        // Send truck message to ups
                        AUCommands od;
                        Order* ord = od.add_order();
                        ord->set_whid(wh_num);
                        ord->set_x(address_x);
                        ord->set_y(address_y);
                        ord->set_packageid(order_id);
                        ord->set_upsusername(ups_account);
                        Product* pd = ord->add_item();
                        pd->set_id(product_id);
                        pd->set_description(product_desciption);
                        pd->set_amount(product_count - 500);
                        mtx.lock();//////lock
                        ord->set_seqnum(ups_seqnum);
                        pair<long int, AUCommands> order_pair(ups_seqnum, od);
                        cout << "Request ups tosend truck, seqnum: " << ups_seqnum << endl;
                        ups_seqnum++;
                        mtx.unlock();/////unlock
                        send_ups_queue.pushback(order_pair);
                        cout << "After buy more, stock enough, send topack to world" << endl;

                    }else{
                        cout << "Initialing product in warehouse:" << endl;
                        // string init_p_w = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (1, 1, 'Apple',1,100);";
                        // dbi->run_query(init_p_w);
                        string init_stock_100 = "UPDATE orders_product SET stock = 100 WHERE product_id = " + to_string(product_id) + " AND wh_id = " + to_string(wh_num) + ";";
                        dbi->run_query(init_stock_100);
                    }
                }
            }
            if (tmp_msg.error_size() != 0) {  // Process the response of purchursemore
                for (int i = 0; i < tmp_msg.error_size(); i++) {
                    // parse the arrived message;
                    string err = tmp_msg.error(i).err();
                    int originseqnum = tmp_msg.error(i).originseqnum ();
                    int seqnum= tmp_msg.error(i).seqnum();

                    // add ack to ack response
                    ack_res.add_acks(seqnum);
                    pair<long int, ACommands> r_acks(-1, ack_res);
                    send_world_queue.pushback(r_acks);

                    cout << endl;
                    cout << "AResponse AErr error:" << endl;
                    cout << "error err:" << err << endl;
                    cout << "error originseqnum:" << originseqnum<< endl;
                    cout << "error seqnum:" << seqnum<< endl;
                    cout << endl;
                }
            }
        }
    }
}