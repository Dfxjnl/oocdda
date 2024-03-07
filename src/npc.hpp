#ifndef OOCDDA_NPC_HPP
#define OOCDDA_NPC_HPP

#include <string>
#include <vector>

#include <ncurses/curses.h>

#include "faction.hpp"
#include "omdata.hpp"
#include "player.hpp"
#include "skill.hpp"

namespace oocdda {
class Game;
class item;
class monster;
class overmap;
struct point;

#define NPC_LOW_VALUE 5
#define NPC_HI_VALUE 8
#define NPC_VERY_HI_VALUE 15

/*
 * Talk: Trust midlow->high, fear low->mid, need doesn't matter
 * Trade: Trust mid->high, fear low->midlow, need is a bonus
 * Follow: Trust high, fear mid->high, need low->mid
 * Defend: Trust mid->high, fear + need high
 * Kill: Trust low->midlow, fear low->midlow, need low
 * Flee: Trust low, fear mid->high, need low
 */

enum npc_attitude {
    NPCATT_NULL = 0, // Don't care/ignoring player
    NPCATT_TALK, // Move to and talk to player
    NPCATT_TRADE, // Move to and trade with player
    NPCATT_FOLLOW, // Follow the player
    NPCATT_FOLLOW_RUN, // Follow the player, don't shoot monsters
    NPCATT_WAIT, // Waiting for the player
    NPCATT_DEFEND, // Kill monsters that threaten the player
    NPCATT_MUG, // Mug the player
    NPCATT_KILL, // Kill the player
    NPCATT_FLEE, // Get away from the player
    NPCATT_SLAVE, // Following the player under duress
    NPCATT_HEAL, // Get to the player and heal them
    NPCATT_MAX
};

enum npc_mission {
    MISSION_NULL = 0, // Nothing in particular
    MISSION_RESCUE_U, // Find the player and aid them
    MISSION_SHOPKEEP, // Stay still unless combat or something and sell stuff
    NUM_MISSIONS
};

enum npc_class {
    NC_NONE,
    NC_SHOPKEEP, // Found in towns.  Stays in his shop mostly.
    NC_DOCTOR, // Found in towns, or roaming.  Stays in the clinic.
    NC_TRADER, // Roaming trader, journeying between towns.
    NC_NINJA,
    NC_COWBOY,
    NC_SCIENTIST,
    NC_BOUNTY_HUNTER,
    NC_MAX
};

enum npc_action {
    npc_pause = 0,
    npc_reload,
    npc_pickup,
    npc_flee_monster,
    npc_melee_monster,
    npc_shoot_monster,
    npc_alt_attack_monster,
    npc_look_for_player,
    npc_heal_player,
    npc_follow_player,
    npc_talk_to_player,
    npc_flee_player,
    npc_mug_player,
    npc_melee_player,
    npc_shoot_player,
    npc_alt_attack_player,
    npc_goto_destination,
    num_npc_actions
};

enum npc_need { need_none, need_ammo, need_weapon, need_gun, need_food, need_drink, num_needs };

enum npc_flag {
    NF_NULL,
    // Items desired
    NF_FOOD_HOARDER,
    NF_DRUGGIE,
    NF_TECHNOPHILE,
    NF_BOOKWORM,
    NF_MAX
};

struct npc_personality {
    // All values should be in the -10 to 10 range.
    signed char aggression;
    signed char bravery;
    signed char collector;
    signed char altruism;
    npc_personality()
    {
        aggression = 0;
        bravery = 0;
        collector = 0;
        altruism = 0;
    }
};

struct npc_opinion {
    signed char trust;
    signed char fear;
    signed char value;
    npc_opinion()
    {
        trust = 0;
        fear = 0;
        value = 0;
    }
};

class npc : public player {
public:
    npc();
    ~npc();
    virtual bool is_npc() { return true; }
    void randomize(Game* g, npc_class type = NC_NONE);
    void randomize_from_fact(Game* g, faction* fac);
    void make_shopkeep(Game* g, oter_id type);
    void set_name();
    void spawn_at(overmap* o, int posx, int posy);
    skill best_skill();
    void starting_weapon(Game* g);
    bool wear_if_wanted(item it);

    void perform_mission(Game* g);
    int minutes_to_u(Game* g);
    bool fac_has_value(faction_value value);
    bool fac_has_job(faction_job job);

    void form_opinion(player* u);
    void decide_needs();
    void talk_to_u(Game* g);
    void say(Game* g, std::string line);
    void init_selling(std::vector<int>& indices, std::vector<int>& prices);
    void init_buying(std::vector<item> you, std::vector<int>& indices, std::vector<int>& prices);
    int value(item& it);

    bool is_friend();
    bool is_following();
    int danger_assessment(Game* g);
    bool bravery_check(int diff);
    void told_to_help(Game* g);
    void told_to_wait(Game* g);
    void told_to_leave(Game* g);

    // Display
    void draw(WINDOW* w, int plx, int ply, bool inv);
    void print_info(WINDOW* w);
    // Movement and combat
    void shift(int sx, int sy); // Shifts the npc to the appropriate submap
                                // Updates current pos AND our plans

    // The following are defined in npcmove.cpp
    void move(Game* g); // Actual movement; depends on target and attitude
    int confident_range(); // Range at which we have 50% chance of a shot hitting
    bool wont_shoot_friend(Game* g); // Confident that we won't shoot a friend
    monster* choose_monster_target(Game* g); // Most often, the closest to us
    bool want_to_attack_player(Game* g);
    int follow_distance(); // How closely do we follow the player?
    bool can_reload();
    bool fetching_item(); // Are we in the process of fetching a particular item?
    bool saw_player_recently(); // Do we have an idea of where u are?
    bool has_destination(); // Do we have a long-term destination?
    bool alt_attack_available(); // Do we have grenades, molotov, etc?
    npc_action method_of_attacking_player(Game* g, std::vector<point>& path);
    npc_action method_of_attacking_monster(Game* g, std::vector<point>& path);
    npc_action long_term_goal_action(Game* g, std::vector<point>& path);
    void set_destination(Game* g); // Pick a place to go
    void go_to_destination(Game* g);

    bool can_move_to(Game* g, int x, int y);
    void move_to(Game* g, int x, int y);
    void move_away_from(Game* g, int x, int y);
    void move_pause();
    void melee_monster(Game* g, monster* m);
    void alt_attack(Game* g, monster* m, player* p);
    void find_items(Game* g);
    void pickup_items(Game* g);
    void melee_player(Game* g, player& foe);
    void alt_attack_player(Game* g, player& foe);
    void heal_player(Game* g, player& patient);
    void mug_player(Game* g, player& mark);
    void look_for_player(Game* g);
    // The preceding are in npcmove.cpp

    void die(Game* g);

    monster* target; // Current monster we want to kill
    npc_attitude attitude; // What we want to do to the player
    int wandx, wandy, wandf; // Location of heard sound, etc.

    // Location:
    int omx, omy, omz; // Which overmap (e.g., o.0.0.0)
    int mapx, mapy; // Which square in that overmap (e.g., m.0.0)
    int plx, ply, plt; // Where we last saw the player, timeout to forgetting
    int itx, ity; // The square containing an item we want
    int goalx, goaly; // Which mapx:mapy square we want to get to

    // Personality & other defining characteristics
    faction* my_fac;
    npc_mission mission;
    npc_personality personality;
    npc_opinion op_of_u;
    std::vector<npc_need> needs;
    unsigned flags : NF_MAX;
};
} // namespace oocdda

#endif // OOCDDA_NPC_HPP
