#ifndef OOCDDA_MAP_HPP
#define OOCDDA_MAP_HPP

#include <string>
#include <vector>

#include <ncurses/curses.h>

#include "item.hpp"
#include "itype.hpp"
#include "mapdata.hpp"
#include "mapitems.hpp"
#include "monster_type.hpp"
#include "omdata.hpp"
#include "point.hpp"
#include "trap.hpp"

namespace oocdda {
class Game;
class Monster;
class overmap;
class player;

class Map {
public:
    // Constructors & Initialization
    Map();
    Map(std::vector<itype*>* itptr,
        std::vector<itype_id> (*miptr)[num_itloc],
        std::vector<trap*>* trptr);

    void init(Game* g, int x, int y);
    void generate(Game* g, overmap* om, int x, int y, int turn);

    // Visual Output
    void draw(Game* g, WINDOW* w);
    void debug();
    void drawsq(WINDOW* w, player& u, int x, int y, bool invert, bool show_items);

    // File I/O
    void save(overmap* om, unsigned int turn, int x, int y);
    void load(Game* g, int wx, int wy);
    void shift(Game* g, int wx, int wy, int x, int y);
    void spawn_monsters(Game* g);

    // Movement and LOS
    int move_cost(int x, int y); // Cost to move through; 0 = impassible
    bool trans(int x, int y);    // Transparent?
    // (Fx, Fy) sees (Tx, Ty), within a range of (range)?
    // tc indicates the Bresenham line used to connect the two points, and may
    //  subsequently be used to form a path between them
    bool sees(int Fx, int Fy, int Tx, int Ty, int range, int& tc);
    // std::vector<point> route(int Fx, int Fy, int Tx, int Ty); // Best route

    // Terrain
    ter_id& ter(int x, int y);          // Terrain at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
    std::string tername(int x, int y);  // Name of terrain at (x, y)
    std::string features(int x, int y); // Words relevant to terrain (sharp, etc)
    bool has_flag(t_flag flag, int x, int y);
    bool is_destructable(int x, int y);

    bool close_door(int x, int y);
    bool open_door(int x, int y, bool inside);
    bool bash(int x, int y, int str, std::string& sound);
    void destroy(Game* g, int x, int y, bool makesound);
    void shoot(Game* g, int x, int y, int& dam, bool hit_items);

    // Radiation
    int& radiation(int x, int y); // Amount of radiation at (x, y);

    // Items
    std::vector<Item>& i_at(int x, int y);
    Item water_from(int x, int y);
    void i_clear(int x, int y);
    void i_rem(int x, int y, int index);
    Point find_item(Item* it);
    void add_item(int x, int y, itype* type, int birthday);
    void add_item(int x, int y, Item new_item);
    void process_active_items(Game* g);

    // Traps
    trap_id& tr_at(int x, int y);
    void add_trap(int x, int y, trap_id t);
    void disarm_trap(Game* g, int x, int y);

    // Fields
    field& field_at(int x, int y);
    bool add_field(Game* g, int x, int y, field_id t, unsigned char density);
    bool process_fields(Game* g);
    void step_in_field(int x, int y, Game* g);
    void mon_in_field(int x, int y, Monster* z);
    // mapgen.h functions
    void place_items(
        items_location loc, int chance, int x1, int y1, int x2, int y2, bool ongrass, int turn);
    void make_all_items_owned();
    void add_spawn(mon_id type, int count, int x, int y);

private:
    void saven(overmap* om, unsigned int turn, int x, int y, int gridx, int gridy);
    bool loadn(Game* g, int x, int y, int gridx, int gridy);
    void draw_map(oter_id terrain_type,
                  oter_id t_north,
                  oter_id t_east,
                  oter_id t_south,
                  oter_id t_west,
                  oter_id t_above,
                  int turn);
    void rotate(int turns); // Rotates the current map 90*turns degress clockwise
                            // Useful for houses, shops, etc

    submap grid[9];
    std::vector<Item> nulitems; // Returned when &i_at() is asked for an OOB value
    ter_id nulter;              // Returned when &ter() is asked for an OOB value
    trap_id nultrap;            // Returned when &tr_at() is asked for an OOB value
    field nulfield;             // Returned when &field_at() is asked for an OOB value
    int nulrad;                 // OOB &radiation()

    std::vector<itype*>* itypes;
    std::vector<trap*>* traps;
    std::vector<itype_id> (*mapitems)[num_itloc];
};
} // namespace oocdda

#endif // OOCDDA_MAP_HPP
