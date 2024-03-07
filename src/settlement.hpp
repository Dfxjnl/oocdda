#ifndef OOCDDA_SETTLEMENT_HPP
#define OOCDDA_SETTLEMENT_HPP

#include "faction.hpp"
#include "omdata.hpp"

namespace oocdda {
class game;

struct settlement {
    settlement();
    settlement(int mapx, int mapy);
    void pick_faction(game* g, int omx, int omy);
    void set_population();
    void populate(game* g) {};

    int num(oter_id ter);
    void add_building(oter_id ter);

    faction fact;
    int posx;
    int posy;
    int pop;
    int size;
    int buildings[ot_wall - ot_set_house + 1];
};
} // namespace oocdda

#endif // OOCDDA_SETTLEMENT_HPP
