
#ifndef __controller_h__
#define __controller_h__

#include "dbregistry.h"

namespace mocainput {

class controller {
public:
    controller(DBRegistry* dbRegistry) : mDbRegistry(dbRegistry) {

    }

protected:
    DBRegistry* mDbRegistry;

private:
    controller();
};

} // namespace mocainput

#endif

