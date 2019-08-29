#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include <iostream>
#include <pqxx/pqxx>


#include "amazon_ups.pb.h"
#include "message_queue.h"
#include "world_amazon.pb.h"
#include "amazon_ups.pb.h"

#include "ups_communicator.h"
#include "world_communicator.h"

#include "ups_processor.h"
#include "web_processor.h"
#include "world_processor.h"
#include "database_interface.h"

using namespace std;
using namespace pqxx;


int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Get worldid ups people told us
    if (argc != 2) {
        printf("Syntax Error: ./Mini_amazon <worldid>\n");
        exit(EXIT_FAILURE);
    }
    long worldid_input = atoi(argv[1]);
    cout << "worldid_input is:" << worldid_input << endl;

    cout << "Initializing system, please wait..." << endl;
    cout << endl;
    cout << endl;
    database_interface* dbi = new database_interface();
    string clear_warehouse_table = "TRUNCATE TABLE warehouse;";
    dbi->run_query(clear_warehouse_table);
    string clear_product_table = "TRUNCATE TABLE orders_product;";
    dbi->run_query(clear_product_table);
    string clear_order_table = "TRUNCATE TABLE orders_order;";
    dbi->run_query(clear_order_table);
    string clear_pendingorder_table = "TRUNCATE TABLE orders_pendingorder;";
    dbi->run_query(clear_pendingorder_table);
    

    // Initialize warehouse and world_communicator
    Warehouse houses[3] = {{1, 0, 0}, {2, 10, 10}, {3, 20, 20}};
    WorldCommunicator* world_communicator = new WorldCommunicator(3, houses);

    // Initialize warehouse and world_communicator
    UpsCommunicator* ups_communicator = new UpsCommunicator(0, NULL);

    // Initialize message queue and multithread
    message_queue<pair<long int, ACommands> > s_w_q;   // Send world queue
    message_queue<AResponses> r_w_q;                   // Receive world queue
    message_queue<pair<long int, AUCommands> > s_u_q;  // Send ups queue
    message_queue<UACommands> r_u_q;                   // Receive ups queue

    long int wnum = 0;
    long int unum = 0;
    mutex mt;

    // Test for world connect and create
    // cout << "Connect to 127.0.0.1 without worldid" << endl;
    // world_communicator->connect("localhost");

    //Test for connect to worldid told by ups
    cout << "Connect world: worldid "<< worldid_input <<endl; 
    //world_communicator->connect("67.159.94.99", worldid_input);
    //world_communicator->connect("67.159.95.162", worldid_input);//For my test
    //world_communicator->connect("67.159.88.195", worldid_input);//This ups world one
    world_communicator->connect("152.3.52.86", worldid_input);//This ups world two

    // Test for Ups socket
    //UpsCommunicator ups_communicator(0, NULL);
    cout << "Connect Ups: Connect to vcm-6873.vm.duke.edu" << endl;
    //ups_communicator->connect("67.159.94.99", worldid_input);
    //ups_communicator->connect("67.159.88.195", worldid_input);//ups one
    ups_communicator->connect("152.3.77.102", worldid_input);//ups two

    /////////////////////////////////
    /// Testing message start here
    /////////////////////////////////
    // Initialize processors
    WorldProcessor* world_processor = new WorldProcessor(s_w_q, r_w_q, s_u_q, world_communicator, wnum, unum, mt);
    UpsProcessor* ups_processor = new UpsProcessor(s_w_q, r_u_q, s_u_q, ups_communicator, wnum, unum, mt); 
    WebProcessor* web_processor = new WebProcessor(s_w_q, s_u_q, wnum, unum, mt); 

    // Test for web connect
    web_processor->connect();

    // send ACommand to world to prepare product in warehouses
    AUCommands wh_init_1;
    AWarehouse * wh1 = wh_init_1.add_whinfo();
    wh1->set_id(1);
    wh1->set_x(0);
    wh1->set_y(0);
    mt.lock();//////lock
    wh1->set_seqnum(unum);
    pair<long int, AUCommands> wh_init_1_pair(unum, wh_init_1);
    unum++;
    mt.unlock();/////unlock
    s_u_q.pushback(wh_init_1_pair);
    cout << "Send ups warehouse1\n";

    AUCommands wh_init_2;
    AWarehouse * wh2 = wh_init_2.add_whinfo();
    wh2->set_id(2);
    wh2->set_x(10);
    wh2->set_y(10);
    mt.lock();//////lock
    wh2->set_seqnum(unum);
    pair<long int, AUCommands> wh_init_2_pair(unum, wh_init_2);
    unum++;
    mt.unlock();/////unlock
    s_u_q.pushback(wh_init_2_pair);
    cout << "Send ups warehouse2\n";

    AUCommands wh_init_3;
    AWarehouse * wh3 = wh_init_3.add_whinfo();
    wh3->set_id(3);
    wh3->set_x(20);
    wh3->set_y(20);
    mt.lock();//////lock
    wh3->set_seqnum(unum);
    pair<long int, AUCommands> wh_init_3_pair(unum, wh_init_3);
    unum++;
    mt.unlock();/////unlock
    s_u_q.pushback(wh_init_3_pair);
    cout << "Send ups warehouse3\n";

    string init_w_1 = "INSERT INTO warehouse(wh_id, wh_x, wh_y) VALUES (1, 0, 0);";
    dbi->run_query(init_w_1);
    string init_w_2 = "INSERT INTO warehouse(wh_id, wh_x, wh_y) VALUES (2, 10, 10);";
    dbi->run_query(init_w_2);
    string init_w_3 = "INSERT INTO warehouse(wh_id, wh_x, wh_y) VALUES (3, 20, 20);";
    dbi->run_query(init_w_3);

    
    // //Initialize an ACommand to buy more, and push into send world queue
    //Warehouse 1 product
    ACommands pm11;
    APurchaseMore* apm11 = pm11.add_buy();
    apm11->set_whnum(1);
    AProduct* pd11 = apm11->add_things();
    pd11->set_id(1);
    pd11->set_description("Apple");
    pd11->set_count(100);
    mt.lock();  //////lock
    apm11->set_seqnum(wnum);
    pair<long int, ACommands> inti_p1_w1(wnum, pm11);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p1_w1);

    ACommands pm21;
    APurchaseMore* apm21= pm21.add_buy();
    apm21->set_whnum(1);
    AProduct* pd21 = apm21->add_things();
    pd21->set_id(2);
    pd21->set_description("Bag");
    pd21->set_count(100);
    mt.lock();  //////lock
    apm21->set_seqnum(wnum);
    pair<long int, ACommands> inti_p2_w1(wnum, pm21);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p2_w1);

    ACommands pm31;
    APurchaseMore* apm31 = pm31.add_buy();
    apm31->set_whnum(1);
    AProduct* pd31 = apm31->add_things();
    pd31->set_id(3);
    pd31->set_description("Comb");
    pd31->set_count(100);
    mt.lock();  //////lock
    apm31->set_seqnum(wnum);
    pair<long int, ACommands> inti_p3_w1(wnum, pm31);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p3_w1);

    //Warehouse 2 product
    ACommands pm12;
    APurchaseMore* apm12 = pm12.add_buy();
    apm12->set_whnum(2);
    AProduct* pd12 = apm12->add_things();
    pd12->set_id(1);
    pd12->set_description("Apple");
    pd12->set_count(100);
    mt.lock();  //////lock
    apm12->set_seqnum(wnum);
    pair<long int, ACommands> inti_p1_w2(wnum, pm12);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p1_w2);

    ACommands pm22;
    APurchaseMore* apm22= pm22.add_buy();
    apm22->set_whnum(2);
    AProduct* pd22 = apm22->add_things();
    pd22->set_id(2);
    pd22->set_description("Bag");
    pd22->set_count(100);
    mt.lock();  //////lock
    apm22->set_seqnum(wnum);
    pair<long int, ACommands> inti_p2_w2(wnum, pm22);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p2_w2);

    ACommands pm32;
    APurchaseMore* apm32 = pm32.add_buy();
    apm32->set_whnum(2);
    AProduct* pd32 = apm32->add_things();
    pd32->set_id(3);
    pd32->set_description("Comb");
    pd32->set_count(100);
    mt.lock();  //////lock
    apm32->set_seqnum(wnum);
    pair<long int, ACommands> inti_p3_w2(wnum, pm32);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p3_w2);

    ACommands pm13;
    APurchaseMore* apm13 = pm13.add_buy();
    apm13->set_whnum(3);
    AProduct* pd13 = apm13->add_things();
    pd13->set_id(1);
    pd13->set_description("Apple");
    pd13->set_count(100);
    mt.lock();  //////lock
    apm13->set_seqnum(wnum);
    pair<long int, ACommands> inti_p1_w3(wnum, pm13);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p1_w3);

    ACommands pm23;
    APurchaseMore* apm23= pm23.add_buy();
    apm23->set_whnum(3);
    AProduct* pd23 = apm23->add_things();
    pd23->set_id(2);
    pd23->set_description("Bag");
    pd23->set_count(100);
    mt.lock();  //////lock
    apm23->set_seqnum(wnum);
    pair<long int, ACommands> inti_p2_w3(wnum, pm23);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p2_w3);

    ACommands pm33;
    APurchaseMore* apm33 = pm33.add_buy();
    apm33->set_whnum(3);
    AProduct* pd33 = apm33->add_things();
    pd33->set_id(3);
    pd33->set_description("Comb");
    pd33->set_count(100);
    mt.lock();  //////lock
    apm33->set_seqnum(wnum);
    pair<long int, ACommands> inti_p3_w3(wnum, pm33);
    wnum++;
    mt.unlock();  /////unlock
    s_w_q.pushback(inti_p3_w3);

    //Test for resend
    // usleep(10000);

    // AResponses sim_other_ack;
    // sim_other_ack.add_acks(1);
    // r_w_q.pushback(sim_other_ack);

    string init_p1_w1 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (1, 1, 'Apple',1,100);";
    dbi->run_query(init_p1_w1);
    string init_p2_w1 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (2, 2, 'Bag',1,100);";
    dbi->run_query(init_p2_w1);
    string init_p3_w1 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (3, 3, 'Comb',1,100);";
    dbi->run_query(init_p3_w1);

    string init_p1_w2 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (4, 1, 'Apple',2,100);";
    dbi->run_query(init_p1_w2);
    string init_p2_w2 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (5, 2, 'Bag',2,100);";
    dbi->run_query(init_p2_w2);
    string init_p3_w2 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (6, 3, 'Comb',2,100);";
    dbi->run_query(init_p3_w2);

    string init_p1_w3 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (7, 1, 'Apple',3,100);";
    dbi->run_query(init_p1_w3);
    string init_p2_w3 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (8, 2, 'Bag',3,100);";
    dbi->run_query(init_p2_w3);
    string init_p3_w3 = "INSERT INTO orders_product(id, product_id, product_name, wh_id, stock) VALUES (9, 3, 'Comb',3,100);";
    dbi->run_query(init_p3_w3);

    cout << "System start, enjoy shopping" << endl;
    // Keep running
    while (1) {
        ;
    }

    world_communicator->disconnect();
    ups_communicator->disconnect();
    web_processor->disconnect();

    // google::protobuf::ShutdownProtobufLibrary();
}
