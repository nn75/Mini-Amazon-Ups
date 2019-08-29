#include <iostream>

#include "warehouse.h"

using namespace std;

Warehouse::Warehouse(int id, int x, int y) : id(id), x(x), y(y) {}

int Warehouse::get_id() { return id; }

int Warehouse::get_x() { return x; }

int Warehouse::get_y() { return y; }
