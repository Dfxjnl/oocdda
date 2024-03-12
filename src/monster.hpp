#ifndef OOCDDA_MONSTER_HPP
#define OOCDDA_MONSTER_HPP

#include <string>
#include <vector>

#include <ncurses/curses.h>

#include "bodypart.hpp"
#include "enums.hpp"
#include "monster_type.hpp"
#include "point.hpp"

namespace oocdda {
class Game;
class Map;
class player;

class Monster {
public:
    Monster();
    Monster(MonsterType* t);
    Monster(MonsterType* t, int x, int y);

    void poly(MonsterType* t);
    void spawn(int x, int y); // All this does is moves the monster to x,y

    // Access
    std::string name();                  // Returns the monster's formal name
    void print_info(Game* g, WINDOW* w); // Prints information to w.
    char symbol();                       // Just our type's symbol; no context
    void draw(WINDOW* w, int plx, int ply, bool inv);
    // Inverts color if inv==true
    bool has_flag(m_flags f); // Returns true if f is set.
    bool made_of(material m); // Returns true if it's made of m
    void load_info(std::string data, std::vector<MonsterType*>* mtypes);
    std::string save_info(); // String of all data, for save files
    void debug(player& u);   // Gives debug info

    // Movement
    void shift(int sx, int sy);             // Shifts the monster to the appropriate submap
                                            // Updates current pos AND our plans
    bool wander();                          // Returns true if we have no plans
    bool can_move_to(Map& m, int x, int y); // Can we move to (x, y)?

    void set_dest(int x, int y, int& t); // Go in a straight line to (x, y)
                                         // t determines WHICH Bresenham line
    void wander_to(int x, int y, int f); // Try to get to (x, y), we don't know
                                         // the route.  Give up after f steps.
    void plan(Game* g);
    void move(Game* g); // Actual movement
    void friendly_move(Game* g);

    Point scent_move(Game* g);
    Point sound_move(Game* g);
    void hit_player(Game* g, player& p);
    void move_to(Game* g, int x, int y);
    void stumble(Game* g, bool moved);

    // Combat
    bool is_fleeing(player& u);            // True if we're fleeing
    int hit(player& p, body_part& bp_hit); // Returns a damage
    void hit_monster(Game* g, int i);
    bool hurt(int dam); // Deals this dam damage; returns true if we dead
    int armor();        // Natural armor, plus any worn armor
    void die(Game* g);

    // Other
    bool make_fungus(Game* g); // Makes this monster into a fungus version
                               // Returns false if no such monster exists
    void make_friendly();
    int posx, posy;
    int wandx, wandy; // Wander destination - Just try to move in that direction
    int wandf;        // Urge to wander - Increased by sound, decrements each move

    // If we were spawned by the map, store our origin for later use
    int spawnmapx, spawnmapy, spawnposx, spawnposy;

    // DEFINING VALUES
    int moves, speed;
    int hp;
    int sp_timeout;
    int friendly;
    MonsterType* type;
    bool dead;

private:
    std::vector<Point> plans;
};
} // namespace oocdda

#endif // OOCDDA_MONSTER_HPP
