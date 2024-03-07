#ifndef OOCDDA_PLAYER_HPP
#define OOCDDA_PLAYER_HPP

#include <string>
#include <vector>

#include <ncurses/curses.h>

#include "bionics.hpp"
#include "bodypart.hpp"
#include "color.hpp"
#include "item.hpp"
#include "itype.hpp"
#include "pldata.hpp"
#include "skill.hpp"

namespace oocdda {
class Game;
class monster;
struct trap;

class player {
public:
    player();
    ~player();
    bool create(Game* g, character_type type);
    int random_good_trait(character_type type);
    int random_bad_trait(character_type type);
    void normalize(Game* g); // Starting set up of HP and inventory

    virtual bool is_npc() { return false; }
    nc_color color(); // What color to draw us as

    void load_info(std::string data); // Load from file matching name
    std::string save_info();          // Save to file matching name
    void disp_info(Game* g);          // '@' key; extending character info
    void disp_status(WINDOW* w);      // The constant data in the lower right

    void reset();        // Resets movement points, stats, and applies pain, effects, etc
    int current_speed(); // Returns the number of movement points we get each turn
    int swim_speed();    // Our speed when swimming

    bool has_trait(int flag);
    void toggle_trait(int flag);

    bool has_bionic(bionic_id b);
    bool has_active_bionic(bionic_id b);
    void add_bionic(bionic_id b);
    void charge_power(int amount);
    void power_bionics(Game* g);
    void activate_bionic(int b, Game* g);

    void mutate(Game* g);

    int sight_range(int light_level);
    bool is_in_sunlight(Game* g);
    bool has_two_arms();
    bool can_wear_boots();
    bool is_armed(); // Whether we're wielding something; true for bionics
    bool avoid_trap(trap* tr);

    void pause();                     // '.' command; pauses & reduces recoil
    int hit_mon(Game* g, monster* z); // Handles hitting a monster up to its death
    // hit_player returns false on a miss, and modifies bp, hitdam, and hitcut
    bool hit_player(player& p, body_part& bp, int& hitdam, int& hitcut);
    int dodge();              // Returns the players's dodge, modded by clothing etc
    int throw_range(char ch); // Range of throwing item; -1:DNE 0:Can't throw

    // Converts bphurt to a hp_part (if side == 0, the left), then does/heals dam
    // hit() processes damage through armor
    void hit(Game* g, body_part bphurt, int side, int dam, int cut);
    // absorb() reduces dam and cut by your armor (and bionics, traits, etc)
    void absorb(Game* g, body_part bp, int& dam, int& cut);
    // hurt() doesn't--effects of disease, what have you
    void hurt(Game* g, body_part bphurt, int side, int dam);

    void heal(body_part bpheal, int side, int dam);
    void healall(int dam);
    void hurtall(int dam);

    int hp_percentage(); // % of HP remaining, overall

    void get_sick(Game* g); // Process diseases
    // infect() gives us a chance to save (mostly from armor)
    void infect(dis_type type, body_part vector, int strength, int duration, Game* g);
    // add_disease() does NOT give us a chance to save
    void add_disease(dis_type type, int duration, Game* g);
    void rem_disease(dis_type type);
    bool has_disease(dis_type type);
    int disease_level(dis_type type);

    void add_addiction(add_type type, int strength);
    bool has_addiction(add_type type);
    void rem_addiction(add_type type);
    int addiction_level(add_type type);

    void suffer(Game* g);
    void vomit(Game* g);

    bool eat(Game* g, char let);     // Eat item; returns false on fail
    bool wield(Game* g, char let);   // Wield item; returns false on fail
    bool wear(Game* g, char let);    // Wear item; returns false on fail
    bool takeoff(Game* g, char let); // Take off item; returns false on fail
    void use(Game* g, char let);     // Use a tool
    void read(Game* g, char let);    // Read a book
    void try_to_sleep(Game* g);      // '$' command; adds DIS_LYING_DOWN
    bool can_sleep(Game* g);         // Checked each turn during DIS_LYING_DOWN

    int warmth(body_part bp);                // Warmth provided by armor &c
    int encumb(body_part bp);                // Encumberance from armor &c
    int armor_bash(body_part bp);            // Bashing resistance
    int armor_cut(body_part bp);             // Cutting  resistance
    int resist(body_part bp);                // Infection &c resistance
    bool wearing_something_on(body_part bp); // True if wearing something on bp

    void practice(skill s, int amount); // Practice a skill

    int weight_carried();
    int volume_carried();
    int weight_capacity();
    int volume_capacity();
    int morale_level(); // Modified by traits, &c

    void sort_inv(); // Sort inventory by type
    std::string weapname(bool charges = true);

    void i_add(item it);
    bool has_active_item(itype_id id);
    void process_active_items(Game* g);
    item i_rem(char let);      // Remove item from inventory; returns ret_null on fail
    item i_rem(itype_id type); // Remove first item w/ this type; fail is ret_null
    item remove_weapon();
    item i_remn(int index);                 // Remove item from inventory; returns ret_null on fail
    item& i_at(char let);                   // Returns the item with inventory letter let
    item& i_of_type(itype_id type);         // Returns the first item with this type
    std::vector<item> inv_dump();           // Dumps full item list (e.g. at death)
    void use_up(itype_id it, int quantity); // Uses up items OR charges if a tool
    void use_amount(itype_id it, int quantity);  // Uses up items
    void use_charges(itype_id it, int quantity); // Uses up charges
    int butcher_factor();                        // Automatically picks our best butchering tool

    bool is_wearing(itype_id it); // Are we wearing a specific itype?
    // has_amount works for quantity, OR charges if it is a tool.
    bool has_amount(itype_id it, int quantity);
    // has_number works ONLY for quantity.
    bool has_number(itype_id it, int quantity);
    // has_charges works ONLY for charges.
    bool has_charges(itype_id it, int quantity);
    bool has_item(char let);                // Has an item with invlet let
    bool has_item(item* it);                // Has a specific item
    std::vector<int> has_ammo(ammotype at); // Returns a list of indices of the ammo

    // ---------------VALUES-----------------
    int posx, posy;
    player_activity activity;

    std::string name;
    bool male;
    bool my_traits[PF_MAX2];
    std::vector<bionic> my_bionics;
    // Current--i.e. modified by disease, pain, etc.
    int str_cur, dex_cur, int_cur, per_cur;
    // Normal--i.e. unmodified by disease
    int str_max, dex_max, int_max, per_max;
    int power_level, max_power_level;
    int hunger, thirst, fatigue, health, morale;
    bool underwater;
    bool can_dodge;
    int oxygen;
    unsigned int recoil;
    unsigned int scent;
    int stim, pain, pkill, radiation;
    int cash;
    int moves;
    int hp_cur[num_hp_parts], hp_max[num_hp_parts];

    int sklevel[num_skill_types];
    int skexercise[num_skill_types];

    bool inv_sorted;
    std::vector<item> inv;
    std::vector<item> worn;
    item weapon;
    item ret_null; // Null item, sometimes returns by weapon() etc

    std::vector<disease> illness;
    std::vector<addiction> addictions;
};
} // namespace oocdda

#endif // OOCDDA_PLAYER_HPP
