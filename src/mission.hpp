#ifndef OOCDDA_MISSION_HPP
#define OOCDDA_MISSION_HPP

#include "itype.hpp"

namespace oocdda {
enum mission_category {
    MISS_NULL = 0,
    MISS_GO_TO, // GOTO considered harmful.
    MISS_FIND_ITEM,
    MISS_FIND_PERSON,
    MISS_ASSASSINATE,
    NUM_MISS_TYPES
};

struct mission {
    mission_category type;
    itype_id item_id;
    int count;
    int npc_id;
};
} // namespace oocdda

#endif // OOCDDA_MISSION_HPP
