#ifndef _WORLD_COMMUNICATOR_H
#define _WORLD_COMMUNICATOR_H

#define WORLD_PORT 23456

#include "communicator.h"

class WorldCommunicator : public Communicator {
   protected:
    virtual bool setup_world(long id);

   public:
    WorldCommunicator(unsigned int n, Warehouse *houses);
};

#endif
