#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <mutex>

using namespace std;
using namespace pqxx;

class database_interface {
   private:
    mutable mutex m;

   public:
    connection *C;

    database_interface() {
        try {
            // Establish a connection to remote database
            // Parameters: database name, user name, user password, host
            // address, port number
            C = new connection(
                "dbname = mini_amazon user = postgres password = passw0rd "
                "hostaddr = 67.159.95.41 port = 5432");
            if (C->is_open()) {
                cout << "Opened database successfully: " << C->dbname() << endl;
            } else {
                cout << "Can't open database" << endl;
            }
        } catch (const std::exception &e) {
            cerr << e.what() << std::endl;
        }
    }

    void run_query(string query) {
        lock_guard<mutex> lock(m);
        work *W;
        W = new work(*C);
        W->exec(query);
        W->commit();
    }

    vector<vector<string> > run_query_with_results(string query) {
        lock_guard<mutex> lock(m);
        work *W;
        W = new work(*C);
        result r = W->exec(query);
        vector<vector<string> > res;
        for (result::const_iterator row = r.begin(); row != r.end(); ++row) {
            vector<string> nrow;
            for (result::tuple::const_iterator field = row->begin();
                 field != row->end(); ++field)
                nrow.push_back(field->c_str());
            res.push_back(nrow);
        }
        W->commit();
        return res;
    }
};
