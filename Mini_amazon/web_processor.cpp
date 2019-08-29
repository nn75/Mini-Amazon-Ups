#include <arpa/inet.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <thread>

#include "web_processor.h"
#include "database_interface.h"

using namespace std;
using namespace pqxx;

// Constructor for Communicator
WebProcessor::WebProcessor(message_queue<pair<long int, ACommands> > &s_w_q,
                           message_queue<pair<long int, AUCommands> > &s_u_q,
                           long int &wnum, long int &unum, mutex &mt)
    : send_world_queue(s_w_q),
      send_ups_queue(s_u_q),
      world_seqnum(wnum),
      ups_seqnum(unum),
      mtx(mt) {
    sock_fd = INVALID_FD;
    web_client_fd = INVALID_WEB_FD;
    type = "Web";
}

// Connect and start receiving
bool WebProcessor::connect() {
    // Connect socket with server
    if (!this->setup_sock()) {
        this->fail_connect("Setup web socket failed");
        return false;
    }

    // For testing
    // this->start_recv();

    // For thread
    web_recevier_thread = thread(&WebProcessor::start_recv, this);
    cout << "start web recevier thread success\n";

    return true;
}

void WebProcessor::fail_connect(const char *err_msg) {
    cerr << type << " Err:  " << err_msg << endl;
    this->disconnect();
}

void WebProcessor::disconnect() {
    if (sock_fd != INVALID_FD) {
        close(sock_fd);
    }
    sock_fd = INVALID_FD;
}

// Set up socket with web
bool WebProcessor::setup_sock() {
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    const char *port = "45678";

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        this->fail_connect("Cannot get address info for Amazon");
        return false;
    }

    sock_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
    if (sock_fd == -1) {
        this->fail_connect("Cannot create web socket");
        return false;
    }

    int yes = 1;
    status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status =
        ::bind(sock_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        this->fail_connect("Cannot bind web socket");
        return false;
    }
    status = listen(sock_fd, 100);
    if (status == -1) {
        this->fail_connect("Cannot listen on web socket");
        return false;
    }
    cout << "web processor activate success\n";

    return true;
}

