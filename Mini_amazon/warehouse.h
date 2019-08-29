#ifndef _WAREHOUSE_H
#define _WAREHOUSE_H

class Warehouse {
   private:
    const int id;
    const int x;
    const int y;

   public:
    Warehouse(int id, int x, int y);
    int get_x();
    int get_y();
    int get_id();
};

#endif
