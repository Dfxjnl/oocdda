#ifndef OOCDDA_OVERMAP_HPP
#define OOCDDA_OVERMAP_HPP

#include <string>
#include <vector>

#include "mongroup.hpp"
#include "npc.hpp"
#include "omdata.hpp"
#include "point.hpp"
#include "settlement.hpp"

namespace oocdda {
class Game;

struct city {
    int x;
    int y;
    int s;
    city(int X = -1, int Y = -1, int S = -1)
        : x(X)
        , y(Y)
        , s(S)
    {
    }
};

struct om_note {
    int x;
    int y;
    int num;
    std::string text;
    om_note(int X = -1, int Y = -1, int N = -1, std::string T = "")
        : x(X)
        , y(Y)
        , num(N)
        , text(T)
    {
    }
};

struct radio_tower {
    int x;
    int y;
    int strength;
    std::string message;
    radio_tower(int X = -1, int Y = -1, int S = -1, std::string M = "")
        : x(X)
        , y(Y)
        , strength(S)
        , message(M)
    {
    }
};

class overmap {
public:
    overmap();
    overmap(Game* g, int x, int y, int z);
    ~overmap();
    void save();
    void save(int x, int y, int z);
    void open(Game* g, int x, int y, int z);
    void generate(Game* g, overmap* north, overmap* east, overmap* south, overmap* west);
    void generate_sub(overmap* above);
    void make_tutorial();

    Point find_closest(Point origin, oter_id type, int type_range, int& dist, bool must_be_seen);
    void first_house(int& x, int& y);

    oter_id& ter(int x, int y);
    bool& seen(int x, int y);

    bool has_note(int x, int y);
    std::string note(int x, int y);
    void add_note(int x, int y, std::string message);

    std::vector<city> cities;
    std::vector<city> roads_out;
    std::vector<settlement> towns;
    std::vector<mongroup> zg;
    std::vector<radio_tower> radios;
    int posx, posy, posz;
    std::vector<npc> npcs;

private:
    oter_id t[OMAPX][OMAPY];
    oter_id nullret;
    bool s[OMAPX][OMAPY];
    bool nullbool;
    std::vector<om_note> notes;
    // Overall terrain
    void place_river(Point pa, Point pb);
    void place_forest();
    void place_settlements();
    void settlement_building(settlement& set, int x, int y);
    // City Building
    void place_cities(std::vector<city>& cities, int min);
    void put_buildings(int x, int y, int dir, city town);
    void make_road(int cx, int cy, int cs, int dir, city town);
    void build_lab(int x, int y, int s);
    void build_anthill(int x, int y, int s);
    void build_tunnel(int x, int y, int s, int dir);
    void build_slimepit(int x, int y, int s);
    void place_rifts();
    // Connection highways
    void place_hiways(std::vector<city> cities, oter_id base);
    void place_subways(std::vector<Point> stations);
    void make_hiway(int x1, int y1, int x2, int y2, oter_id base);
    void building_on_hiway(int x, int y, int dir);
    // Polishing
    bool is_road(oter_id base, int x, int y); // Dependant on road type
    bool is_road(int x, int y);
    void polish(oter_id min = ot_null, oter_id max = ot_tutorial);
    void good_road(oter_id base, int x, int y);
    void good_river(int x, int y);
    // Monsters, radios, etc.
    void place_mongroups();
    void place_radios();
    void place_NPCs(Game* g);
    // File I/O
};
} // namespace oocdda

#endif // OOCDDA_OVERMAP_HPP