// Start receving from web
void WebProcessor::start_recv() {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    while (1) {
        web_client_fd =
            accept(sock_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (web_client_fd != -1) {
            this->get_buy_info();
            web_client_fd = INVALID_WEB_FD;
        } else {
            cout << "One user: Connect Success!" << endl;
        }
    }
}

vector<string> WebProcessor::split(const string &str, const string &sep) {
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(sep, prev);
        if (pos == string::npos) {
            pos = str.length();
        }
        string token = str.substr(prev, pos - prev);
        if (!token.empty()) {
            tokens.push_back(token);
        }
        prev = pos + sep.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

// Parse info received from web
void WebProcessor::get_buy_info() {
    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));

    recv(web_client_fd, buffer, 2048, 0);

    string info(buffer);
    vector<string> tokens = split(info, "/");
    string tracking_number = tokens[0];
    // string ups_account = tokens[1];

    cout << "New Order Tracking number: " << tracking_number << endl;
    //      << "\nUPS account: " << ups_account << endl;

    database_interface* dbi = new database_interface();

    // Get info from orders_pendingorder
    string get_pending_order =
        "SELECT * FROM orders_pendingorder WHERE tracking_number = " +
        tracking_number + ";";
    vector<vector<string>> res_pending = dbi->run_query_with_results(get_pending_order);
    long int tracking_number_int = (long int) atoi(tracking_number.c_str());
    int user_id;
    string product_name;
    int amount;
    int address_x;
    int address_y;
    string ups_account = "";
    if (res_pending.size() == 1) {
        user_id = atoi(res_pending[0][2].c_str());
        product_name = res_pending[0][3];
        amount = atoi(res_pending[0][4].c_str());
        address_x = atoi(res_pending[0][5].c_str());
        address_y = atoi(res_pending[0][6].c_str());
        ups_account = res_pending[0][7];
    } else {
        cout << "Use tracking number to get pending order failed\n";
    }

    //Decide which warehouse to buy more stock according to dist
    string decide_warehouse = "SELECT * FROM warehouse;";
    vector<vector<string>> res_wh = dbi->run_query_with_results(decide_warehouse);
    int min_dist = INT_MAX;
    int closest_wh_id = 0;
    int closest_wh_x = 0;
    int closest_wh_y = 0;
    for (int i = 0; i < res_wh.size(); i++) {
        int wh_id = atoi(res_wh[i][0].c_str());
        int wh_x = atoi(res_wh[i][1].c_str());
        int wh_y = atoi(res_wh[i][2].c_str());
        int curr_dist = sqrt((address_x - wh_x) * (address_x - wh_x) +
                             (address_y - wh_y) * (address_y - wh_y));
        if (curr_dist < min_dist) {
            min_dist = curr_dist;
            closest_wh_id = wh_id;
            closest_wh_x = wh_x;
            closest_wh_y = wh_y;
        }
    }
    cout << "The closest warehouse id is:" << closest_wh_id << endl;

    //Get the product_id from the closest warehouse
    string get_product_id = "SELECT * FROM orders_product WHERE wh_id = " +
                            to_string(closest_wh_id) +
                            " AND product_name = '" + product_name + "';";
    vector<vector<string>> res_pid = dbi->run_query_with_results(get_product_id);
    if (res_pid .size() == 1) {// There exists such product in this warehouse
        int product_id = atoi(res_pid[0][1].c_str());
        int stock = atoi(res_pid[0][4].c_str());

        // If the (stock - amount) < 100, send purchase more message to world, leave order in pending order
        if (stock - amount < 100) {  
            // Add to real order list, set status stocking
            std::string add_stocking_order =
                "INSERT INTO orders_order(tracking_number, user_id, "
                "ups_account, "
                "product_id, wh_id, truck_id,status,adr_x,adr_y, amount) VALUES ( " +
                to_string(tracking_number) + ", " + to_string(user_id) + ", '" +
                ups_account + "', " + to_string(product_id) + ", " +
                to_string(closest_wh_id) + ", " + to_string(-1) + ", " +
                "'stocking'" + ", " + to_string(address_x) + ", " +
                to_string(address_y) + ", "+ to_string(amount) +");";
            dbi->run_query(add_stocking_order);

            ////Send purchase more message to world
            ACommands buy;
            APurchaseMore *apm_buy = buy.add_buy();
            apm_buy->set_whnum(closest_wh_id);
            AProduct *pd_buy = apm_buy->add_things();
            pd_buy->set_id(product_id);
            pd_buy->set_description(product_name);
            pd_buy->set_count(amount + 500);
            mtx.lock();  //////lock
            apm_buy->set_seqnum(world_seqnum);
            pair<long int, ACommands> buy_pair(world_seqnum, buy);
            cout << "web processor send world seqnum:" << world_seqnum << endl;
            world_seqnum++;
            mtx.unlock();  /////unlock
            send_world_queue.pushback(buy_pair);
            cout << "Stock not enough, ask world to buy more" << endl;

        } else if (stock - amount >= 100) {  
            // If stock enough send pack message to world and send truck message to ups, add it to real order table
            // Add to real order list, set status packing
            string add_packing_order =
                "INSERT INTO orders_order(tracking_number, user_id, "
                "ups_account, "
                "product_id, wh_id, truck_id,status,adr_x,adr_y, amount) VALUES ( " +
                to_string(tracking_number) + ", " + to_string(user_id) + ", '" +
                ups_account + "', " + to_string(product_id) + ", " +
                to_string(closest_wh_id) + ", " + to_string(-1) + ", " +
                "'packing'" + ", " + to_string(address_x) + ", " +
                to_string(address_y) + ", " + to_string(amount) + ");";
            dbi->run_query(add_packing_order);

            //Minus stock of product
            string minus_stock = "UPDATE orders_product SET stock = stock-"+to_string(amount)+" WHERE product_id = " + to_string(product_id) +
                        " AND wh_id = " + to_string(closest_wh_id) + ";";
            dbi->run_query(minus_stock);

            // Send pack message to world
            ACommands topack;
            APack *ap_topack = topack.add_topack();
            ap_topack->set_whnum(closest_wh_id);
            AProduct *pd_topack = ap_topack->add_things();
            pd_topack->set_id(product_id);
            pd_topack->set_description(product_name);
            pd_topack->set_count(amount);
            ap_topack->set_shipid(tracking_number_int);
            mtx.lock();  //////lock
            ap_topack->set_seqnum(world_seqnum);
            pair<long int, ACommands> topack_pair(world_seqnum, topack);
            world_seqnum++;
            mtx.unlock();  /////unlock
            send_world_queue.pushback(topack_pair);
            cout << "Stock enough send topack to world" << endl;

            // Send truck message to ups
            AUCommands od;
            Order* ord = od.add_order();
            ord->set_whid(closest_wh_id);
            ord->set_x(address_x);
            ord->set_y(address_y);
            ord->set_packageid(tracking_number_int);
            ord->set_upsusername(ups_account);
            Product* pd = ord->add_item();
            pd->set_id(product_id);
            pd->set_description(product_name);
            pd->set_amount(amount);
            mtx.lock();//////lock
            ord->set_seqnum(ups_seqnum);
            pair<long int, AUCommands> order_pair(ups_seqnum, od);
            cout << "ups_seqnum ask ups to send truck: " << ups_seqnum << endl;
            cout << "ups_account is: " << ups_account << endl;
            ups_seqnum++;
            mtx.unlock();/////unlock
            send_ups_queue.pushback(order_pair);
            cout << "Stock enough send truck to ups" << endl;
        }else{
            cout << "Stock limit error" << endl;
        }
    } else {
        //There's no such product in this warehouse, create a new product in this warehouse
        //cout << "Use wh_id and product_name to get product failed\n";
        mtx.lock(); 
        //Get product id
        string whether_new_product = "SELECT * FROM orders_product WHERE product_name = '" + product_name + "';";
        vector<vector<string>> res_new_old = dbi->run_query_with_results(whether_new_product);
        int insert_new_product_id = 0;
        if(res_new_old.size() == 1){
            cout << "Product exists in world, but not in warehouse" << closest_wh_id << endl;
            insert_new_product_id = atoi(res_new_old[0][1].c_str());
        }else{
            cout << "Product not exists in world, create new"<< endl;
            string get_new_product_id = "SELECT COUNT(DISTINCT product_name) FROM orders_product";
            vector<vector<string> > res_new_product_id = dbi->run_query_with_results(get_new_product_id);
            insert_new_product_id = 1 + atoi(res_new_product_id[0][0].c_str());
            cout << "insert_new_product_id is:" << insert_new_product_id << endl;
        }
        //Count total items in orders_product
        string get_new_id = "SELECT COUNT(*) FROM orders_product";
        vector<vector<string> > res_new_id = dbi->run_query_with_results(get_new_id);
        int biggest_existing = atoi(res_new_id[0][0].c_str());
        int new_id = 1 + biggest_existing;
        //Insert into order_product
        string insert_new_product = "INSERT INTO orders_product (id, product_id, product_name, wh_id, stock) VALUES ( " + to_string(new_id) + ", " + 
                to_string(insert_new_product_id) + ", '" + to_string(product_name) + "', " +to_string(closest_wh_id) + ", " + to_string(0) + ");";
        dbi->run_query(insert_new_product);
        mtx.unlock();

        string add_stocking_order = "INSERT INTO orders_order(tracking_number, user_id, ups_account, product_id, wh_id, truck_id,status,adr_x,adr_y, amount) VALUES ( " +
                to_string(tracking_number) + ", " + to_string(user_id) + ", '" +
                ups_account + "', " + to_string(insert_new_product_id) + ", " +
                to_string(closest_wh_id) + ", " + to_string(-1) + ", " +
                "'stocking'" + ", " + to_string(address_x) + ", " +
                to_string(address_y) + ", " + to_string(amount) + ");";
        dbi->run_query(add_stocking_order);

        cout << "Send APurchaseMore for new product to world" << endl;     
        ACommands new_buy;
        APurchaseMore *new_apm_buy = new_buy.add_buy();
        new_apm_buy->set_whnum(closest_wh_id);
        AProduct *new_pd_buy = new_apm_buy->add_things();
        new_pd_buy->set_id(insert_new_product_id);
        new_pd_buy->set_description(product_name);
        new_pd_buy->set_count(amount + 500);
        mtx.lock();  //////lock
        new_apm_buy->set_seqnum(world_seqnum);
        pair<long int, ACommands> new_buy_pair(world_seqnum, new_buy);
        world_seqnum++;
        mtx.unlock();  /////unlock
        send_world_queue.pushback(new_buy_pair);
        
    }
}
