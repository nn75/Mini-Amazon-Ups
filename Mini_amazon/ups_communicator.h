#ifndef _UPS_COMMUNICATOR_H
#define _UPS_COMMUNICATOR_H

#define UPS_PORT 34567

#include "communicator.h"

class UpsCommunicator : public Communicator {
   protected:
    virtual bool setup_world(long id);

   public:
    UpsCommunicator(unsigned int n, Warehouse *houses);
};

#endif
