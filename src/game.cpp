#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "game.hpp"

#include <dirent.h>
#include <ncurses/curses.h>

#include "bionics.hpp"
#include "bodypart.hpp"
#include "color.hpp"
#include "enums.hpp"
#include "file_utils.hpp"
#include "item.hpp"
#include "itype.hpp"
#include "keypress.hpp"
#include "line.hpp"
#include "mapdata.hpp"
#include "monster_type.hpp"
#include "npc.hpp"
#include "omdata.hpp"
#include "output.hpp"
#include "pldata.hpp"
#include "rng.hpp"
#include "skill.hpp"
#include "trap.hpp"
#include "tutorial.hpp"

namespace oocdda {
#define LONG_RANGE 10
#define BLINK_SPEED 300
#define BULLET_SPEED 10000000
#define EXPLOSION_SPEED 70000000

void intro();
nc_color sev(int a); // Right now, ONLY used for scent debugging....

// This is the main game set-up process.
Game::Game()
{
    clear(); // Clear the screen
    intro(); // Print an intro screen, make sure we're at least 80x25

    ensure_save_directory_exists();

    // Gee, it sure is init-y around here!
    init_itypes();   // Set up item types                   (SEE itypedef.cpp)
    init_mtypes();   // Set up monster types                (SEE mtypedef.cpp)
    init_monitems(); // Set up which items monsters carry   (SEE monitemsdef.cpp)
    init_traps();    // Set up the trap types               (SEE trapdef.cpp)
    init_mapitems(); // Set up which items appear where     (SEE mapitemsdef.cpp)
    init_recipes();  // Set up crafting reciptes            (SEE crafting.cpp)
    init_moncats();  // Set up monster categories           (SEE mongroupdef.cpp)

    m = Map(&itypes, &mapitems, &traps); // Init the root map with our vectors

    // Set up the main UI windows.
    // Aw hell, we getting ncursey up in here!
    w_terrain = newwin(SEEY * 2 + 1, SEEX * 2 + 1, 0, 0);
    werase(w_terrain);
    w_minimap = newwin(7, 7, 0, SEEX * 2 + 1);
    werase(w_minimap);
    w_HP = newwin(14, 7, 7, SEEX * 2 + 1);
    werase(w_HP);
    w_moninfo = newwin(12, 48, 0, SEEX * 2 + 8);
    werase(w_moninfo);
    w_messages = newwin(9, 48, 12, SEEX * 2 + 8);
    werase(w_messages);
    w_status = newwin(4, 55, 21, SEEX * 2 + 1);
    werase(w_status);
    // Even though we may already have 'd', nextinv will be incremented as needed
    nextinv = 'd';
    last_target = -1;    // We haven't targeted any monsters yet
    curmes = 0;          // We haven't read any messages yet
    uquit = false;       // We haven't quit the game
    debugmon = false;    // We're not debugging monster behavior
    in_tutorial = false; // We're not in a tutorial game

    // Reset kill counts to 0.
    for (int& kill : kills) {
        kill = 0;
    }

    // Set the scent map to 0.
    for (auto& rows : grscent) {
        for (int& scent : rows) {
            scent = 0;
        }
    }

    if (opening_screen()) { // Opening menu
        // Finally, draw the screen!
        refresh_all();
        draw();
    }
}

Game::~Game()
{
    for (const auto& itype : itypes) {
        delete itype;
    }

    for (const auto& mtype : mtypes) {
        delete mtype;
    }

    for (const auto& trap : traps) {
        delete trap;
    }

    delwin(w_terrain);
    delwin(w_minimap);
    delwin(w_HP);
    delwin(w_moninfo);
    delwin(w_messages);
    delwin(w_status);
}

bool Game::opening_screen()
{
    WINDOW* w_open = newwin(25, 80, 0, 0);
    erase();
    for (int i = 0; i < 80; i++)
        mvwputch(w_open, 21, i, c_white, LINE_OXOX);
    mvwprintz(w_open, 0, 1, c_blue, "Welcome to Cataclysm!");
    mvwprintz(w_open, 1, 0, c_red, "\
 This alpha release is highly unstable. Please report any crashes or bugs to\n\
 fivedozenwhales@gmail.com.");
    refresh();
    wrefresh(w_open);
    refresh();
    std::vector<std::string> savegames;
    std::string tmp;
    dirent* dp;
    DIR* dir = opendir("save");

    while ((dp = readdir(dir)) != nullptr) {
        tmp = dp->d_name;

        if (tmp.find(".sav") != std::string::npos && savegames.size() < 18) {
            savegames.push_back(tmp.substr(0, tmp.find(".sav")));
        }
    }

    wrefresh(w_open);
    int sel1 = 1, sel2 = 1, layer = 1;
    char ch;
    bool start = false;
    do {
        if (layer == 1) {
            mvwprintz(w_open, 4, 1, (sel1 == 1 ? h_white : c_white), "New Game");
            mvwprintz(w_open, 5, 1, (sel1 == 2 ? h_white : c_white), "Load Game");
            mvwprintz(w_open, 6, 1, (sel1 == 3 ? h_white : c_white), "Tutorial");
            mvwprintz(w_open, 7, 1, (sel1 == 4 ? h_white : c_white), "Help");
            mvwprintz(w_open, 8, 1, (sel1 == 5 ? h_white : c_white), "Quit");
            wrefresh(w_open);
            refresh();
            ch = input();
            if (ch == 'k' && sel1 > 1)
                sel1--;
            if (ch == 'j' && sel1 < 5)
                sel1++;
            if (ch == 'l' || ch == '\n' || ch == '>') {
                if (sel1 == 5) {
                    uquit = true;
                    return false;
                }
                if (sel1 == 4) {
                    help();
                    clear();
                    mvwprintz(w_open, 0, 1, c_blue, "Welcome to Cataclysm!");
                    mvwprintz(w_open, 1, 0, c_red, "\
 This alpha release is highly unstable. Please report any crashes or bugs to\n\
 fivedozenwhales@gmail.com.");
                    wrefresh(w_open);
                    refresh();
                } else if (sel1 == 3) {
                    u.normalize(this);
                    start_tutorial(TUT_BASIC);
                    return true;
                } else {
                    sel2 = 1;
                    layer = 2;
                }
                mvwprintz(w_open, 4, 1, (sel1 == 1 ? c_white : c_dkgray), "New Game");
                mvwprintz(w_open, 5, 1, (sel1 == 2 ? c_white : c_dkgray), "Load Game");
                mvwprintz(w_open, 6, 1, (sel1 == 3 ? c_white : c_dkgray), "Tutorial");
                mvwprintz(w_open, 7, 1, (sel1 == 3 ? c_white : c_dkgray), "Help");
                mvwprintz(w_open, 8, 1, (sel1 == 4 ? c_white : c_dkgray), "Quit");
            }
        } else if (layer == 2) {
            if (sel1 == 1) { // New Character
                mvwprintz(w_open, 4, 12, (sel2 == 1 ? h_white : c_white), "Custom Character");
                mvwprintz(w_open, 5, 12, (sel2 == 2 ? h_white : c_white), "Preset Character");
                mvwprintz(w_open, 6, 12, (sel2 == 3 ? h_white : c_white), "Random Character");
                wrefresh(w_open);
                refresh();
                ch = input();
                if (ch == 'k' && sel2 > 1)
                    sel2--;
                if (ch == 'j' && sel2 < 3)
                    sel2++;
                if (ch == 'h' || ch == '<' || ch == KEY_ESCAPE) {
                    mvwprintz(w_open, 4, 12, c_black, "                ");
                    mvwprintz(w_open, 5, 12, c_black, "                ");
                    mvwprintz(w_open, 6, 12, c_black, "                ");
                    layer = 1;
                    sel1 = 1;
                }
                if (ch == 'l' || ch == '\n' || ch == '>') {
                    if (sel2 == 1) {
                        if (!u.create(this, PLTYPE_CUSTOM)) {
                            delwin(w_open);
                            return (opening_screen());
                        }
                        start_game();
                        start = true;
                        ch = 0;
                    }
                    if (sel2 == 2) {
                        layer = 3;
                        sel1 = 2;
                        mvwprintz(w_open, 4, 12, c_dkgray, "Custom Character");
                        mvwprintz(w_open, 5, 12, c_white, "Preset Character");
                        mvwprintz(w_open, 6, 12, c_dkgray, "Random Character");
                    }
                    if (sel2 == 3) {
                        if (!u.create(this, PLTYPE_RANDOM)) {
                            delwin(w_open);
                            return (opening_screen());
                        }
                        start_game();
                        start = true;
                        ch = 0;
                    }
                }
            } else if (sel1 == 2) { // Load Character
                if (savegames.size() == 0)
                    mvwprintz(w_open, 5, 12, c_red, "No save games found!");
                else {
                    for (int i {0}; i < static_cast<int>(savegames.size()); ++i) {
                        mvwprintz(w_open, 5 + i, 12,
                                  (sel2 - 1 == i ? nc_color::h_white : nc_color::c_white),
                                  savegames[i].c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                ch = input();
                if (ch == 'k' && sel2 > 1)
                    sel2--;

                if (ch == 'j' && sel2 < static_cast<int>(savegames.size())) {
                    ++sel2;
                }

                if (ch == 'h' || ch == '<' || ch == KEY_ESCAPE) {
                    layer = 1;

                    for (int i {0}; i < static_cast<int>(savegames.size()) + 1; ++i) {
                        mvwprintz(w_open, 5 + i, 12, nc_color::c_black,
                                  "                                ");
                    }
                }
                if (ch == 'l' || ch == '\n' || ch == '>') {
                    if (sel2 > 0 && savegames.size() > 0) {
                        load(savegames[sel2 - 1]);
                        start = true;
                        ch = 0;
                    }
                }
            }
        } else if (layer == 3) { // Character presets
            for (int i = 2; i < PLTYPE_MAX; i++)
                mvwprintz(w_open, 3 + i, 29, (sel1 == i ? h_white : c_white),
                          pltype_name[i].c_str());
            for (int i = 22; i < 25; i++)
                mvwprintw(w_open, i, 0, "                                                  \
                              ");
            mvwprintz(w_open, 22, 0, c_magenta, pltype_desc[sel1].c_str());
            wrefresh(w_open);
            refresh();
            ch = input();
            if (ch == 'k' && sel1 > 2)
                sel1--;
            if (ch == 'j' && sel1 < PLTYPE_MAX - 1)
                sel1++;
            if (ch == 'h' || ch == '<' || ch == KEY_ESCAPE) {
                sel1 = 1;
                layer = 2;
                for (int i = 2; i < PLTYPE_MAX; i++)
                    mvwprintz(w_open, 3 + i, 12, c_black, "                                 ");
                for (int i = 22; i < 25; i++)
                    mvwprintw(w_open, i, 0, "                                                 \
                                ");
            }
            if (ch == 'l' || ch == '\n' || ch == '>') {
                if (!u.create(this, character_type(sel1))) {
                    delwin(w_open);
                    return (opening_screen());
                }
                start_game();
                start = true;
                ch = 0;
            }
        }
    } while (ch != 0);
    delwin(w_open);
    if (start == false)
        uquit = true;
    return start;
}

// Set up all default values for a new game
void Game::start_game()
{
    turn = 0;     // It's turn 0...
    run_mode = 1; // run_mode is on by default...
    mostseen = 0; // ...and mostseen is 0, we haven't seen any monsters yet.

    // Init some factions.
    init_factions();
    cur_om = overmap(this, 0, 0, 0); // We start in the (0,0,0) overmap.
    // Find a random house on the map, and set us there.
    cur_om.first_house(levx, levy);
    levz = 0;
    // Start the overmap out with none of it seen by the player...
    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPX; j++)
            cur_om.seen(i, j) = false;
    }
    // ...except for our immediate neighborhood.
    for (int i = -20; i <= 20; i++) {
        for (int j = -20; j <= 20; j++)
            cur_om.seen(levx + i, levy + j) = true;
    }
    // Convert the overmap coordinates to submap coordinates
    levx = levx * 2 - 1;
    levy = levy * 2 - 1;
    // Init the starting map at this location.
    m.init(this, levx, levy);
    // Start us off somewhere in the house.
    u.posx = SEEX + 4;
    u.posy = SEEY + 5;
    nextspawn = 300; // No monsters until 8:30 AM!
    temp = 65;       // Kind of cool for June, but okay.

    // Testing pet dog!
    Monster doggy(mtypes[mon_dog], u.posx - 1, u.posy - 1);
    doggy.friendly = -1;
    z.push_back(doggy);
}

void Game::start_tutorial(tut_type type)
{
    for (bool& seen : tutorials_seen) {
        seen = false;
    }

    // Set the scent map to 0.
    for (auto& row : grscent) {
        for (int& element : row) {
            element = 0;
        }
    }

    temp = 65; // Kind of cool for June, but okay.
    in_tutorial = true;
    switch (type) {
    case TUT_NULL:
        debugmsg("Null tutorial requested.");
        return;
    case TUT_BASIC:
        // We use a Z-factor of 10 so that we don't plop down tutorial rooms in the
        // middle of the "real" game world
        u.name = "John Smith";
        levx = 100;
        levy = 99;
        cur_om = overmap(this, 0, 0, TUTORIAL_Z - 1);
        cur_om.make_tutorial();
        cur_om.save(0, 0, 9);
        cur_om = overmap(this, 0, 0, TUTORIAL_Z);
        cur_om.make_tutorial();
        m.init(this, levx, levy);
        u.toggle_trait(PF_QUICK);
        u.sklevel[sk_gun] = 5;
        u.sklevel[sk_melee] = 5;
        // Start the overmap out with all of it seen by the player
        for (int i = 0; i < OMAPX; i++) {
            for (int j = 0; j < OMAPX; j++)
                cur_om.seen(i, j) = true;
        }
        // Init the starting map at this location.
        m.init(this, levx, levy);
        // Make sure the map is totally reset
        for (int i = 0; i < SEEX * 3; i++) {
            for (int j = 0; j < SEEY * 3; j++)
                m.i_at(i, j).clear();
        }
        // Special items for the tutorial.
        m.add_item(5, SEEY * 2 + 1, itypes[itm_helmet_bike], 0);
        m.add_item(4, SEEY * 2 + 1, itypes[itm_backpack], 0);
        m.add_item(3, SEEY * 2 + 1, itypes[itm_pants_cargo], 0);
        m.add_item(7, SEEY * 3 - 4, itypes[itm_machete], 0);
        m.add_item(7, SEEY * 3 - 4, itypes[itm_9mm], 0);
        m.add_item(7, SEEY * 3 - 4, itypes[itm_9mmP], 0);
        m.add_item(7, SEEY * 3 - 4, itypes[itm_uzi], 0);
        m.add_item(SEEX * 2 - 2, SEEY * 2 + 5, itypes[itm_bubblewrap], 0);
        m.add_item(SEEX * 2 - 2, SEEY * 2 + 6, itypes[itm_grenade], 0);
        m.add_item(SEEX * 2 - 3, SEEY * 2 + 6, itypes[itm_flashlight], 0);
        m.add_item(SEEX * 2 - 2, SEEY * 2 + 7, itypes[itm_cig], 0);
        m.add_item(SEEX * 2 - 2, SEEY * 2 + 7, itypes[itm_codeine], 0);
        m.add_item(SEEX * 2 - 3, SEEY * 2 + 7, itypes[itm_water], 0);

        levz = 0;
        u.posx = SEEX + 2;
        u.posy = SEEY + 4;
        break;
    default:
        debugmsg("Haven't made that tutorial yet.");
        return;
    }
}

void Game::init_factions()
{
    int num = dice(2, 6);
    faction tmp;
    tmp.make_army();
    factions.push_back(tmp);
    for (int i = 0; i < num; i++) {
        tmp.randomize();
        tmp.id = i + 1;
        tmp.likes_u = 100;
        tmp.respects_u = 100;
        tmp.known_by_u = true;
        factions.push_back(tmp);
    }
}

// MAIN GAME LOOP
// Returns true if game is over (death, saved, quit, etc)
bool Game::do_turn()
{
    if (is_game_over())
        return true;
    turn++;
    process_events();
    if (in_tutorial) {
        if (turn == 1) {
            tutorial_message(LESSON_INTRO); // Goes through a list of intro topics
            tutorial_message(LESSON_INTRO);
        } else if (turn == 3)
            tutorial_message(LESSON_INTRO);
        if (turn == 50) {
            Monster tmp(mtypes[mon_zombie], 3, 3);
            z.push_back(tmp);
        }
    }
    // Check if we've overdosed... in any deadly way.
    if (u.stim > 150) {
        add_msg("You have a sudden heart attack!");
        u.hp_cur[hp_torso] = 0;
    } else if (u.stim < -100 || u.pkill > 120) {
        add_msg("Your breathing stops completely.");
        u.hp_cur[hp_torso] = 0;
    }
    if (turn % 50 == 0) { // Hunger, thirst, & fatigue up every 5 minutes
        if (!u.has_trait(PF_LIGHTEATER) || !one_in(3))
            u.hunger++; // If light eater, around 1/3 of the time it's skipped
        if (u.has_bionic(bio_recycler) && turn % 300 == 0)
            u.hunger--;
        if (!u.has_bionic(bio_recycler) || turn % 100 == 0)
            u.thirst++; // If has a recycler, 1/2 of the time it's skipped
        u.fatigue++;
        if (u.fatigue == 192 && !u.has_disease(DI_LYING_DOWN) && !u.has_disease(DI_SLEEP))
            add_msg("You're feeling tired.  Press '$' to lie down for sleep.");
        if (u.stim < 0)
            u.stim++;
        if (u.stim > 0)
            u.stim--;
        if (u.pkill > 0)
            u.pkill--;
        if (u.pkill < 0)
            u.pkill++;
        if (u.has_bionic(bio_solar) && u.is_in_sunlight(this))
            u.charge_power(1);
    }
    if (turn % 300 == 0) { // Pain and morale up/down every 30 minutes
        if (one_in(10)) {  // Morale shifts are rare
            if (u.morale < 0 && one_in(3))
                u.morale++;
            else if (u.morale > 0)
                u.morale--;
            if (one_in(2))
                u.morale--;
        }
        if (u.pain > 0)
            u.pain--;
        else if (u.pain < 0)
            u.pain++;
        if (u.has_trait(PF_REGEN) && !one_in(3))
            u.healall(1);
        if (u.has_trait(PF_ROT) && !one_in(3))
            u.hurtall(1);
        if (u.radiation > 1 && one_in(3))
            u.radiation--;
        u.get_sick(this);
    }

    // The following happens when we stay still; 40/240 minutes overdue for spawn.
    if ((!u.has_trait(pl_flag::PF_INCONSPICUOUS) && static_cast<int>(turn) > nextspawn + 400)
        || (u.has_trait(pl_flag::PF_INCONSPICUOUS) && static_cast<int>(turn) > nextspawn + 2400)) {
        spawn_mon(-1 + 2 * static_cast<int>(rng(0, 1)), -1 + 2 * static_cast<int>(rng(0, 1)));
        nextspawn = static_cast<int>(turn);
    }

    m.process_fields(this);
    m.process_active_items(this);
    m.step_in_field(u.posx, u.posy, this);
    monmove();
    om_npcs_move();
    u.reset();
    u.process_active_items(this);
    u.suffer(this);
    check_warmth();
    update_skills();
    if (u.has_disease(DI_SLEEP)) {
        draw();
        refresh();
    }
    process_activity();
    if (is_game_over())
        return true;
    while (u.moves > 0) {
        draw();
        get_input();
        if (is_game_over())
            return true;
    }
    update_scent();
    return false;
}

void Game::update_skills()
{
    //    SKILL   TURNS/--
    //	1	2048
    //	2	1024
    //	3	 512
    //	4	 256
    //	5	 128
    //	6	  64
    //	7+	  32
    int tmp;
    for (int i = 0; i < num_skill_types; i++) {
        tmp = u.sklevel[i] > 7 ? 7 : u.sklevel[i];
        if (u.sklevel[i] > 0 && turn % (4096 / int(pow(2, tmp - 1))) == 0
            && ((u.has_trait(PF_FORGETFUL) && one_in(3))
                || (!u.has_trait(PF_FORGETFUL) && one_in(4)))) {
            if (u.has_bionic(bio_memory) && u.power_level > 0) {
                if (one_in(5))
                    u.power_level--;
            } else
                u.skexercise[i]--;
        }
        if (u.skexercise[i] < -100) {
            u.sklevel[i]--;
            add_msg("Your skill in %s has reduced to %d!", skill_name(skill(i)).c_str(),
                    u.sklevel[i]);
            u.skexercise[i] = 0;
        } else if (u.skexercise[i] >= 100) {
            u.sklevel[i]++;
            add_msg("Your skill in %s has increased to %d!", skill_name(skill(i)).c_str(),
                    u.sklevel[i]);
            u.skexercise[i] = 0;
        }
    }
}

void Game::process_events()
{
    for (std::size_t i {0}; i < events.size(); ++i) {
        if (events[i].turn <= static_cast<int>(turn)) {
            events[i].actualize(this);
            events.erase(events.begin() + static_cast<std::ptrdiff_t>(i));
            --i;
        }
    }
}

void Game::process_activity()
{
    it_gun* reloading {nullptr};
    it_book* reading {nullptr};

    if (u.activity.type != ACT_NULL) {
        draw();
        if (u.activity.type == ACT_WAIT) { // Based on time, not speed
            u.activity.moves_left -= 100;
            u.moves = 0;
        } else {
            u.activity.moves_left -= u.moves;
            u.moves = 0;
        }
        if (u.activity.moves_left <= 0) { // We finished our activity!
            switch (u.activity.type) {
            case ACT_RELOAD:
                u.weapon.reload(u, u.activity.index);
                if (u.weapon.is_gun())
                    reloading = dynamic_cast<it_gun*>(u.weapon.type);
                if (u.weapon.is_gun() && one_in(u.sklevel[reloading->skill_used]))
                    u.practice(reloading->skill_used, rng(2, 6));
                if (u.weapon.is_gun() && reloading->skill_used == sk_shotgun) {
                    add_msg("You insert a cartridge into your %s.", u.weapon.tname().c_str());
                    if (u.recoil < 8)
                        u.recoil = 8;
                    if (u.recoil > 8)
                        u.recoil = (8 + u.recoil) / 2;
                } else {
                    add_msg("You reload your %s.", u.weapon.tname().c_str());
                    u.recoil = 6;
                    if (in_tutorial) {
                        tutorial_message(LESSON_GUN_FIRE);
                        Monster tmp(mtypes[mon_zombie], u.posx, u.posy - 6);
                        z.push_back(tmp);
                        tmp.spawn(u.posx + 2, u.posy - 5);
                        z.push_back(tmp);
                        tmp.spawn(u.posx - 2, u.posy - 5);
                        z.push_back(tmp);
                    }
                }
                break;
            case ACT_READ:
                if (u.activity.index == -2)
                    reading = dynamic_cast<it_book*>(u.weapon.type);
                else
                    reading = dynamic_cast<it_book*>(u.inv[u.activity.index].type);
                if (u.morale < reading->fun * 10 || reading->fun < 0)
                    u.morale += reading->fun;
                if (u.sklevel[reading->type] < reading->level) {
                    add_msg("You learn a little about %s!", skill_name(reading->type).c_str());
                    u.skexercise[reading->type] += rng(reading->time, reading->time * 2);
                }
                break;
            case ACT_WAIT:
                add_msg("You finish waiting.");
                break;
            case ACT_CRAFT:
                complete_craft();
                break;

            default:
                break;
            }
            u.activity.type = ACT_NULL;
        }
    }
}

void Game::cancel_activity() { u.activity.type = ACT_NULL; }

void Game::cancel_activity_query(std::string message)
{
    switch (u.activity.type) {
    case ACT_READ:
        if (query_yn("%s Stop reading?", message.c_str()))
            u.activity.type = ACT_NULL;
        break;
    case ACT_RELOAD:
        if (query_yn("%s Stop reloading?", message.c_str()))
            u.activity.type = ACT_NULL;
        break;
    case ACT_CRAFT:
        if (query_yn("%s Stop crafting?", message.c_str()))
            u.activity.type = ACT_NULL;
        break;
    default:
        u.activity.type = ACT_NULL;
    }
}

void Game::get_input()
{
    char ch = input(); // See keypress.h

    // These are the default characters for all actions.  It's the job of input(),
    // found in keypress.h, to translate the user's input into these characters.
    if (ch == 'y' || ch == 'u' || ch == 'h' || ch == 'j' || ch == 'k' || ch == 'l' || ch == 'b'
        || ch == 'n') {
        int movex, movey;
        get_direction(movex, movey, ch);
        plmove(movex, movey);
    } else if (ch == '>')
        vertical_move(-1, false);
    else if (ch == '<')
        vertical_move(1, false);
    else if (ch == '.') {
        if (run_mode == 2) // Monsters around and we don't wanna pause
            add_msg("Monster spotted--run mode is on! (Press '!' to turn it off.)");
        else
            u.pause();
    } else if (ch == 'o')
        open();
    else if (ch == 'c')
        close();
    else if (ch == 'p') {
        u.power_bionics(this);
        refresh_all();
    } else if (ch == 'e')
        examine();
    else if (ch == ';')
        look_around();
    else if (ch == ',' || ch == 'g')
        pickup(u.posx, u.posy, 1);
    else if (ch == 'd')
        drop();
    else if (ch == 'i') {
        char ch = inv();
        if (u.has_item(ch)) {
            full_screen_popup(u.i_at(ch).info(true).c_str());
            refresh_all();
        }
    } else if (ch == 'B')
        butcher();
    else if (ch == 'E')
        eat();
    else if (ch == 'a')
        use_item();
    else if (ch == 'W')
        wear();
    else if (ch == 'w')
        wield();
    else if (ch == '^')
        wait();
    else if (ch == 'T')
        takeoff();
    else if (ch == 'r')
        reload();
    else if (ch == 'U')
        unload();
    else if (ch == 'R')
        read();
    else if (ch == 't')
        plthrow();
    else if (ch == 'f')
        plfire(false);
    else if (ch == 'F')
        plfire(true);
    else if (ch == 'C')
        chat();
    // <DEBUG>
    else if (ch == 'z') {
        debugmsg("%d radio towers", cur_om.radios.size());
        for (int i = 0; i < OMAPX; i++) {
            for (int j = 0; j < OMAPY; j++)
                cur_om.seen(i, j) = true;
        }
    } else if (ch == 'M') {
        erase();
        mvprintw(0, 0, "%zu addictions", u.addictions.size());

        for (std::size_t i {0}; i < u.addictions.size(); ++i) {
            mvprintw(1 + static_cast<int>(i), 0, "%d: int %d sate %d",
                     static_cast<int>(u.addictions[i].type), u.addictions[i].intensity,
                     u.addictions[i].sated);
        }

        mvprintw(20, 0, "Turn %d; nextspawn %d", turn, nextspawn);
        getch();
    } else if (ch == 'Z')
        wish();
    else if (ch == 'G') {
        npc temp;
        temp.randomize(this);
        temp.attitude = NPCATT_TALK;
        temp.spawn_at(&cur_om, levx + (1 * rng(-2, 2)), levy + (1 * rng(-2, 2)));
        temp.posx = u.posx - 4;
        temp.posy = u.posy - 4;
        active_npc.push_back(temp);
    } else if (ch == 'g')
        groupdebug();
    else if (ch == 'q')
        debugmon = !debugmon;
    else if (ch == '\'') {
        display_scent();
        getch();
    } else if (ch == '*')
        teleport();
    else if (ch == '%')
        disp_kills();
    // </DEBUG>
    else if (ch == ':' || ch == 'm')
        draw_overmap();
    else if (ch == '@') {
        u.disp_info(this);
        refresh_all();
    } else if (ch == '#')
        list_factions();
    else if (ch == '&')
        craft();
    else if (ch == '$' && query_yn("Are you sure you want to sleep?")) {
        u.try_to_sleep(this);
        u.moves = 0;
    } else if (ch == '!') {
        if (run_mode == 0) {
            run_mode = 1;
            add_msg("Run mode ON!");
        } else {
            run_mode = 0;
            add_msg("Run mode OFF!");
        }
    } else if (ch == 's')
        smash();
    else if (ch == 'S' && query_yn("Save and quit?")) {
        save();
        u.moves = 0;
        uquit = true;
    } else if (ch == 'Q' && query_yn("Commit suicide?")) {
        u.moves = 0;
        std::vector<Item> tmp = u.inv_dump();
        Item your_body;
        your_body.make_corpse(itypes[itm_corpse], mtypes[mon_null], turn);
        your_body.name = u.name;
        m.add_item(u.posx, u.posy, your_body);

        for (const auto& item : tmp) {
            m.add_item(u.posx, u.posy, item);
        }

        m.save(&cur_om, turn, levx, levy);
        uquit = true;
    } else if (ch == '?') {
        help();
        refresh_all();
    }
}

int& Game::scent(int x, int y)
{
    if (x < 0 || x >= SEEX * 3 || y < 0 || y >= SEEY * 3) {
        nulscent = 0;
        return nulscent; // Out-of-bounds - null scent
    }
    return grscent[x][y];
}

void Game::update_scent()
{
    signed int newscent[SEEX * 3][SEEY * 3];
    int x, y, i, j, squares_used;
    if (!u.has_active_bionic(bio_scent_mask))
        scent(u.posx, u.posy) = u.scent;
    else
        scent(u.posx, u.posy) = 0;
    for (x = 0; x < SEEX * 3; x++) {
        for (y = 0; y < SEEY * 3; y++) {
            newscent[x][y] = 0;
            squares_used = 0;
            for (i = -1; i <= 1; i++) {
                for (j = -1; j <= 1; j++) {
                    if ((m.move_cost(x, y) != 0 || m.has_flag(bashable, x, y))
                        && scent(x, y) <= scent(x + i, y + j)) {
                        newscent[x][y] += scent(x + i, y + j);
                        squares_used++;
                    }
                }
            }
            newscent[x][y] /= (squares_used + 1);
            if (m.field_at(x, y).type == fd_slime && newscent[x][y] < 10 * m.field_at(x, y).density)
                newscent[x][y] = 10 * m.field_at(x, y).density;
            if (newscent[x][y] > 10000) {
                debugmsg("Wacky scent at %d, %d (%d)", x, y, newscent[x][y]);
                newscent[x][y] = 0; // Scent should never be higher
            }
        }
    }
    for (x = 0; x < SEEX * 3; x++) {
        for (y = 0; y < SEEY * 3; y++)
            scent(x, y) = newscent[x][y];
    }
    if (!u.has_active_bionic(bio_scent_mask))
        scent(u.posx, u.posy) = u.scent;
    else
        scent(u.posx, u.posy) = 0;
}

bool Game::is_game_over()
{
    if (uquit)
        return true;
    for (int i = 0; i <= hp_torso; i++) {
        if (u.hp_cur[i] < 1) {
            popup("You would've died just now.\n\
But since it's an alpha test, KEEP PLAYING! (hp %d <0)\n\
Press spacebar to be healed and relieved of pain...",
                  i);
            u.moves = 120;
            u.pain = 0;
            u.hp_cur[i] = u.hp_max[i];
        }
    }
    return false;
}

void Game::load(std::string name)
{
    std::ifstream fin;
    std::stringstream fn;
    fn << "save/" << name << ".sav";
    std::string filename = fn.str();
    fin.open(filename.c_str());
    // First, read in basic game state information.
    if (!fin.is_open()) {
        debugmsg("No save game exists!");
        return;
    }
    u = player();
    u.name = name;
    u.ret_null = Item(itypes[0], 0);
    u.weapon = Item(itypes[0], 0);
    int tmprun, tmptar, tmptemp, comx, comy;
    fin >> turn >> tmptar >> tmprun >> mostseen >> nextinv >> nextspawn >> tmptemp >> levx >> levy
        >> levz >> comx >> comy;
    cur_om = overmap(this, comx, comy, 0);
    // m = map(&itypes, &mapitems, &traps); // Init the root map with our vectors
    m.init(this, levx, levy);
    run_mode = tmprun;
    last_target = tmptar;
    temp = tmptemp;

    // Next, the scent map.
    for (auto& row : grscent) {
        for (int& element : row) {
            fin >> element;
        }
    }

    // Now the number of monsters...
    int nummon;
    fin >> nummon;
    // ... and the data on each one.
    std::string data;
    z.clear();
    Monster montmp;
    char junk;
    if (fin.peek() == '\n')
        fin.get(junk); // Chomp that pesky endline
    for (int i = 0; i < nummon; i++) {
        getline(fin, data);
        montmp.load_info(data, &mtypes);
        z.push_back(montmp);
    }
    // Finally, the data on the player.
    if (fin.peek() == '\n')
        fin.get(junk); // Chomp that pesky endline
    getline(fin, data);
    u.load_info(data);
    // And the player's inventory...
    char item_place;
    std::string itemdata;
    while (!fin.eof()) {
        fin >> item_place;
        if (fin.eof())
            return;
        getline(fin, itemdata);
        if (item_place == 'I')
            u.inv.push_back(Item(itemdata, this));
        else if (item_place == 'C')
            u.inv[u.inv.size() - 1].contents.push_back(Item(itemdata, this));
        else if (item_place == 'W')
            u.worn.push_back(Item(itemdata, this));
        else if (item_place == 'w')
            u.weapon = Item(itemdata, this);
        else if (item_place == 'c')
            u.weapon.contents.push_back(Item(itemdata, this));
    }

    // And the kill counts.
    for (int& kill : kills) {
        fin >> kill;
    }

    fin.close();
    draw();
}

void Game::save()
{
    char buff[128];
    std::ofstream fout;
    sprintf(buff, "save/%s.sav", u.name.c_str());
    fout.open(buff);
    // First, write out basic game state information.
    fout << turn << " " << int(last_target) << " " << int(run_mode) << " " << mostseen << " "
         << nextinv << " " << nextspawn << " " << int(temp) << " " << levx << " " << levy << " "
         << levz << " " << cur_om.posx << " " << cur_om.posy << " " << std::endl;

    // Next, the scent map.
    for (const auto& row : grscent) {
        for (const int element : row) {
            fout << element << ' ';
        }
    }

    // Now save all monsters.
    fout << std::endl << z.size() << std::endl;

    for (auto& monster : z) {
        fout << monster.save_info() << '\n';
    }

    // And finally the player.
    fout << u.save_info() << std::endl;

    // Save the kill counts, too.
    for (const int kill : kills) {
        fout << kill << ' ';
    }

    fout << std::endl;
    fout.close();
    // aaaand the overmap, and the local map.
    cur_om.save();
    m.save(&cur_om, turn, levx, levy);
}

void Game::advance_nextinv()
{
    nextinv++;
    if (nextinv == '{')
        nextinv = 'A';
    if (nextinv == '[')
        nextinv = 'a';
}

void Game::add_msg(const char* msg, ...)
{
    char buff[512];
    va_list ap;
    va_start(ap, msg);
    vsprintf(buff, msg, ap);
    va_end(ap);

    int maxlength = 80 - (SEEX * 2 + 10); // Matches write_msg() below
    if (messages.size() == 256)
        messages.erase(messages.begin());
    curmes++;
    std::string s(buff);
    size_t split;

    while (static_cast<int>(s.length()) > maxlength) {
        split = s.find_last_of(' ', maxlength);
        messages.push_back(s.substr(0, split));
        curmes++;
        s = s.substr(split);
    }

    messages.push_back(s);
}

void Game::add_event(event_type type, int on_turn, faction* rel)
{
    debugmsg("type %d, on turn %d (%d)", type, on_turn, turn);
    event tmp(type, on_turn, rel);
    events.push_back(tmp);
}

void Game::mondebug()
{
    int tc;

    for (auto& monster : z) {
        monster.debug(u);

        if (monster.has_flag(m_flags::MF_SEES)
            && m.sees(monster.posx, monster.posy, u.posx, u.posy, -1, tc)) {
            debugmsg("The %s can see you.", monster.name().c_str());
        } else {
            debugmsg("The %s can't see you...", monster.name().c_str());
        }
    }
}

void Game::groupdebug()
{
    erase();
    mvprintw(0, 0, "OM %d : %d    M %d : %d", cur_om.posx, cur_om.posy, levx, levy);
    int dist, linenum = 1;

    for (std::size_t i {0}; i < cur_om.zg.size(); ++i) {
        dist = trig_dist(levx, levy, cur_om.zg[i].posx, cur_om.zg[i].posy);

        if (dist <= cur_om.zg[i].radius) {
            mvprintw(linenum, 0, "Zgroup %zu: Centered at %d:%d, radius %d, pop %d", i,
                     cur_om.zg[i].posx, cur_om.zg[i].posy, cur_om.zg[i].radius,
                     cur_om.zg[i].population);
            ++linenum;
        }
    }

    getch();
}

void Game::draw_overmap()
{
    timeout(BLINK_SPEED); // Enable blinking!
    WINDOW* w_map = newwin(25, 80, 0, 0);
    bool legend = true, blink = true, note_here = false, npc_here = false;
    std::string note, npc_name;
    int cursx = (levx + 1) / 2, cursy = (levy + 1) / 2;
    int origx = cursx, origy = cursy;
    char ch {};
    overmap hori, vert, diag;
    do {
        int omx, omy;
        bool seen;
        oter_id cur_ter;
        nc_color ter_color;
        long ter_sym;
        if (cursx < 40) {
            hori = overmap(this, cur_om.posx - 1, cur_om.posy, 0);
            if (cursy < 12)
                diag = overmap(this, cur_om.posx - 1, cur_om.posy - 1, 0);
            if (cursy > OMAPY - 13)
                diag = overmap(this, cur_om.posx - 1, cur_om.posy + 1, 0);
        }
        if (cursx > OMAPX - 41) {
            hori = overmap(this, cur_om.posx + 1, cur_om.posy, 0);
            if (cursy < 12)
                diag = overmap(this, cur_om.posx + 1, cur_om.posy - 1, 0);
            if (cursy > OMAPY - 13)
                diag = overmap(this, cur_om.posx + 1, cur_om.posy + 1, 0);
        }
        if (cursy < 12)
            vert = overmap(this, cur_om.posx, cur_om.posy - 1, 0);
        if (cursy > OMAPY - 13)
            vert = overmap(this, cur_om.posx, cur_om.posy + 1, 0);
        for (int i = -40; i < 40; i++) {
            for (int j = -12; j <= (ch == 'j' ? 13 : 12); j++) {
                omx = cursx + i;
                omy = cursy + j;
                seen = false;
                npc_here = false;
                if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) { // It's in-bounds
                    cur_ter = cur_om.ter(omx, omy);
                    seen = cur_om.seen(omx, omy);

                    if ((note_here = cur_om.has_note(omx, omy))) {
                        note = cur_om.note(omx, omy);
                    }

                    for (const auto& npc : cur_om.npcs) {
                        if ((npc.mapx + 1) / 2 == omx && (npc.mapy + 1) / 2 == omy) {
                            npc_here = true;
                            npc_name = npc.name;
                            break;
                        }
                    }
                } else if (omx < 0) {
                    omx += OMAPX;
                    if (omy < 0 || omy >= OMAPY) {
                        omy += (omy < 0 ? OMAPY : 0 - OMAPY);
                        cur_ter = diag.ter(omx, omy);
                        seen = diag.seen(omx, omy);

                        if ((note_here = diag.has_note(omx, omy))) {
                            note = diag.note(omx, omy);
                        }
                    } else {
                        cur_ter = hori.ter(omx, omy);
                        seen = hori.seen(omx, omy);

                        if ((note_here = hori.has_note(omx, omy))) {
                            note = hori.note(omx, omy);
                        }
                    }
                } else if (omx >= OMAPX) {
                    omx -= OMAPX;
                    if (omy < 0 || omy >= OMAPY) {
                        omy += (omy < 0 ? OMAPY : 0 - OMAPY);
                        cur_ter = diag.ter(omx, omy);
                        seen = diag.seen(omx, omy);

                        if ((note_here = diag.has_note(omx, omy))) {
                            note = diag.note(omx, omy);
                        }
                    } else {
                        cur_ter = hori.ter(omx, omy);
                        seen = hori.seen(omx, omy);

                        if ((note_here = hori.has_note(omx, omy))) {
                            note = hori.note(omx, omy);
                        }
                    }
                } else if (omy < 0) {
                    omy += OMAPY;
                    cur_ter = vert.ter(omx, omy);
                    seen = vert.seen(omx, omy);

                    if ((note_here = vert.has_note(omx, omy))) {
                        note = vert.note(omx, omy);
                    }
                } else if (omy >= OMAPY) {
                    omy -= OMAPY;
                    cur_ter = vert.ter(omx, omy);
                    seen = vert.seen(omx, omy);

                    if ((note_here = vert.has_note(omx, omy))) {
                        note = vert.note(omx, omy);
                    }
                } else
                    debugmsg("No data loaded! omx: %d omy: %d", omx, omy);
                if (seen) {
                    if (note_here && blink) {
                        ter_color = c_yellow;
                        ter_sym = 'N';
                    } else if (omx == origx && omy == origy && blink) {
                        ter_color = u.color();
                        ter_sym = '@';
                    } else if (npc_here && blink) {
                        ter_color = c_pink;
                        ter_sym = '@';
                    } else {
                        if (cur_ter >= num_ter_types)
                            debugmsg("Bad ter %d (%d, %d)", cur_ter, omx, omy);
                        ter_color = oterlist[cur_ter].color;
                        ter_sym = oterlist[cur_ter].sym;
                    }
                } else {
                    ter_color = c_dkgray;
                    ter_sym = '#';
                }
                if (j == 0 && i == 0)
                    mvwputch_hi(w_map, 12, 40, ter_color, ter_sym);
                else
                    mvwputch(w_map, 12 + j, 40 + i, ter_color, ter_sym);
            }
        }
        if (cur_om.has_note(cursx, cursy)) {
            note = cur_om.note(cursx, cursy);

            for (int i {0}; i < static_cast<int>(note.length()); ++i) {
                mvwputch(w_map, 1, i, nc_color::c_white, LINE_OXOX);
            }

            mvwputch(w_map, 1, note.length(), c_white, LINE_XOOX);
            mvwputch(w_map, 0, note.length(), c_white, LINE_XOXO);
            mvwprintz(w_map, 0, 0, c_yellow, note.c_str());
        } else if (npc_here) {
            for (int i {0}; i < static_cast<int>(npc_name.length()); ++i) {
                mvwputch(w_map, 1, i, nc_color::c_white, LINE_OXOX);
            }

            mvwputch(w_map, 1, npc_name.length(), c_white, LINE_XOOX);
            mvwputch(w_map, 0, npc_name.length(), c_white, LINE_XOXO);
            mvwprintz(w_map, 0, 0, c_yellow, npc_name.c_str());
        }
        if (legend) {
            cur_ter = cur_om.ter(cursx, cursy);
            mvwputch(w_map, 16, 50, c_white, LINE_OXXO);
            // Clear the legend
            for (int i = 51; i < 80; i++) {
                mvwputch(w_map, 16, i, c_white, LINE_OXOX);
                for (int j = 17; j < 25; j++)
                    mvwputch(w_map, j, i, c_black, 'x');
            }
            for (int i = 17; i < 25; i++)
                mvwputch(w_map, i, 50, c_white, LINE_XOXO);
            if (cur_om.seen(cursx, cursy)) {
                mvwputch(w_map, 17, 51, oterlist[cur_ter].color, oterlist[cur_ter].sym);
                mvwprintz(w_map, 17, 53, oterlist[cur_ter].color, "%s",
                          oterlist[cur_ter].name.c_str());
            } else
                mvwprintz(w_map, 17, 51, c_dkgray, "# Unexplored");
            mvwprintz(w_map, 19, 51, c_magenta, "Use movement keys to pan.  ");
            mvwprintz(w_map, 20, 51, c_magenta, "0 - Center map on character");
            mvwprintz(w_map, 21, 51, c_magenta, "t - Toggle legend          ");
            mvwprintz(w_map, 22, 51, c_magenta, "/ - Search                 ");
            mvwprintz(w_map, 23, 51, c_magenta, "N - Add a note             ");
            mvwprintz(w_map, 24, 51, c_magenta, "Esc or q - Return to game  ");
        }
        wrefresh(w_map);
        ch = input();

        if (ch != ERR)
            blink = true;
        if (ch == 'y' || ch == 'u' || ch == 'h' || ch == 'j' || ch == 'k' || ch == 'l' || ch == 'b'
            || ch == 'n') {
            int dirx, diry;
            get_direction(dirx, diry, ch);
            cursx += dirx;
            cursy += diry;
        } else if (ch == '0') {
            cursx = origx;
            cursy = origy;
        } else if (ch == '\n') {
            z.clear();
            m.save(&cur_om, turn, levx, levy);
            levx = cursx * 2;
            levy = cursy * 2;
            m.load(this, levx, levy);
        } else if (ch == 'N') {
            timeout(-1);
            cur_om.add_note(cursx, cursy, string_input_popup("Enter note:"));
            timeout(BLINK_SPEED);
        } else if (ch == '/') {
            timeout(-1);
            std::string term = string_input_popup("Search term:");
            timeout(BLINK_SPEED);
            int range = 1;
            for (int i = 0; i < num_ter_types; i++) {
                if (oterlist[i].name.find(term) != std::string::npos) {
                    if (i == ot_forest || i == ot_hive || i == ot_hiway_ns || i == ot_bridge_ns)
                        range = 2;
                    else if (i >= ot_road_ns && i < ot_road_nesw_manhole)
                        range = ot_road_nesw_manhole - i + 1;
                    else if (i >= ot_river_center && i < ot_river_nw)
                        range = ot_river_nw - i + 1;
                    else if (i >= ot_house_north && i < ot_lab)
                        range = 4;
                    else if (i == ot_lab)
                        range = 2;
                    int maxdist = OMAPX;
                    Point found = cur_om.find_closest(Point(cursx, cursy), oter_id(i), range,
                                                      maxdist, true);
                    i = num_ter_types;
                    if (found.x != -1) {
                        cursx = found.x;
                        cursy = found.y;
                    }
                }
            }
        } else if (ch == 't')
            legend = !legend;
        else if (ch == ERR)
            blink = !blink;
    } while (ch != KEY_ESCAPE && ch != 'q' && ch != 'Q' && ch != ' ' && ch != '\n');
    timeout(-1);
    werase(w_map);
    wrefresh(w_map);
    delwin(w_map);
    erase();
    refresh_all();
}

void Game::disp_kills()
{
    WINDOW* w = newwin(25, 80, 0, 0);
    std::vector<MonsterType*> types;
    std::vector<int> count;
    for (int i = 0; i < num_monsters; i++) {
        if (kills[i] > 0) {
            types.push_back(mtypes[i]);
            count.push_back(kills[i]);
        }
    }

    mvwprintz(w, 0, 35, c_red, "KILL COUNTS:");

    if (types.size() == 0) {
        mvwprintz(w, 2, 2, c_white, "You haven't killed any monsters yet!");
        wrefresh(w);
        getch();
        werase(w);
        wrefresh(w);
        delwin(w);
        refresh_all();
        return;
    }

    for (std::size_t i {0}; i < types.size(); ++i) {
        if (i < 24) {
            mvwprintz(w, i + 1, 0, types[i]->color, "%c %s", types[i]->sym, types[i]->name.c_str());
            int hori = 25;
            if (count[i] >= 10)
                hori = 24;
            if (count[i] >= 100)
                hori = 23;
            if (count[i] >= 1000)
                hori = 22;
            mvwprintz(w, i + 1, hori, c_white, "%d", count[i]);
        } else {
            mvwprintz(w, i + 1, 40, types[i]->color, "%c %s", types[i]->sym,
                      types[i]->name.c_str());
            int hori = 65;
            if (count[i] >= 10)
                hori = 64;
            if (count[i] >= 100)
                hori = 63;
            if (count[i] >= 1000)
                hori = 62;
            mvwprintz(w, i + 1, hori, c_white, "%d", count[i]);
        }
    }

    wrefresh(w);
    getch();
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh_all();
}

void Game::disp_NPCs()
{
    WINDOW* w = newwin(25, 80, 0, 0);
    mvwprintz(w, 0, 0, c_white, "Your position: %d:%d", levx, levy);
    std::vector<npc*> closest;
    closest.push_back(&cur_om.npcs[0]);

    for (std::size_t i {1}; i < cur_om.npcs.size(); ++i) {
        if (closest.size() < 20)
            closest.push_back(&cur_om.npcs[i]);
        else if (rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy)
                 < rl_dist(levx, levy, closest[19]->mapx, closest[19]->mapy)) {
            for (int j = 0; j < 20; j++) {
                if (rl_dist(levx, levy, closest[j]->mapx, closest[j]->mapy)
                    > rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy)) {
                    closest.insert(closest.begin() + j, &cur_om.npcs[i]);
                    closest.erase(closest.end() - 1);
                    j = 20;
                }
            }
        }
    }

    for (int i = 0; i < 20; i++)
        mvwprintz(w, i + 2, 0, c_white, "%s: %d:%d", closest[i]->name.c_str(), closest[i]->mapx,
                  closest[i]->mapy);

    wrefresh(w);
    getch();
    werase(w);
    wrefresh(w);
    delwin(w);
}

faction* Game::list_factions(std::string title)
{
    std::vector<faction> valfac; // Factions that we know of.

    for (const auto& faction : factions) {
        if (faction.known_by_u) {
            valfac.push_back(faction);
        }
    }

    if (valfac.size() == 0) { // We don't know of any factions!
        popup("You don't know of any factions.  Press Spacebar...");
        return NULL;
    }
    WINDOW* w_list = newwin(25, MAX_FAC_NAME_SIZE, 0, 0);
    WINDOW* w_info = newwin(25, 80 - MAX_FAC_NAME_SIZE, 0, MAX_FAC_NAME_SIZE);
    int maxlength = 79 - MAX_FAC_NAME_SIZE;
    int sel = 0;

    // Init w_list content
    mvwprintz(w_list, 0, 0, c_white, title.c_str());

    for (std::size_t i {0}; i < valfac.size(); ++i) {
        const nc_color col = i == 0 ? nc_color::h_white : nc_color::c_white;
        mvwprintz(w_list, static_cast<int>(i) + 1, 0, col, valfac[i].name.c_str());
    }

    wrefresh(w_list);
    // Init w_info content
    // fac_*_text() is in faction.cpp
    mvwprintz(w_info, 0, 0, c_white, "Ranking: %s", fac_ranking_text(valfac[0].likes_u).c_str());
    mvwprintz(w_info, 1, 0, c_white, "Respect: %s", fac_respect_text(valfac[0].respects_u).c_str());
    std::string desc = valfac[0].describe();
    int linenum = 3;

    while (static_cast<int>(desc.length()) > maxlength) {
        size_t split = desc.find_last_of(' ', maxlength);
        std::string line = desc.substr(0, split);
        mvwprintz(w_info, linenum, 0, c_white, line.c_str());
        desc = desc.substr(split + 1);
        linenum++;
    }

    mvwprintz(w_info, linenum, 0, c_white, desc.c_str());
    wrefresh(w_info);
    char ch;
    do {
        ch = input();
        switch (ch) {
        case 'j': // Move selection down
            mvwprintz(w_list, sel + 1, 0, c_white, valfac[sel].name.c_str());

            if (sel == static_cast<int>(valfac.size()) - 1) {
                // Wrap around.
                sel = 0;
            } else
                sel++;
            break;
        case 'k': // Move selection up
            mvwprintz(w_list, sel + 1, 0, c_white, valfac[sel].name.c_str());
            if (sel == 0)
                sel = valfac.size() - 1; // Wrap around
            else
                sel--;
            break;
        case KEY_ESCAPE:
        case 'q':
            sel = -1;
            break;
        }
        if (ch == 'j' || ch == 'k') { // Changed our selection... update the windows
            mvwprintz(w_list, sel + 1, 0, h_white, valfac[sel].name.c_str());
            wrefresh(w_list);
            werase(w_info);
            // fac_*_text() is in faction.cpp
            mvwprintz(w_info, 0, 0, c_white, "Ranking: %s",
                      fac_ranking_text(valfac[sel].likes_u).c_str());
            mvwprintz(w_info, 1, 0, c_white, "Respect: %s",
                      fac_respect_text(valfac[sel].respects_u).c_str());
            std::string desc = valfac[sel].describe();
            int linenum = 3;

            while (static_cast<int>(desc.length()) > maxlength) {
                size_t split = desc.find_last_of(' ', maxlength);
                std::string line = desc.substr(0, split);
                mvwprintz(w_info, linenum, 0, c_white, line.c_str());
                desc = desc.substr(split + 1);
                linenum++;
            }

            mvwprintz(w_info, linenum, 0, c_white, desc.c_str());
            wrefresh(w_info);
        }
    } while (ch != KEY_ESCAPE && ch != '\n' && ch != 'q');
    werase(w_list);
    werase(w_info);
    delwin(w_list);
    delwin(w_info);
    refresh_all();
    if (sel == -1)
        return NULL;
    return &(factions[valfac[sel].id]);
}

void Game::display_scent()
{
    int sc;
    char ch[12];

    for (int x = u.posx - SEEX; x <= u.posx + SEEX; x++) {
        for (int y = u.posy - SEEY; y <= u.posy + SEEY; y++) {
            sc = scent(x, y);
            std::sprintf(ch, "%d", 10 * (sc - ((sc - 1) / (u.scent / 6))));
            mvputch(y + SEEY - u.posy, x + SEEX - u.posx, sev((sc - 1) / (u.scent / 6)), ch[0]);
        }
    }
}

void Game::tutorial_message(tut_lesson lesson)
{
    // Cycle through intro lessons
    if (lesson == LESSON_INTRO) {
        while (lesson != NUM_LESSONS && tutorials_seen[lesson]) {
            switch (lesson) {
            case LESSON_INTRO:
                lesson = LESSON_MOVE;
                break;
            case LESSON_MOVE:
                lesson = LESSON_LOOK;
                break;
            case LESSON_LOOK:
                lesson = NUM_LESSONS;
                break;

            default:
                break;
            }
        }
        if (lesson == NUM_LESSONS)
            return;
    }
    if (tutorials_seen[lesson])
        return;
    tutorials_seen[lesson] = true;
    popup_top(tut_text[lesson].c_str());
    refresh_all();
}

void Game::draw()
{
    // Draw map
    werase(w_terrain);
    draw_ter();
    mon_info();
    // Draw Status
    draw_HP();
    werase(w_status);
    u.disp_status(w_status);
    int minutes = int(turn / 10); // One turn = 6 seconds
    int hours = 8 + int(minutes / 60);
    minutes = minutes % 60;
    int day = 12 + int(hours / 24);
    hours = hours % 24;
    if (hours == 12)
        mvwprintz(w_status, 0, 30, c_ltgray, "July %d, 12:%02d PM", day, minutes);
    else if (hours == 0)
        mvwprintz(w_status, 0, 30, c_ltgray, "July %d, 12:%02d AM", day, minutes);
    else if (hours > 12)
        mvwprintz(w_status, 0, 30, c_ltgray, "July %d, %d:%02d PM", day, hours - 12, minutes);
    else
        mvwprintz(w_status, 0, 30, c_ltgray, "July %d, %d:%02d AM", day, hours, minutes);

    oter_id cur_ter = cur_om.ter((levx + 1) / 2, (levy + 1) / 2);
    mvwprintz(w_status, 0, 0, oterlist[cur_ter].color, oterlist[cur_ter].name.c_str());
    if (run_mode != 0)
        mvwputch(w_status, 0, 28, c_red, '!');
    wrefresh(w_status);
    // Draw messages
    write_msg();
}

void Game::draw_ter()
{
    int t = 0;
    m.draw(this, w_terrain);

    // Draw monsters
    int distx, disty;

    for (auto& monster : z) {
        disty = std::abs(monster.posy - u.posy);
        distx = std::abs(monster.posx - u.posx);

        if (distx <= SEEX && disty <= SEEY && u_see(&monster, t)) {
            monster.draw(w_terrain, u.posx, u.posy, /*inv=*/false);
        } else if (monster.has_flag(m_flags::MF_WARM) && distx <= SEEX && disty <= SEEY
                   && (u.has_active_bionic(bionic_id::bio_infrared)
                       || u.has_trait(pl_flag::PF_INFRARED))) {
            mvwputch(w_terrain, SEEY + monster.posy - u.posy, SEEX + monster.posx - u.posx,
                     nc_color::c_red, '?');
        }
    }

    // Draw NPCs.
    for (auto& npc : active_npc) {
        disty = std::abs(npc.posy - u.posy);
        distx = std::abs(npc.posx - u.posx);

        if (distx <= SEEX && disty <= SEEY && u_see(npc.posx, npc.posy, t)) {
            npc.draw(w_terrain, u.posx, u.posy, /*inv=*/false);
        }
    }

    if (u.has_disease(DI_VISUALS))
        hallucinate();
    wrefresh(w_terrain);
    if (in_tutorial && light_level() == 1) {
        if (u.has_amount(itm_flashlight, 1))
            tutorial_message(LESSON_DARK);
        else
            tutorial_message(LESSON_DARK_NO_FLASH);
    }
}

void Game::refresh_all()
{
    draw();
    draw_minimap();
    wrefresh(w_HP);
    wrefresh(w_moninfo);
    wrefresh(w_messages);
    refresh();
}

void Game::draw_HP()
{
    int curhp;
    nc_color col;
    for (int i = 0; i < num_hp_parts; i++) {
        curhp = u.hp_cur[i];
        if (curhp == u.hp_max[i])
            col = c_green;
        else if (curhp > u.hp_max[i] * .8)
            col = c_ltgreen;
        else if (curhp > u.hp_max[i] * .5)
            col = c_yellow;
        else if (curhp > u.hp_max[i] * .3)
            col = c_ltred;
        else
            col = c_red;
        if (u.has_trait(PF_HPIGNORANT)) {
            mvwprintz(w_HP, i * 2 + 1, 0, col, " ***  ");
        } else {
            if (curhp >= 100)
                mvwprintz(w_HP, i * 2 + 1, 0, col, "%d     ", curhp);
            else if (curhp >= 10)
                mvwprintz(w_HP, i * 2 + 1, 0, col, " %d    ", curhp);
            else
                mvwprintz(w_HP, i * 2 + 1, 0, col, "  %d    ", curhp);
        }
    }
    mvwprintz(w_HP, 0, 0, c_ltgray, "HEAD:");
    mvwprintz(w_HP, 2, 0, c_ltgray, "TORSO:");
    mvwprintz(w_HP, 4, 0, c_ltgray, "L. ARM:");
    mvwprintz(w_HP, 6, 0, c_ltgray, "R. ARM:");
    mvwprintz(w_HP, 8, 0, c_ltgray, "L. LEG:");
    mvwprintz(w_HP, 10, 0, c_ltgray, "R. LEG:");
    mvwprintz(w_HP, 12, 0, c_ltgray, "POW:");
    if (u.max_power_level == 0)
        mvwprintz(w_HP, 13, 0, c_ltgray, " --   ");
    else {
        if (u.power_level == u.max_power_level)
            col = c_blue;
        else if (u.power_level >= u.max_power_level * .5)
            col = c_ltblue;
        else if (u.power_level > 0)
            col = c_yellow;
        else
            col = c_red;
        if (u.power_level >= 100)
            mvwprintz(w_HP, 13, 0, col, "%d     ", u.power_level);
        else if (u.power_level >= 10)
            mvwprintz(w_HP, 13, 0, col, " %d    ", u.power_level);
        else
            mvwprintz(w_HP, 13, 0, col, "  %d    ", u.power_level);
    }
    wrefresh(w_HP);
}

void Game::draw_minimap()
{
    // Draw the box
    werase(w_minimap);
    mvwputch(w_minimap, 0, 0, c_white, LINE_OXXO);
    mvwputch(w_minimap, 0, 6, c_white, LINE_OOXX);
    mvwputch(w_minimap, 6, 0, c_white, LINE_XXOO);
    mvwputch(w_minimap, 6, 6, c_white, LINE_XOOX);
    for (int i = 1; i < 6; i++) {
        mvwputch(w_minimap, i, 0, c_white, LINE_XOXO);
        mvwputch(w_minimap, i, 6, c_white, LINE_XOXO);
        mvwputch(w_minimap, 0, i, c_white, LINE_OXOX);
        mvwputch(w_minimap, 6, i, c_white, LINE_OXOX);
    }

    int cursx = (levx + 1) / 2;
    int cursy = (levy + 1) / 2;
    int omx, omy;
    auto cur_ter {oter_id::ot_null};
    nc_color ter_color;
    long ter_sym;
    bool seen = true;
    overmap hori;
    overmap vert;
    if (cursx < 2)
        hori = overmap(this, cur_om.posx - 1, cur_om.posy, 0);
    if (cursx > OMAPX - 3)
        hori = overmap(this, cur_om.posx + 1, cur_om.posy, 0);
    if (cursy < 2)
        vert = overmap(this, cur_om.posx, cur_om.posy - 1, 0);
    if (cursy > OMAPY - 3)
        vert = overmap(this, cur_om.posx, cur_om.posy + 1, 0);
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            omx = cursx + i;
            omy = cursy + j;
            if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) {
                cur_ter = cur_om.ter(omx, omy);
                cur_om.seen(omx, omy) = true;
            } else if ((omx < 0 || omx >= OMAPX) && (omy < 0 || omy >= OMAPY)) {
                cur_ter = ot_null;
            } else if (omx < 0) {
                omx += OMAPX;
                cur_ter = hori.ter(omx, omy);
                hori.seen(omx, omy) = true;
            } else if (omx >= OMAPX) {
                omx -= OMAPX;
                cur_ter = hori.ter(omx, omy);
                hori.seen(omx, omy) = true;
            } else if (omy < 0) {
                omy += OMAPY;
                cur_ter = vert.ter(omx, omy);
                vert.seen(omx, omy) = true;
            } else if (omy >= OMAPY) {
                omy -= OMAPY;
                cur_ter = vert.ter(omx, omy);
                vert.seen(omx, omy) = true;
            } else {
                debugmsg("No data loaded! omx: %d omy: %d", omx, omy);
            }
            ter_color = oterlist[cur_ter].color;
            ter_sym = oterlist[cur_ter].sym;
            if (i == 0 && j == 0)
                mvwputch_hi(w_minimap, 3, 3, ter_color, ter_sym);
            else
                mvwputch(w_minimap, 3 + j, 3 + i, ter_color, ter_sym);
        }
    }
    // Two spaces away!
    int barx, bary;
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            omx = cursx + i;
            omy = cursy + j;
            barx = cursx + sgn(i);
            bary = cursy + sgn(j);
            if (abs(i) == 2 || abs(j) == 2) {
                if (i == 0)
                    barx = cursx;
                if (j == 0)
                    bary = cursy;
                seen = false;
                if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY)
                    seen = cur_om.seen(omx, omy);
                if (barx >= 0 && barx < OMAPX && bary >= 0 && bary < OMAPY) {
                    cur_ter = cur_om.ter(barx, bary);
                } else if ((barx < 0 || barx >= OMAPX) && (bary < 0 || bary >= OMAPY)) {
                    cur_ter = ot_null;
                } else if (barx < 0) {
                    barx += OMAPX;
                    cur_ter = hori.ter(barx, bary);
                } else if (barx >= OMAPX) {
                    barx -= OMAPX;
                    cur_ter = hori.ter(barx, bary);
                } else if (bary < 0) {
                    bary += OMAPY;
                    cur_ter = vert.ter(barx, bary);
                } else if (bary >= OMAPY) {
                    bary -= OMAPY;
                    cur_ter = vert.ter(barx, bary);
                }
                if (oterlist[cur_ter].see_cost <= 2 || seen) {
                    if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) {
                        cur_ter = cur_om.ter(omx, omy);
                        cur_om.seen(omx, omy) = true;
                    } else if ((omx < 0 || omx >= OMAPX) && (omy < 0 || omy >= OMAPY)) {
                        cur_ter = ot_null;
                    } else if (omx < 0) {
                        omx += OMAPX;
                        cur_ter = hori.ter(omx, omy);
                        hori.seen(omx, omy) = true;
                    } else if (omx >= OMAPX) {
                        omx -= OMAPX;
                        cur_ter = hori.ter(omx, omy);
                        hori.seen(omx, omy) = true;
                    } else if (omy < 0) {
                        omy += OMAPY;
                        cur_ter = vert.ter(omx, omy);
                        vert.seen(omx, omy) = true;
                    } else if (omy >= OMAPY) {
                        omy -= OMAPY;
                        cur_ter = vert.ter(omx, omy);
                        vert.seen(omx, omy) = true;
                    }
                    ter_color = oterlist[cur_ter].color;
                    ter_sym = oterlist[cur_ter].sym;
                    mvwputch(w_minimap, 3 + j, 3 + i, ter_color, ter_sym);
                }
            }
        }
    }
    wrefresh(w_minimap);
}

void Game::hallucinate()
{
    for (int i = 0; i <= SEEX * 2 + 1; i++) {
        for (int j = 0; j <= SEEY * 2 + 1; j++) {
            if (one_in(10)) {
                char ter_sym = terlist[m.ter(i + rng(-2, 2), j + rng(-2, 2))].sym;
                nc_color ter_col = terlist[m.ter(i + rng(-2, 2), j + rng(-2, 2))].color;
                mvwputch(w_terrain, j, i, ter_col, ter_sym);
            }
        }
    }
    wrefresh(w_terrain);
}

unsigned char Game::light_level()
{
    int ret;
    if (levz < 0) // Underground!
        ret = 1;
    else {
        int minutes = int(turn / 10); // One turn = 6 seconds
        int hours = 8 + int(minutes / 60);
        minutes = minutes % 60;
        hours = hours % 24;
        if (hours < 6 || hours >= 21)
            ret = 1;
        else if (hours >= 6 && hours < 7)
            ret = int(minutes);
        else if (hours >= 20 && hours < 21)
            ret = 60 - int(minutes);
        else
            ret = 60;
    }
    if (ret < 10 && u.has_active_item(itm_flashlight_on))
        ret = 10;
    if (ret < 8 && u.has_active_bionic(bio_flashlight))
        ret = 8;
    return ret;
}

bool Game::sees_u(int x, int y, int& t)
{
    return (!u.has_active_bionic(bio_cloak) && m.sees(x, y, u.posx, u.posy, light_level(), t));
}

bool Game::u_see(int x, int y, int& t)
{
    // if (debugmon)
    // debugmsg("u_see range %d (light level %d)", u.sight_range(light_level()), light_level());
    return m.sees(u.posx, u.posy, x, y, u.sight_range(light_level()), t);
}

bool Game::u_see(Monster* mon, int& t)
{
    if (mon->has_flag(MF_DIGS) && !u.has_active_bionic(bio_ground_sonar)
        && rl_dist(u.posx, u.posy, mon->posx, mon->posy) > 1)
        return false; // Can't see digging monsters until we're right next to them
    int range = u.sight_range(light_level());
    return m.sees(u.posx, u.posy, mon->posx, mon->posy, range, t);
}

bool Game::pl_sees(player* p, Monster* mon, int& t)
{
    if (mon->has_flag(MF_DIGS) && !p->has_active_bionic(bio_ground_sonar)
        && rl_dist(p->posx, p->posy, mon->posx, mon->posy) > 1)
        return false; // Can't see digging monsters until we're right next to them
    int range = p->sight_range(light_level());
    return m.sees(p->posx, p->posy, mon->posx, mon->posy, range, t);
}

Point Game::find_item(Item* it)
{
    if (u.has_item(it))
        return Point(u.posx, u.posy);
    Point ret = m.find_item(it);
    if (ret.x != -1 && ret.y != -1)
        return ret;

    for (const auto& npc : active_npc) {
        for (const auto& item : npc.inv) {
            if (it == &item) {
                return {npc.posx, npc.posy};
            }
        }
    }

    return Point(-999, -999);
}

void Game::remove_item(Item* it)
{
    Point ret;
    if (it == &u.weapon) {
        u.remove_weapon();
        return;
    }

    for (std::size_t i {0}; i < u.inv.size(); ++i) {
        if (it == &u.inv[i]) {
            u.i_remn(static_cast<int>(i));
            return;
        }
    }

    for (std::size_t i {0}; i < u.worn.size(); ++i) {
        if (it == &u.worn[i]) {
            u.worn.erase(u.worn.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }

    ret = m.find_item(it);
    if (ret.x != -1 && ret.y != -1) {
        for (std::size_t i {0}; i < m.i_at(ret.x, ret.y).size(); ++i) {
            if (it == &m.i_at(ret.x, ret.y)[i]) {
                m.i_rem(ret.x, ret.y, static_cast<int>(i));
                return;
            }
        }
    }

    for (auto& npc : active_npc) {
        if (it == &npc.weapon) {
            npc.remove_weapon();
            return;
        }

        for (std::size_t j {0}; j < npc.inv.size(); ++j) {
            if (it == &npc.inv[j]) {
                npc.i_remn(static_cast<int>(j));
                return;
            }
        }

        for (std::size_t j {0}; j < u.worn.size(); ++j) {
            if (it == &npc.worn[j]) {
                npc.worn.erase(u.worn.begin() + static_cast<int>(j));
                return;
            }
        }
    }
}

bool vector_has(std::vector<int> vec, int test)
{
    return std::ranges::any_of(vec, [test](const int value) { return test == value; });
}

void Game::mon_info()
{
    werase(w_moninfo);
    int buff;
    int newseen = 0;
    // 0 1 2
    // 3 4 5
    // 6 7 8
    std::vector<int> unique_types[10];
    int direction;

    for (auto& monster : z) {
        if (u_see(&monster, buff)) {
            if (!monster.is_fleeing(u) && monster.friendly == 0) {
                ++newseen;
            }

            if (monster.posx < u.posx - SEEX) {
                if (monster.posy < u.posy - SEEY) {
                    direction = 0;
                } else if (monster.posy > u.posy + SEEY) {
                    direction = 6;
                } else {
                    direction = 3;
                }
            } else if (monster.posx > u.posx + SEEX) {
                if (monster.posy < u.posy - SEEY) {
                    direction = 2;
                } else if (monster.posy > u.posy + SEEY) {
                    direction = 8;
                } else {
                    direction = 5;
                }
            } else {
                if (monster.posy < u.posy - SEEY) {
                    direction = 1;
                } else if (monster.posy > u.posy + SEEY) {
                    direction = 7;
                } else {
                    direction = 4;
                }
            }

            if (!vector_has(unique_types[direction], monster.type->id)) {
                unique_types[direction].push_back(monster.type->id);
            }
        }
    }

    for (std::size_t i {0}; i < active_npc.size(); ++i) {
        if (u_see(active_npc[i].posx, active_npc[i].posy, buff)) {
            // TODO: NPC invisibility.
            if (active_npc[i].attitude == npc_attitude::NPCATT_KILL) {
                ++newseen;
            }

            if (active_npc[i].posx < u.posx - SEEX) {
                if (active_npc[i].posy < u.posy - SEEY) {
                    direction = 0;
                } else if (active_npc[i].posy > u.posy + SEEY) {
                    direction = 6;
                } else {
                    direction = 3;
                }
            } else if (active_npc[i].posx > u.posx + SEEX) {
                if (active_npc[i].posy < u.posy - SEEY) {
                    direction = 2;
                } else if (active_npc[i].posy > u.posy + SEEY) {
                    direction = 8;
                } else {
                    direction = 5;
                }
            } else {
                if (active_npc[i].posy < u.posy - SEEY) {
                    direction = 1;
                } else if (active_npc[i].posy > u.posy + SEEY) {
                    direction = 7;
                } else {
                    direction = 4;
                }
            }

            unique_types[direction].push_back(-1 - static_cast<int>(i));
        }
    }

    if (newseen > mostseen) {
        cancel_activity_query("Monster spotted!");
        if (run_mode == 1)
            run_mode = 2; // Stop movement!
    }
    mostseen = newseen;
    int line = 0;
    nc_color tmpcol;
    for (int i = 0; i < 9; i++) {
        if (unique_types[i].size() > 0) {
            switch (i) {
            case 0:
                mvwprintz(w_moninfo, line, 0, c_magenta, "NORTHWEST");
                break;
            case 1:
                mvwprintz(w_moninfo, line, 0, c_magenta, "NORTH");
                break;
            case 2:
                mvwprintz(w_moninfo, line, 0, c_magenta, "NORTHEAST");
                break;
            case 3:
                mvwprintz(w_moninfo, line, 0, c_magenta, "WEST");
                break;
            case 4:
                mvwprintz(w_moninfo, line, 0, c_magenta, "NEARBY");
                break;
            case 5:
                mvwprintz(w_moninfo, line, 0, c_magenta, "EAST");
                break;
            case 6:
                mvwprintz(w_moninfo, line, 0, c_magenta, "SOUTHWEST");
                break;
            case 7:
                mvwprintz(w_moninfo, line, 0, c_magenta, "SOUTH");
                break;
            case 8:
                mvwprintz(w_moninfo, line, 0, c_magenta, "SOUTHEAST");
                break;
            }
            line++;
        }

        for (std::size_t j {0}; j < unique_types[i].size() && line < 12; ++j) {
            buff = unique_types[i][j];
            if (buff < 0) {
                switch (active_npc[(buff + 1) * -1].attitude) {
                case NPCATT_KILL:
                    tmpcol = c_red;
                    break;
                case NPCATT_FOLLOW:
                    tmpcol = c_ltgreen;
                    break;
                case NPCATT_DEFEND:
                    tmpcol = c_green;
                    break;
                default:
                    tmpcol = c_pink;
                    break;
                }
                mvwputch(w_moninfo, line, 0, tmpcol, '@');
                mvwprintw(w_moninfo, line, 2, "%s", active_npc[(buff + 1) * -1].name.c_str());
            } else {
                mvwputch(w_moninfo, line, 0, mtypes[buff]->color, mtypes[buff]->sym);
                mvwprintw(w_moninfo, line, 2, "%s", mtypes[buff]->name.c_str());
            }
            line++;
        }
    }
    wrefresh(w_moninfo);
    refresh();
}

void Game::monmove()
{
    for (std::size_t i {0}; i < z.size(); ++i) {
        if (i > z.size()) {
            debugmsg("Moving out of bounds monster! i %d, z.size() %d", i, z.size());
        }

        while (!z[i].can_move_to(m, z[i].posx, z[i].posy) && i < z.size()) {
            // If we can't move to our current position, assign us to a new one
            bool okay = false;
            int xdir = rng(1, 2) * 2 - 3, ydir = rng(1, 2) * 2 - 3; // -1 or 1
            int startx = z[i].posx - 3 * xdir, endx = z[i].posx + 3 * xdir;
            int starty = z[i].posy - 3 * ydir, endy = z[i].posy + 3 * ydir;
            for (int x = startx; x != endx && !okay; x += xdir) {
                for (int y = starty; y != endy && !okay; y += ydir) {
                    if (z[i].can_move_to(m, x, y)) {
                        z[i].posx = x;
                        z[i].posy = y;
                        okay = true;
                    }
                }
            }
            if (!okay)
                z.erase(z.begin() + i); // Delete us if no replacement found
        }

        bool dead = false;
        while (z[i].moves > 0 && !dead) {
            z[i].plan(this); // Formulate a path to follow
            z[i].move(this); // Move one square, possibly hit u
            m.mon_in_field(z[i].posx, z[i].posy, &z[i]);

            if (z[i].hurt(0)) { // Maybe we died...
                kill_mon(i);
                dead = true;
            }
        }
        if (dead)
            i--;
        else {
            if (in_tutorial && u.pain > 0)
                tutorial_message(LESSON_PAIN);
            if (u.has_active_bionic(bio_alarm) && u.power_level >= 1 && abs(z[i].posx - u.posx) <= 5
                && abs(z[i].posy - u.posy) <= 5) {
                u.power_level--;
                add_msg("Your motion alarm goes off!");
                u.activity.type = ACT_NULL;
                if (u.has_disease(DI_SLEEP) || u.has_disease(DI_LYING_DOWN)) {
                    u.rem_disease(DI_SLEEP);
                    u.rem_disease(DI_LYING_DOWN);
                }
            }
            // We might have stumbled out of range of the player; if so, delete us
            if (z[i].posx < 0 - SEEX * 3 || z[i].posy < 0 - SEEX * 3 || z[i].posx > SEEX * 6
                || z[i].posy > SEEY * 6) {
                int group = valid_group((mon_id)(z[i].type->id), levx, levy);
                if (group != -1) {
                    cur_om.zg[group].population++;
                    if (cur_om.zg[group].population / pow(cur_om.zg[group].radius, 2) > 5)
                        cur_om.zg[group].radius++;
                } else if (mt_to_mc((mon_id)(z[i].type->id)) != mcat_null) {
                    cur_om.zg.push_back(
                        mongroup(mt_to_mc((mon_id)(z[i].type->id)), levx, levy, 1, 1));
                }
                z.erase(z.begin() + i);
                i--;
            } else
                z[i].moves += z[i].speed;
        }
    }

    // Now, do active NPCs.
    for (std::size_t i {0}; i < active_npc.size(); ++i) {
        if (active_npc[i].hp_cur[hp_head] <= 0 || active_npc[i].hp_cur[hp_torso] <= 0) {
            active_npc[i].die(this);
            active_npc.erase(active_npc.begin() + static_cast<std::ptrdiff_t>(i));
            --i;
        } else {
            active_npc[i].reset();
            active_npc[i].suffer(this);

            while (active_npc[i].moves > 0) {
                active_npc[i].move(this);
            }
        }
    }
}

void Game::om_npcs_move()
{
    /*
     for (int i = 0; i < cur_om.npcs.size(); i++) {
      cur_om.npcs[i].perform_mission(this);
      if (abs(cur_om.npcs[i].mapx - levx) <= 1 && abs(cur_om.npcs[i].mapy - levy) <= 1) {
       cur_om.npcs[i].posx = u.posx + SEEX * 2 * (cur_om.npcs[i].mapx - levx);
       cur_om.npcs[i].posy = u.posy + SEEY * 2 * (cur_om.npcs[i].mapy - levy);
       active_npc.push_back(cur_om.npcs[i]);
       cur_om.npcs.erase(cur_om.npcs.begin() + i);
       i--;
      }
     }
    */
}

void Game::check_warmth()
{
    // HEAD
    int warmth = u.warmth(bp_head) + int((temp - 65) / 10);
    if (warmth <= -6) {
        add_msg("Your head is freezing!");
        u.hurt(this, bp_head, 0, (0 - (warmth / 2)));
    } else if (warmth <= -3) {
        add_msg("Your head is cold.");
        u.hurt(this, bp_head, 0, 2);
    } else if (warmth >= 8) {
        add_msg("Your head is overheating!");
        u.hurt(this, bp_head, 0, warmth - 5);
    }
    if (warmth >= 6) {
        if (warmth < 12)
            add_msg("Your head is too hot.");
        if (rng(0, 15) < warmth)
            u.add_disease(DI_HEATSTROKE, warmth * rng(1, 16), this);
    }
    // FACE -- Mouth and eyes
    warmth = u.warmth(bp_eyes) + u.warmth(bp_mouth) + int((temp - 65) / 10);
    if (warmth <= -6) {
        add_msg("Your face is freezing!");
        u.hurt(this, bp_head, 0, (0 - warmth));
    } else if (warmth <= -4) {
        add_msg("Your face is cold.");
        u.hurt(this, bp_head, 0, 1);
        if (rng(0, -10) > warmth)
            u.add_disease(DI_FBFACE, warmth * rng(-6, -16), this);
    } else if (warmth >= 12) {
        add_msg("Your face is overheating!");
        u.hurt(this, bp_head, 0, warmth - 6);
    }
    if (warmth >= 9) {
        if (warmth < 12)
            add_msg("Your face is too hot.");
        if (rng(0, 15) < warmth)
            u.add_disease(DI_HEATSTROKE, warmth * rng(4, 16), this);
    }
    // TORSO
    warmth = u.warmth(bp_torso) + int((temp - 65) / 10);
    if (warmth <= -5) {
        add_msg("Your body is freezing!");
        u.hurt(this, bp_torso, 0, 0 - warmth);
    } else if (warmth <= -2) {
        add_msg("Your body is cold.");
        u.hurt(this, bp_torso, 0, rng(1, 2));
    } else if (warmth >= 12) {
        add_msg("Your body is overheating!");
        u.hurt(this, bp_torso, 0, warmth - 6);
    }
    if (warmth >= 8) {
        if (warmth < 12)
            add_msg("Your body is too hot.");
        if (rng(0, 15) < warmth)
            u.add_disease(DI_HEATSTROKE, warmth * rng(6, 18), this);
    }
    // HANDS
    warmth = u.warmth(bp_hands) + int((temp - 65) / 10);
    if (warmth <= -4) {
        add_msg("Your hands are freezing!");
        if (rng(0, -10) > warmth)
            u.add_disease(DI_FBHANDS, warmth * rng(-6, -20), this);
    } else if (warmth >= 8) {
        add_msg("Your hands are overheating!");
        if (rng(0, 15) < warmth)
            u.add_disease(DI_HEATSTROKE, warmth * rng(1, 5), this);
    }
    // LEGS
    warmth = u.warmth(bp_legs) + int((temp - 65) / 10);
    if (warmth <= -6) {
        add_msg("Your legs are freezing!");
        u.moves -= 50;
        u.hurt(this, bp_legs, 0, 1);
        u.hurt(this, bp_legs, 1, 1);
    } else if (warmth <= -3) {
        add_msg("Your legs are very cold.");
        u.moves -= 20;
    } else if (warmth >= 8) {
        add_msg("Your legs are overheating!");
        if (rng(0, 15) < warmth)
            u.add_disease(DI_HEATSTROKE, warmth * rng(2, 8), this);
    }
    // FEET
    warmth = u.warmth(bp_feet) + int((temp - 65) / 10);
    if (warmth <= -3) {
        add_msg("Your feet are freezing!");
        if (rng(0, -10) > warmth)
            u.add_disease(DI_FBFEET, warmth * rng(-4, -20), this);
    } else if (warmth >= 12) {
        add_msg("Your feet are overheating!");
        if (rng(0, 15) < warmth)
            u.add_disease(DI_HEATSTROKE, warmth * rng(2, 10), this);
    }
}

void Game::sound(int x, int y, int vol, std::string description)
{
    // First, alert all monsters (that can hear) to the sound
    double dist;

    for (auto& monster : z) {
        if (monster.has_flag(m_flags::MF_HEARS)) {
            dist = trig_dist(x, y, monster.posx, monster.posy);

            if (monster.has_flag(m_flags::MF_GOODHEARING) && static_cast<int>(dist) / 2 <= vol) {
                monster.wander_to(x, y, vol - (static_cast<int>(dist) / 2));
            } else if (dist <= vol && dist >= 2) {
                // Adjacent sounds are likely cause by us.
                monster.wander_to(x, y, static_cast<int>(vol - dist));
            }
        }
    }

    // Loud sounds make the next spawn sooner!
    if (vol >= 20 && nextspawn > vol + 20) {
        int max = (vol - 20);
        int min = int(max / 6);
        if (max > 50)
            max = 50;
        nextspawn -= rng(min, max);
    }
    // Next, display the sound as the player hears it
    if (description == "")
        return; // No description (e.g., footsteps)
    if (u.has_bionic(bio_ears))
        vol = vol * 3.5;
    if (u.has_trait(PF_BADHEARING))
        vol = vol * .5;
    dist = trig_dist(x, y, u.posx, u.posy);
    if (dist > vol)
        return; // Too far away, we didn't hear it!
    if (u.has_disease(DI_SLEEP)
        && ((!u.has_trait(PF_HEAVYSLEEPER) && dice(2, 20) < vol - dist)
            || (u.has_trait(PF_HEAVYSLEEPER) && dice(3, 20) < vol - dist))) {
        u.rem_disease(DI_SLEEP);
        add_msg("You're woken up by a noise.");
        return;
    }
    cancel_activity_query("Heard a noise!");
    // We need to figure out where it was coming from, relative to the player
    int dx = x - u.posx;
    int dy = y - u.posy;
    // If it came from us, don't print a direction
    if (dx == 0 && dy == 0) {
        if (description[0] >= 'a' && description[0] <= 'z')
            description[0] += 'A' - 'a'; // Capitalize the sound
        add_msg("%s", description.c_str());
        return;
    }
    std::string direction = direction_from(u.posx, u.posy, x, y);
    add_msg("From %s you hear %s", direction.c_str(), description.c_str());
}

void Game::explosion(int x, int y, int power, int shrapnel, bool fire)
{
    timespec ts; // Timespec for the animation of the explosion
    ts.tv_sec = 0;
    ts.tv_nsec = EXPLOSION_SPEED;
    int radius = sqrt(power / 4);
    int dam;
    std::string junk;
    if (power >= 30)
        sound(x, y, power * 10, "a huge explosion!");
    else
        sound(x, y, power * 10, "an explosion!");
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            if (i == x && j == y)
                dam = 3 * power;
            else
                dam = 3 * power / (abs(i - x) + abs(j - y));
            if (m.has_flag(bashable, i, j))
                m.bash(i, j, dam, junk);
            if (m.has_flag(bashable, i, j)) // Double up for tough doors, etc.
                m.bash(i, j, dam, junk);
            if (m.is_destructable(i, j) && rng(25, 100) < dam)
                m.destroy(this, i, j, false);
            if (mon_at(i, j) != -1 && z[mon_at(i, j)].hurt(rng(dam / 2, dam * 1.5)))
                kill_mon(mon_at(i, j));
            if (u.posx == i && u.posy == j) {
                add_msg("You're caught in the explosion!");
                u.hit(this, bp_torso, 0, rng(dam / 2, dam * 1.5), 0);
                u.hit(this, bp_head, 0, rng(dam / 3, dam), 0);
                u.hit(this, bp_legs, 0, rng(dam / 3, dam), 0);
                u.hit(this, bp_legs, 1, rng(dam / 3, dam), 0);
                u.hit(this, bp_arms, 0, rng(dam / 3, dam), 0);
                u.hit(this, bp_arms, 1, rng(dam / 3, dam), 0);
            }
            if (fire) {
                if (m.field_at(i, j).type == fd_smoke)
                    m.field_at(i, j) = field(fd_fire, 1, 0);
                m.add_field(this, i, j, fd_fire, dam / 10);
            }
        }
    }
    for (int i = 1; i <= radius; i++) {
        mvwputch(w_terrain, y - i + SEEY - u.posy, x - i + SEEX - u.posx, c_red, '/');
        mvwputch(w_terrain, y - i + SEEY - u.posy, x + i + SEEX - u.posx, c_red, '\\');
        mvwputch(w_terrain, y + i + SEEY - u.posy, x - i + SEEX - u.posx, c_red, '\\');
        mvwputch(w_terrain, y + i + SEEY - u.posy, x + i + SEEX - u.posx, c_red, '/');
        for (int j = 1 - i; j < 0 + i; j++) {
            mvwputch(w_terrain, y - i + SEEY - u.posy, x + j + SEEX - u.posx, c_red, '-');
            mvwputch(w_terrain, y + i + SEEY - u.posy, x + j + SEEX - u.posx, c_red, '-');
            mvwputch(w_terrain, y + j + SEEY - u.posy, x - i + SEEX - u.posx, c_red, '|');
            mvwputch(w_terrain, y + j + SEEY - u.posy, x + i + SEEX - u.posx, c_red, '|');
        }
        wrefresh(w_terrain);
        nanosleep(&ts, NULL);
    }

    if (shrapnel <= 0)
        return;
    int sx, sy, t, ijunk, tx, ty;
    std::vector<Point> traj;
    ts.tv_sec = 0;
    ts.tv_nsec = BULLET_SPEED; // Reset for animation of bullets
    for (int i = 0; i < shrapnel; i++) {
        sx = rng(x - radius, x + radius);
        sy = rng(y - radius, y + radius);
        if (m.sees(x, y, sx, sy, 50, t))
            traj = line_to(x, y, sx, sy, t);
        else
            traj = line_to(x, y, sx, sy, 0);
        dam = rng(40, 100);

        for (std::size_t j {0}; j < traj.size(); ++j) {
            if (i > 0)
                m.drawsq(w_terrain, u, traj[i - 1].x, traj[i - 1].y, false, true);
            if (u_see(traj[i].x, traj[i].y, ijunk)) {
                mvwputch(w_terrain, traj[i].y + SEEY - u.posy, traj[i].x + SEEX - u.posx, c_red,
                         '`');
                wrefresh(w_terrain);
                nanosleep(&ts, NULL);
            }
            tx = traj[i].x;
            ty = traj[i].y;
            if (mon_at(tx, ty) != -1) {
                dam -= z[mon_at(tx, ty)].armor();
                if (z[mon_at(tx, ty)].hurt(dam))
                    kill_mon(mon_at(tx, ty));
            } else if (npc_at(tx, ty) != -1) {
                body_part hit = random_body_part();
                if (hit == bp_eyes || hit == bp_mouth || hit == bp_head)
                    dam = rng(2 * dam, 5 * dam);
                else if (hit == bp_torso)
                    dam = rng(1.5 * dam, 3 * dam);
                int npcdex = npc_at(tx, ty);
                active_npc[npcdex].hit(this, hit, rng(0, 1), 0, dam);
                if (active_npc[npcdex].hp_cur[hp_head] <= 0
                    || active_npc[npcdex].hp_cur[hp_torso] <= 0) {
                    active_npc[npcdex].die(this);
                    active_npc.erase(active_npc.begin() + npcdex);
                }
            } else {
                m.shoot(this, tx, ty, dam, j == traj.size() - 1);
            }
        }
    }
}

void Game::use_computer(int x, int y)
{
    int difficulty;
    std::vector<std::string> options;
    u.practice(sk_computer, 5);
    switch (m.ter(x, y)) {
    case t_computer_nether:
        difficulty = 4;
        options.push_back("Release Specimens");
        options.push_back("Terminate Specimens");
        options.push_back("Flash Portal");
        options.push_back("Activate Resonance Cascade");
        break;
    case t_computer_lab:
        difficulty = 2;
        options.push_back("Read Research Logs");
        options.push_back("Download Map Data");
        break;
    default:
        debugmsg("%s is not a computer!", m.tername(x, y).c_str());
        return;
    }
    int success = u.sklevel[sk_computer] - difficulty + int((u.int_cur - 8) / 2)
        + rng(0 - int(difficulty / 2), int(difficulty / 2));
    int choice = menu_vec(m.tername(x, y).c_str(), options);

    switch (m.ter(x, y)) {
    case t_computer_nether:
        if (success < -2) {
            sound(u.posx, u.posy, 40, "An alarm sounds!");
            add_msg("Manhacks drop from compartments in the ceiling.");
            Monster robot(mtypes[mon_manhack]);
            int mx, my, num_robots = rng(4, 8);
            for (int i = 0; i < num_robots; i++) {
                int tries = 0;
                do {
                    mx = rng(u.posx - 3, u.posx + 3);
                    my = rng(u.posy - 3, u.posy + 3);
                    tries++;
                } while ((m.move_cost(mx, my) == 0 || mon_at(mx, my) != -1) && tries < 10);
                if (tries != 10) {
                    robot.spawn(mx, my);
                    z.push_back(robot);
                }
            }
            return;
        }
        switch (choice) {
        case 1:
            sound(x - 3, y - 3, 40, "an alarm sounding!");
            if (success <= 0) {
                add_msg("ERROR: Improper variable entered.  Please try again.");
                return;
            }
            for (int i = 0; i < SEEX * 3; i++) {
                for (int j = 0; j < SEEY * 3; j++) {
                    if (m.ter(i, j) == t_reinforced_glass_h || m.ter(i, j) == t_reinforced_glass_v)
                        m.ter(i, j) = t_floor;
                }
            }
            break;
        case 2:
            if (success <= 1) {
                add_msg("ERROR: Improper variable entered.  Please try again.");
                return;
            }
            for (int i = 0; i < SEEX * 3; i++) {
                for (int j = 0; j < SEEY * 3; j++) {
                    if (((m.ter(i, j - 1) == t_reinforced_glass_h && m.ter(i, j + 1) == t_wall_h)
                         || (m.ter(i, j + 1) == t_reinforced_glass_h
                             && m.ter(i, j - 1) == t_wall_h))
                        && mon_at(i, j) != -1)
                        kill_mon(mon_at(i, j));
                }
            }
            break;
        case 3:
            if (success < 0) {
                add_msg("Sequence incomplete.  Please try again.");
                return;
            } else if (success < 2) {
                popup("ERROR!  Phase interference override!");
                resonance_cascade(u.posx, u.posy);
                return;
            }
            for (int i = 0; i < SEEX * 3; i++) {
                for (int j = 0; j < SEEY * 3; j++) {
                    int numtowers = 0;
                    for (int xt = i - 2; xt <= i + 2; xt++) {
                        for (int yt = j - 2; yt <= j + 2; yt++) {
                            if (m.ter(xt, yt) == t_radio_tower)
                                numtowers++;
                        }
                    }
                    if (numtowers == 4) {
                        if (m.tr_at(i, j) == tr_portal)
                            m.tr_at(i, j) = tr_null;
                        else
                            m.add_trap(i, j, tr_portal);
                    }
                }
            }
            break;
        case 4:
            if (success < 2) {
                add_msg("Sequence incomplete; please try again.");
                return;
            }
            if (!query_yn("WARNING: Resonance Cascade carries severe risk!  Continue?"))
                return;
            if (success > 5) {
                std::vector<Point> cascade_points;
                for (int i = x - 10; i <= x + 10; i++) {
                    for (int j = y - 10; j <= y + 10; j++) {
                        if (m.ter(i, j) == t_radio_tower)
                            cascade_points.push_back(Point(i, j));
                    }
                }
                if (cascade_points.size() == 0)
                    resonance_cascade(u.posx, u.posy);
                else {
                    Point p = cascade_points[rng(0, cascade_points.size() - 1)];
                    resonance_cascade(p.x, p.y);
                }
            } else
                resonance_cascade(u.posx, u.posy);
            break;
        }
        break;
    case t_computer_lab:
        if (success < -2) {
            sound(u.posx, u.posy, 40, "An alarm sounds!");
            add_msg("Manhacks drop from compartments in the ceiling.");
            Monster robot(mtypes[mon_manhack]);
            int mx, my, num_robots = rng(4, 8);
            for (int i = 0; i < num_robots; i++) {
                int tries = 0;
                do {
                    mx = rng(u.posx - 3, u.posx + 3);
                    my = rng(u.posy - 3, u.posy + 3);
                    tries++;
                } while ((m.move_cost(mx, my) == 0 || mon_at(mx, my) != -1) && tries < 10);
                if (tries != 10) {
                    robot.spawn(mx, my);
                    z.push_back(robot);
                }
            }
            return;
        }
        switch (choice) {
        case 1:
            if (success >= 1) {
                int lines = 0, notes = 0;
                std::string log, tmp;
                int ch;
                const std::filesystem::path lab_notes_file {"data/LAB_NOTES"};
                std::ifstream fin {lab_notes_file};

                while (fin.good()) {
                    ch = fin.get();
                    if (ch == '%')
                        notes++;
                }

                while (lines < 23 && lines < success * 5) {
                    fin.seekg(0, std::ios::beg);
                    int choice = rng(1, notes);
                    while (choice > 0) {
                        getline(fin, tmp);
                        if (tmp.find_first_of('%') == 0)
                            choice--;
                    }
                    do {
                        getline(fin, tmp);
                        if (lines < 23 && lines < success * 5 && tmp.find_first_of('%') != 0) {
                            log.append(tmp);
                            log.append("\n");
                        }
                    } while (tmp.find_first_of('%') != 0);
                }
                full_screen_popup(log.c_str());
            } else if (one_in(4)) {
                add_msg("ERROR - OS CORRUPTED!");
                m.ter(x, y) = t_computer_broken;
            } else
                add_msg("Access denied.");
            break;
        case 2:
            if (success >= 0) {
                int minx = int(levx / 2) - 20 - success * 15;
                int maxx = int(levx / 2) + 20 + success * 15;
                int miny = int(levy / 2) - 20 - success * 15;
                int maxy = int(levy / 2) + 20 + success * 15;
                if (minx < 0)
                    minx = 0;
                if (maxx >= OMAPX)
                    maxx = OMAPX - 1;
                if (miny < 0)
                    miny = 0;
                if (maxy >= OMAPY)
                    maxy = OMAPY - 1;
                overmap tmp(this, cur_om.posx, cur_om.posy, 0);
                for (int i = minx; i <= maxx; i++) {
                    for (int j = miny; j <= maxy; j++)
                        tmp.seen(i, j) = true;
                }
                tmp.save(cur_om.posx, cur_om.posy, 0);
                add_msg("Surface map data downloaded.");
            } else {
                add_msg("Surface map data corrupted.");
                if (one_in(4)) {
                    m.ter(x, y) = t_computer_broken;
                    add_msg("The computer breaks down!");
                }
            }
            break;
        }
        break;

    default:
        break;
    }
}

void Game::resonance_cascade(int x, int y)
{
    int maxglow = 100 - 5 * trig_dist(x, y, u.posx, u.posy);
    int minglow = 60 - 5 * trig_dist(x, y, u.posx, u.posy);
    mon_id spawn;
    Monster invader;
    if (minglow < 0)
        minglow = 0;
    if (maxglow > 0)
        u.add_disease(DI_TELEGLOW, rng(minglow, maxglow) * 100, this);
    int startx = (x < 8 ? 0 : x - 8), endx = (x + 8 >= SEEX * 3 ? SEEX * 3 - 1 : x + 8);
    int starty = (y < 8 ? 0 : y - 8), endy = (y + 8 >= SEEY * 3 ? SEEY * 3 - 1 : y + 8);
    for (int i = startx; i <= endx; i++) {
        for (int j = starty; j <= endy; j++) {
            switch (rng(1, 80)) {
            case 1:
            case 2:
                emp_blast(i, j);
                break;
            case 3:
            case 4:
            case 5:
                for (int k = i - 1; k <= i + 1; k++) {
                    for (int l = j - 1; l <= j + 1; l++) {
                        auto type {field_id::fd_null};

                        switch (rng(1, 7)) {
                        case 1:
                            type = fd_blood;
                            break;

                        case 2:
                            type = fd_bile;
                            break;

                        case 3:
                        case 4:
                            type = fd_slime;
                            break;

                        case 5:
                            type = fd_fire;
                            break;

                        case 6:
                        case 7:
                            type = fd_nuke_gas;
                            break;
                        }

                        if (m.field_at(k, l).type == fd_null || !one_in(3))
                            m.field_at(k, l) = field(type, 3, 0);
                    }
                }
                break;
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
                m.tr_at(i, j) = tr_portal;
                break;
            case 11:
            case 12:
                m.tr_at(i, j) = tr_goo;
                break;
            case 13:
            case 14:
            case 15:
                spawn = moncats[mcat_nether][rng(0, moncats[mcat_nether].size() - 1)];
                invader = Monster(mtypes[spawn], i, j);
                z.push_back(invader);
                break;
            case 16:
            case 17:
            case 18:
                m.destroy(this, i, j, true);
                break;
            case 19:
                explosion(i, j, rng(1, 10), rng(0, 1) * rng(0, 6), one_in(4));
                break;
            }
        }
    }
}

void Game::emp_blast(int x, int y)
{
    int rn;
    if (m.has_flag(computer, x, y)) {
        add_msg("The %s is rendered non-functional!", m.tername(x, y).c_str());
        m.ter(x, y) = t_computer_broken;
        return;
    }

    // TODO: More terrain effects.
    switch (m.ter(x, y)) {
    case t_card_reader:
        rn = rng(1, 100);
        if (rn > 92 || rn < 50) {
            add_msg("The card reader is rendered non-functional.");
            m.ter(x, y) = t_card_reader_broken;
        }
        if (rn > 90) {
            add_msg("The nearby doors slide open!");
            for (int i = -5; i <= 5; i++) {
                for (int j = -5; j <= 5; j++) {
                    if (m.ter(x + i, y + j) == t_door_metal_locked)
                        m.ter(x + i, y + j) = t_floor;
                }
            }
        }
        break;

    default:
        break;
    }

    int mondex = mon_at(x, y);
    if (mondex != -1) {
        if (z[mondex].has_flag(MF_ELECTRONIC)) {
            add_msg("The EMP blast fries the %s!", z[mondex].name().c_str());
            int dam = dice(10, 10);
            if (z[mondex].hurt(dam))
                kill_mon(mondex);
        } else
            add_msg("The %s is unaffected by the EMP blast.", z[mondex].name().c_str());
    }
    if (u.posx == x && u.posy == y) {
        if (u.power_level > 0) {
            add_msg("The EMP blast drains your power.");
            u.charge_power(rng(-20, -5));
        }
        // TODO: More effects?
    }

    // Drain any items of their battery charge.
    for (auto& item : m.i_at(x, y)) {
        if (item.is_tool() && dynamic_cast<it_tool*>(item.type)->ammo == ammotype::AT_BATT) {
            item.charges = 0;
        }
    }

    // TODO: Drain NPC energy reserves
}

int Game::npc_at(int x, int y)
{
    for (std::size_t i {0}; i < active_npc.size(); ++i) {
        if (active_npc[i].posx == x && active_npc[i].posy == y) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int Game::mon_at(int x, int y)
{
    for (std::size_t i {0}; i < z.size(); ++i) {
        if (z[i].posx == x && z[i].posy == y) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void Game::kill_mon(int index)
{
    if (index < 0 || index >= static_cast<int>(z.size())) {
        debugmsg("Tried to kill monster %d! (%d in play)", index, z.size());
        return;
    }

    if (z[index].dead)
        return;
    z[index].dead = true;
    kills[z[index].type->id]++; // Increment our kill counter

    for (auto& npc : active_npc) {
        if (npc.target == &z[index]) {
            npc.target = nullptr;
        }
    }

    z[index].die(this);
    // If they left a corpse, give a tutorial message on butchering
    if (in_tutorial && !(tutorials_seen[LESSON_BUTCHER])) {
        for (const auto& item : m.i_at(z[index].posx, z[index].posy)) {
            if (item.type->id == itype_id::itm_corpse) {
                tutorial_message(tut_lesson::LESSON_BUTCHER);
                break;
            }
        }
    }
    z.erase(z.begin() + index);
    if (last_target == index)
        last_target = -1;
    else if (last_target > index)
        last_target--;
}

void Game::open()
{
    u.moves -= 100;
    bool didit = false;
    mvwprintw(w_terrain, 0, 0, "Open where? (hjklyubn) ");
    wrefresh(w_terrain);
    int openx, openy;
    get_direction(openx, openy, input());
    if (openx != -2 && openy != -2)
        if (m.ter(u.posx, u.posy) == t_floor)
            didit = m.open_door(u.posx + openx, u.posy + openy, true);
        else
            didit = m.open_door(u.posx + openx, u.posy + openy, false);
    else
        add_msg("Invalid direction.");
    if (!didit) {
        switch (m.ter(u.posx + openx, u.posy + openy)) {
        case t_door_locked:
            add_msg("The door is locked!");
            break;
        case t_door_o:
            add_msg("That door is already open.");
            u.moves += 100;
            break;
        default:
            add_msg("No door there.");
            u.moves += 100;
        }
    }
    if (in_tutorial)
        tutorial_message(LESSON_CLOSE);
}

void Game::close()
{
    bool didit = false;
    mvwprintw(w_terrain, 0, 0, "Close where? (hjklyubn) ");
    wrefresh(w_terrain);
    int closex, closey;
    get_direction(closex, closey, input());
    if (closex != -2 && closey != -2) {
        closex += u.posx;
        closey += u.posy;
        if (mon_at(closex, closey) != -1)
            add_msg("There's a %s in the way!", z[mon_at(closex, closey)].name().c_str());
        else if (m.i_at(closex, closey).size() > 0)
            add_msg("There's %s in the way!",
                    m.i_at(closex, closey).size() == 1 ? m.i_at(closex, closey)[0].tname().c_str()
                                                       : "some stuff");
        else
            didit = m.close_door(closex, closey);
    } else
        add_msg("Invalid direction.");
    if (didit)
        u.moves -= 90;
    if (in_tutorial)
        tutorial_message(LESSON_SMASH);
}

void Game::smash()
{
    bool didit = false;
    std::string bashsound, extra;
    int smashskill = int(u.str_cur / 2 + u.weapon.type->melee_dam);
    mvwprintw(w_terrain, 0, 0, "Smash what? (hjklyubn) ");
    wrefresh(w_terrain);
    char ch = input();
    if (ch == KEY_ESCAPE) {
        add_msg("Never mind.");
        return;
    }
    int smashx, smashy;
    get_direction(smashx, smashy, ch);
    if (smashx != -2 && smashy != -2)
        didit = m.bash(u.posx + smashx, u.posy + smashy, smashskill, bashsound);
    else
        add_msg("Invalid direction.");
    if (didit) {
        if (extra != "")
            add_msg(extra.c_str());
        sound(u.posx, u.posy, 18, bashsound);
        u.moves -= 80;
        if (u.sklevel[sk_melee] == 0)
            u.practice(sk_melee, rng(0, 1) * rng(0, 1));
        if (u.weapon.made_of(GLASS) && rng(0, u.weapon.volume() + 8) < u.weapon.volume()) {
            add_msg("Your %s shatters!", u.weapon.tname().c_str());

            for (const auto& content : u.weapon.contents) {
                m.add_item(u.posx, u.posy, content);
            }

            sound(u.posx, u.posy, 16, "");
            u.hit(this, bp_hands, 1, 0, rng(0, u.weapon.volume()));
            if (u.weapon.volume() > 20) // Hurt left arm too, if it was big
                u.hit(this, bp_hands, 0, 0, rng(0, u.weapon.volume() * .5));
            u.remove_weapon();
        }
    } else
        add_msg("There's nothing there!");
}

void Game::use_item()
{
    char ch = inv("Use item:");
    auto tut {itype_id::itm_null};

    if (in_tutorial)
        tut = itype_id(u.i_at(ch).type->id);
    u.use(this, ch);
    if (in_tutorial) {
        if (tut == itm_grenade)
            tutorial_message(LESSON_ACT_GRENADE);
        else if (tut == itm_bubblewrap)
            tutorial_message(LESSON_ACT_BUBBLEWRAP);
    }
}

void Game::examine()
{
    mvwprintw(w_terrain, 0, 0, "Examine where? (hjklyubn) ");
    wrefresh(w_terrain);
    int examx, examy;
    char ch = input();
    if (ch == KEY_ESCAPE || ch == 'e' || ch == 'q')
        return;
    get_direction(examx, examy, ch);
    if (examx == -2 || examy == -2) {
        add_msg("Invalid direction.");
        return;
    }
    examx += u.posx;
    examy += u.posy;
    add_msg("That is a %s.", m.tername(examx, examy).c_str());
    if (m.has_flag(sealed, examx, examy)) {
        if (m.trans(examx, examy)) {
            std::string buff;
            if (m.i_at(examx, examy).size() <= 3 && m.i_at(examx, examy).size() != 0) {
                buff = "It contains ";

                for (std::size_t i {0}; i < m.i_at(examx, examy).size(); ++i) {
                    buff += m.i_at(examx, examy)[i].tname();

                    if (i + 2 < m.i_at(examx, examy).size()) {
                        buff += ", ";
                    } else if (i + 1 < m.i_at(examx, examy).size()) {
                        buff += ", and ";
                    }
                }

                buff += ",";
            } else if (m.i_at(examx, examy).size() != 0)
                buff = "It contains many items,";
            buff += " but is firmly sealed.";
            add_msg(buff.c_str());
        } else {
            add_msg("There's something in there, but you can't see what it is, and the\
 %s is firmly sealed.",
                    m.tername(examx, examy).c_str());
        }
    } else {
        if (in_tutorial && m.i_at(examx, examy).size() == 0)
            tutorial_message(LESSON_INTERACT);
        if (m.i_at(examx, examy).size() == 0
            && !(m.has_flag(swimmable, examx, examy) || m.ter(examx, examy) == t_toilet))
            add_msg("It is empty.");
        else
            pickup(examx, examy, 0);
    }
    if (m.has_flag(computer, examx, examy)) {
        use_computer(examx, examy);
        return;
    }
    if (m.ter(examx, examy) == t_card_reader) {
        if (u.has_amount(itm_card_id, 1) && query_yn("Swipe your ID card?")) {
            u.moves -= 100;
            for (int i = -5; i <= 5; i++) {
                for (int j = -5; j <= 5; j++) {
                    if (m.ter(examx + i, examy + j) == t_door_metal_locked)
                        m.ter(examx + i, examy + j) = t_floor;
                }
            }
            add_msg("You insert your ID card.");
            add_msg("The nearby doors slide into the floor.");
            u.use_up(itm_card_id, 1);
        }
        bool using_electrohack
            = (u.has_amount(itm_electrohack, 1) && query_yn("Use electrohack on the reader?"));
        bool using_fingerhack = (!using_electrohack && u.has_bionic(bio_fingerhack)
                                 && u.power_level > 0 && query_yn("Use fingerhack on the reader?"));
        if (using_electrohack || using_fingerhack) {
            u.moves -= 500;
            u.practice(sk_computer, 20);
            int success = rng(u.sklevel[sk_computer] / 4 - 2, u.sklevel[sk_computer] * 2);
            success += rng(-3, 3);
            if (using_fingerhack)
                success++;
            if (u.int_cur < 8)
                success -= rng(0, int((8 - u.int_cur) / 2));
            else if (u.int_cur > 8)
                success += rng(0, int((u.int_cur - 8) / 2));
            if (success < 0) {
                add_msg("You cause a short circuit!");
                if (success <= -5) {
                    if (using_electrohack) {
                        add_msg("Your electrohack is ruined!");
                        u.use_up(itm_electrohack, 1);
                    } else {
                        add_msg("Your power is drained!");
                        u.charge_power(0 - rng(0, u.power_level));
                    }
                }
                m.ter(examx, examy) = t_card_reader_broken;
            } else if (success < 6)
                add_msg("Nothing happens.");
            else {
                add_msg("You activate the panel!");
                add_msg("The nearby doors slide into the floor.");
                for (int i = -5; i <= 5; i++) {
                    for (int j = -5; j <= 5; j++) {
                        if (m.ter(examx + i, examy + j) == t_door_metal_locked)
                            m.ter(examx + i, examy + j) = t_floor;
                    }
                }
            }
        }
    } else if (m.ter(examx, examy) == t_gas_pump && query_yn("Pump gas?")) {
        char ch = inv("Select container or tool to hold gas:");
        Item* cont = &(u.i_at(ch));

        if (cont->is_tool() && (dynamic_cast<it_tool*>(cont->type))->ammo == AT_GAS) {
            add_msg("You fill your %s with gasoline.", cont->tname().c_str());
            cont->curammo = dynamic_cast<it_ammo*>(itypes[itm_gasoline]);
            cont->charges = (dynamic_cast<it_tool*>(cont->type))->max_charges;
        } else if (cont->type->id == itm_null)
            add_msg("Never mind.");
        else if (!cont->is_container())
            add_msg("That %s won't hold gas.", cont->tname().c_str());
        else if (!cont->contents.empty())
            add_msg("Your %s is not empty.", cont->tname().c_str());
        else {
            u.moves -= 400;
            Item gas(itypes[itm_gasoline], turn);
            if (one_in(u.dex_cur)) {
                add_msg("You accidentally spill the gasoline.");
                m.add_item(u.posx, u.posy, gas);
            } else {
                add_msg("You fill your %s with gasoline.", cont->tname().c_str());
                cont->put_in(gas);
            }
        }
    } else if (m.ter(examx, examy) == t_slot_machine) {
        if (u.cash > 0 && query_yn("Insert $1?")) {
            do {
                if (one_in(5))
                    popup("Three cherries... you get your coin back!");
                else if (one_in(20)) {
                    popup("Three bells... you win $5!");
                    u.cash += 4; // Minus the $1 we wagered
                } else if (one_in(50)) {
                    popup("Three stars... you win $20!");
                    u.cash += 19;
                } else if (one_in(1000)) {
                    popup("JACKPOT!  You win $500!");
                    u.cash += 499;
                } else {
                    popup("No win.");
                    u.cash -= 1;
                }
            } while (u.cash > 0 && query_yn("Play again?"));
        }
    } else if (m.ter(examx, examy) == t_bulletin) {
        switch (menu("Bulletin Board", "Check jobs", "Check events", "Check other notices",
                     "Post notice", "Cancel", NULL)) {
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        }
    }
    if (m.tr_at(examx, examy) != tr_null
        && u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(examx, examy)]->visibility
        && query_yn("There is a %s there.  Disarm?", traps[m.tr_at(examx, examy)]->name.c_str()))
        m.disarm_trap(this, examx, examy);
}

void Game::look_around()
{
    int lx = u.posx, ly = u.posy;
    int mx, my, junk;
    char ch;
    WINDOW* w_look = newwin(13, 48, 12, SEEX * 2 + 8);
    wborder(w_look, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO,
            LINE_XOOX);
    mvwprintz(w_look, 1, 1, c_white, "Looking Around");
    mvwprintz(w_look, 2, 1, c_white, "Use directional keys to move the cursor");
    mvwprintz(w_look, 3, 1, c_white, "to a nearby square.");
    wrefresh(w_look);
    do {
        ch = input();
        if (!u_see(lx, ly, junk))
            mvwputch(w_terrain, ly - u.posy + SEEY, lx - u.posx + SEEX, c_black, ' ');
        draw_ter();
        get_direction(mx, my, ch);
        if (mx != -2 && my != -2) { // Directional key pressed
            lx += mx;
            ly += my;
            if (lx < u.posx - SEEX)
                lx = u.posx - SEEX;
            if (lx > u.posx + SEEX)
                lx = u.posx + SEEX;
            if (ly < u.posy - SEEY)
                ly = u.posy - SEEY;
            if (ly > u.posy + SEEY)
                ly = u.posy + SEEY;
        }
        for (int i = 1; i < 12; i++) {
            for (int j = 1; j < 46; j++)
                mvwputch(w_look, i, j, c_white, ' ');
        }
        if (u_see(lx, ly, junk)) {
            if (m.move_cost(lx, ly) == 0)
                mvwprintw(w_look, 1, 1, "%s; Impassable", m.tername(lx, ly).c_str());
            else
                mvwprintw(w_look, 1, 1, "%s; Movement cost %d", m.tername(lx, ly).c_str(),
                          m.move_cost(lx, ly) * 50);
            mvwprintw(w_look, 2, 1, "%s", m.features(lx, ly).c_str());
            field tmpfield = m.field_at(lx, ly);

            if (tmpfield.type != field_id::fd_null) {
                mvwprintz(w_look, 4, 1, fieldlist.at(tmpfield.type).color[tmpfield.density - 1],
                          "%s", fieldlist.at(tmpfield.type).name[tmpfield.density - 1].c_str());
            }

            if (m.tr_at(lx, ly) != tr_null
                && u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(lx, ly)]->visibility)
                mvwprintz(w_look, 5, 1, traps[m.tr_at(lx, ly)]->color, "%s",
                          traps[m.tr_at(lx, ly)]->name.c_str());
            int dex = mon_at(lx, ly);
            if (dex != -1 && u_see(&(z[dex]), junk)) {
                z[mon_at(lx, ly)].draw(w_terrain, u.posx, u.posy, true);
                z[mon_at(lx, ly)].print_info(this, w_look);
                if (m.i_at(lx, ly).size() > 1)
                    mvwprintw(w_look, 3, 1, "There are several items there.");
                else if (m.i_at(lx, ly).size() == 1)
                    mvwprintw(w_look, 3, 1, "There is an item there.");
            } else if (npc_at(lx, ly) != -1) {
                active_npc[npc_at(lx, ly)].draw(w_terrain, u.posx, u.posy, true);
                active_npc[npc_at(lx, ly)].print_info(w_look);
                if (m.i_at(lx, ly).size() > 1)
                    mvwprintw(w_look, 3, 1, "There are several items there.");
                else if (m.i_at(lx, ly).size() == 1)
                    mvwprintw(w_look, 3, 1, "There is an item there.");
            } else if (m.i_at(lx, ly).size() > 0) {
                mvwprintw(w_look, 3, 1, "There is a %s there.", m.i_at(lx, ly)[0].tname().c_str());
                if (m.i_at(lx, ly).size() > 1)
                    mvwprintw(w_look, 4, 1, "There are other items there as well.");
                m.drawsq(w_terrain, u, lx, ly, true, true);
            } else
                m.drawsq(w_terrain, u, lx, ly, true, true);

        } else if (lx == u.posx && ly == u.posy) {
            mvwputch_inv(w_terrain, SEEX, SEEY, u.color(), '@');
            mvwprintw(w_look, 1, 1, "You (%s)", u.name.c_str());
        } else {
            mvwputch(w_terrain, ly - u.posy + SEEY, lx - u.posx + SEEX, c_white, 'x');
            mvwprintw(w_look, 1, 1, "Unseen.");
        }
        wrefresh(w_look);
        wrefresh(w_terrain);
    } while (ch != ' ' && ch != KEY_ESCAPE && ch != ';' && ch != '\n');
}

// Pick up items at (posx, posy).
void Game::pickup(int posx, int posy, int min)
{
    write_msg();
    if (u.weapon.type->id == itm_bio_claws) {
        add_msg("You cannot pick up items with your claws out!");
        return;
    }
    bool weight_is_okay = (u.weight_carried() <= u.weight_capacity() * .25);
    bool volume_is_okay = (u.volume_carried() <= u.volume_capacity() - 2);
    // Picking up water?
    if (m.i_at(posx, posy).size() == 0) {
        if (m.has_flag(swimmable, posx, posy) || m.ter(posx, posy) == t_toilet) {
            char ch = inv("Select container to hold water:");
            Item* cont = &(u.i_at(ch));
            if (cont->type->id == itm_null)
                return;
            else if (!cont->is_container())
                add_msg("That %s won't hold water.", cont->tname().c_str());
            else if (!cont->contents.empty())
                add_msg("Your %s is not empty.", cont->tname().c_str());
            else {
                Item water = m.water_from(posx, posy);
                it_container* cont_type = dynamic_cast<it_container*>(cont->type);

                if ((cont_type->flags & flag_to_bit_position(container_flags::con_wtight)) == 0U) {
                    add_msg("That %s isn't watertight!", cont->tname().c_str());
                    return;
                }

                if ((cont_type->flags & flag_to_bit_position(container_flags::con_seals)) == 0U) {
                    if (query_yn("Can't transport water in a %s. Drink it now?",
                                 cont->tname().c_str())) {
                        cont->put_in(water);
                        u.eat(this, cont->invlet);
                    }
                } else {
                    add_msg("You fill your %s with water.", cont->tname().c_str());
                    cont->put_in(water);
                }
            }
        }
        return;
        // Few item here, just get it
    }

    if (static_cast<int>(m.i_at(posx, posy).size()) <= min) {
        int iter = 0;
        Item newit = m.i_at(posx, posy)[0];
        if (newit.made_of(LIQUID)) {
            add_msg("You can't pick up a liquid!");
            return;
        }
        while ((newit.invlet == 0 || u.has_item(newit.invlet)) && iter < 52) {
            newit.invlet = nextinv;
            iter++;
            advance_nextinv();
        }
        if (iter == 52) {
            add_msg("You're carrying too many items!");
            return;
        }

        if (u.weight_carried() + static_cast<int>(newit.weight()) > u.weight_capacity()) {
            add_msg("The %s is too heavy!", newit.tname().c_str());
            --nextinv;
        } else if (u.volume_carried() + static_cast<int>(newit.volume()) > u.volume_capacity()) {
            if (u.is_armed()) {
                if (u.weapon.type->id < num_items && // Not a bionic
                    query_yn("Drop your %s and pick up %s?", u.weapon.tname().c_str(),
                             newit.tname().c_str())) {
                    m.i_clear(posx, posy);
                    m.add_item(posx, posy, u.remove_weapon());
                    u.i_add(newit);
                    u.wield(this, newit.invlet);
                    u.moves -= 100;
                    add_msg("Wielding %c - %s", newit.invlet, newit.tname().c_str());
                    if (in_tutorial) {
                        tutorial_message(LESSON_FULL_INV);
                        if (newit.is_armor())
                            tutorial_message(LESSON_GOT_ARMOR);
                        else if (newit.is_gun())
                            tutorial_message(LESSON_GOT_GUN);
                        else if (newit.is_weap())
                            tutorial_message(LESSON_GOT_WEAPON);
                        else if (newit.is_ammo())
                            tutorial_message(LESSON_GOT_AMMO);
                        else if (newit.is_tool())
                            tutorial_message(LESSON_GOT_TOOL);
                        else if (newit.is_food() || newit.is_food_container())
                            tutorial_message(LESSON_GOT_FOOD);
                    }
                } else
                    nextinv--;
            } else {
                u.i_add(newit);
                u.wield(this, newit.invlet);
                m.i_clear(posx, posy);
                u.moves -= 100;
                add_msg("Wielding %c - %s", newit.invlet, newit.tname().c_str());
                if (in_tutorial) {
                    tutorial_message(LESSON_WIELD_NO_SPACE);
                    if (newit.is_armor())
                        tutorial_message(LESSON_GOT_ARMOR);
                    else if (newit.is_gun())
                        tutorial_message(LESSON_GOT_GUN);
                    else if (newit.is_ammo())
                        tutorial_message(LESSON_GOT_AMMO);
                    else if (newit.is_tool())
                        tutorial_message(LESSON_GOT_TOOL);
                    else if (newit.is_food() || newit.is_food_container())
                        tutorial_message(LESSON_GOT_FOOD);
                }
            }
        } else if (!u.is_armed()
                   && (u.volume_carried() + static_cast<int>(newit.volume())
                           > u.volume_capacity() - 2
                       || newit.is_weap())) {
            u.weapon = newit;
            m.i_clear(posx, posy);
            u.moves -= 100;
            add_msg("Wielding %c - %s", newit.invlet, newit.tname().c_str());
            if (in_tutorial) {
                if (newit.is_weap())
                    tutorial_message(LESSON_AUTOWIELD);
                else
                    tutorial_message(LESSON_WIELD_NO_SPACE);
                if (newit.is_armor())
                    tutorial_message(LESSON_GOT_ARMOR);
                else if (newit.is_gun())
                    tutorial_message(LESSON_GOT_GUN);
                else if (newit.is_ammo())
                    tutorial_message(LESSON_GOT_AMMO);
                else if (newit.is_tool())
                    tutorial_message(LESSON_GOT_TOOL);
                else if (newit.is_food() || newit.is_food_container())
                    tutorial_message(LESSON_GOT_FOOD);
            }
        } else {
            u.i_add(newit);
            m.i_clear(posx, posy);
            u.moves -= 100;
            add_msg("%c - %s", newit.invlet, newit.tname().c_str());
            if (in_tutorial) {
                tutorial_message(LESSON_ITEM_INTO_INV);
                if (newit.is_armor())
                    tutorial_message(LESSON_GOT_ARMOR);
                else if (newit.is_gun())
                    tutorial_message(LESSON_GOT_GUN);
                else if (newit.is_weap())
                    tutorial_message(LESSON_GOT_WEAPON);
                else if (newit.is_ammo())
                    tutorial_message(LESSON_GOT_AMMO);
                else if (newit.is_tool())
                    tutorial_message(LESSON_GOT_TOOL);
                else if (newit.is_food() || newit.is_food_container())
                    tutorial_message(LESSON_GOT_FOOD);
            }
        }
        if (weight_is_okay && u.weight_carried() >= u.weight_capacity() * .25)
            add_msg("You're overburdened!");
        if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
            add_msg("You struggle to carry such a large volume!");
            if (in_tutorial)
                tutorial_message(LESSON_OVERLOADED);
        }
        return;
    }
    // Otherwise, we have 2 or more items and should list them, etc.
    WINDOW* w_pickup = newwin(12, 48, 0, SEEX * 2 + 8);
    WINDOW* w_item_info = newwin(12, 48, 12, SEEX * 2 + 8);
    int maxitems = 9; // Number of items to show at one time.
    std::vector<Item> here = m.i_at(posx, posy);
    std::vector<bool> getitem(here.size());

    for (std::size_t i {0}; i < here.size(); ++i) {
        getitem[i] = false;
    }

    int ch {' '};
    int start = 0, cur_it, iter;
    int new_weight = u.weight_carried(), new_volume = u.volume_carried();
    bool update = true;
    mvwprintw(w_pickup, 0, 0, "PICK UP");
    // Now print the two lists; those on the ground and about to be added to inv
    // Continue until we hit return or space
    do {
        for (int i = 1; i < 12; i++) {
            for (int j = 0; j < 48; j++)
                mvwaddch(w_pickup, i, j, ' ');
        }
        if (ch == '<' && start > 0) {
            start -= maxitems;
            mvwprintw(w_pickup, maxitems + 2, 0, "         ");
        }

        if (ch == '>' && start + maxitems < static_cast<int>(here.size())) {
            start += maxitems;
            mvwprintw(w_pickup, maxitems + 2, 12, "            ");
        }

        if (ch >= 'a' && ch <= 'a' + static_cast<int>(here.size()) - 1) {
            ch -= 'a';
            getitem[ch] = !getitem[ch];
            if (getitem[ch]) {
                mvwprintw(w_item_info, 1, 0, "%s", here[ch].info().c_str());
                wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO,
                        LINE_OOXX, LINE_XXOO, LINE_XOOX);
                wrefresh(w_item_info);
                new_weight += here[ch].weight();
                new_volume += here[ch].volume();
                update = true;
            } else {
                wclear(w_item_info);
                wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO,
                        LINE_OOXX, LINE_XXOO, LINE_XOOX);
                wrefresh(w_item_info);
                new_weight -= here[ch].weight();
                new_volume -= here[ch].volume();
                update = true;
            }
        }

        if (ch == ',' || ch == 'g') {
            int count = 0;

            for (std::size_t i {0}; i < here.size(); ++i) {
                if (getitem[i]) {
                    ++count;
                } else {
                    new_weight += static_cast<int>(here[i].weight());
                    new_volume += static_cast<int>(here[i].volume());
                }

                getitem[i] = true;
            }

            if (count == static_cast<int>(here.size())) {
                for (std::size_t i {0}; i < here.size(); ++i) {
                    getitem[i] = false;
                }

                new_weight = u.weight_carried();
                new_volume = u.volume_carried();
            }

            update = true;
        }
        for (cur_it = start; cur_it < start + maxitems; cur_it++) {
            mvwprintw(w_pickup, 1 + (cur_it % maxitems), 0,
                      "                                        ");

            if (cur_it < static_cast<int>(here.size())) {
                mvwputch(w_pickup, 1 + (cur_it % maxitems), 0, c_white, char(cur_it + 'a'));
                if (getitem[cur_it])
                    wprintw(w_pickup, " + ");
                else
                    wprintw(w_pickup, " - ");

                wprintw(w_pickup, "%s", here[cur_it].tname().c_str());

                if (here[cur_it].charges > 0)
                    wprintw(w_pickup, " (%d)", here[cur_it].charges);
            }
        }
        if (start > 0)
            mvwprintw(w_pickup, maxitems + 2, 0, "< Go Back");

        if (cur_it < static_cast<int>(here.size())) {
            mvwprintw(w_pickup, maxitems + 2, 12, "> More items");
        }

        if (update) { // Update weight & volume information
            update = false;
            mvwprintw(w_pickup, 0, 7, "                           ");
            mvwprintz(w_pickup, 0, 9, (new_weight >= u.weight_capacity() * .25 ? c_red : c_white),
                      "Wgt %d", new_weight);
            wprintz(w_pickup, c_white, "/%d", u.weight_capacity());
            mvwprintz(w_pickup, 0, 22, (new_volume > u.volume_capacity() - 2 ? c_red : c_white),
                      "Vol %d", new_volume);
            wprintz(w_pickup, c_white, "/%d", u.volume_capacity() - 2);
        }
        wrefresh(w_pickup);
        ch = getch();
    } while (ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
    if (ch == KEY_ESCAPE) {
        werase(w_pickup);
        wrefresh(w_pickup);
        delwin(w_pickup);
        return;
    }
    // At this point we've selected our items, now we add them to our inventory
    int curmit = 0;
    bool got_water = false; // Did we try to pick up water?

    for (std::size_t i {0}; i < here.size(); ++i) {
        iter = 0;
        // This while loop guarantees the inventory letter won't be a repeat. If it
        // tries all 52 letters, it fails and we don't pick it up.
        if (getitem[i] && here[i].made_of(LIQUID))
            got_water = true;
        else if (getitem[i]) {
            while ((here[i].invlet == 0 || u.has_item(here[i].invlet)) && iter < 52) {
                here[i].invlet = nextinv;
                advance_nextinv();
                iter++;
            }
            if (iter == 52) {
                add_msg("You're carrying too many items!");
                werase(w_pickup);
                wrefresh(w_pickup);
                delwin(w_pickup);
                return;
            }

            if (u.weight_carried() + static_cast<int>(here[i].weight()) > u.weight_capacity()) {
                add_msg("The %s is too heavy!", here[i].tname().c_str());
                --nextinv;
            } else if (u.volume_carried() + static_cast<int>(here[i].volume())
                       > u.volume_capacity()) {
                if (u.is_armed()) {
                    if (u.weapon.type->id < num_items && // Not a bionic
                        query_yn("Drop your %s and pick up %s?", u.weapon.tname().c_str(),
                                 here[i].tname().c_str())) {
                        m.add_item(posx, posy, u.remove_weapon());
                        u.i_add(here[i]);
                        u.wield(this, here[i].invlet);
                        m.i_rem(posx, posy, curmit);
                        u.moves -= 100;
                        curmit--;
                        if (in_tutorial) {
                            tutorial_message(LESSON_WIELD_NO_SPACE);
                            if (here[i].is_armor())
                                tutorial_message(LESSON_GOT_ARMOR);
                            else if (here[i].is_gun())
                                tutorial_message(LESSON_GOT_GUN);
                            else if (here[i].is_weap())
                                tutorial_message(LESSON_GOT_WEAPON);
                            else if (here[i].is_ammo())
                                tutorial_message(LESSON_GOT_AMMO);
                            else if (here[i].is_tool())
                                tutorial_message(LESSON_GOT_TOOL);
                            else if (here[i].is_food() || here[i].is_food_container())
                                tutorial_message(LESSON_GOT_FOOD);
                        }
                    } else
                        nextinv--;
                } else {
                    u.i_add(here[i]);
                    u.wield(this, here[i].invlet);
                    m.i_rem(posx, posy, curmit);
                    curmit--;
                    u.moves -= 100;
                    if (in_tutorial) {
                        tutorial_message(LESSON_WIELD_NO_SPACE);
                        if (here[i].is_armor())
                            tutorial_message(LESSON_GOT_ARMOR);
                        else if (here[i].is_gun())
                            tutorial_message(LESSON_GOT_GUN);
                        else if (here[i].is_weap())
                            tutorial_message(LESSON_GOT_WEAPON);
                        else if (here[i].is_ammo())
                            tutorial_message(LESSON_GOT_AMMO);
                        else if (here[i].is_tool())
                            tutorial_message(LESSON_GOT_TOOL);
                        else if (here[i].is_food() || here[i].is_food_container())
                            tutorial_message(LESSON_GOT_FOOD);
                    }
                }
            } else if (!u.is_armed()
                       && (u.volume_carried() + static_cast<int>(here[i].volume())
                               > u.volume_capacity() - 2
                           || here[i].is_weap())) {
                u.weapon = here[i];
                m.i_rem(posx, posy, curmit);
                u.moves -= 100;
                curmit--;
                if (in_tutorial) {
                    if (here[i].is_weap())
                        tutorial_message(LESSON_AUTOWIELD);
                    else
                        tutorial_message(LESSON_WIELD_NO_SPACE);
                    if (here[i].is_armor())
                        tutorial_message(LESSON_GOT_ARMOR);
                    else if (here[i].is_gun())
                        tutorial_message(LESSON_GOT_GUN);
                    else if (here[i].is_weap())
                        tutorial_message(LESSON_GOT_WEAPON);
                    else if (here[i].is_ammo())
                        tutorial_message(LESSON_GOT_AMMO);
                    else if (here[i].is_tool())
                        tutorial_message(LESSON_GOT_TOOL);
                    else if (here[i].is_food() || here[i].is_food_container())
                        tutorial_message(LESSON_GOT_FOOD);
                }
            } else {
                u.i_add(here[i]);
                m.i_rem(posx, posy, curmit);
                u.moves -= 100;
                curmit--;
                if (in_tutorial) {
                    tutorial_message(LESSON_ITEM_INTO_INV);
                    if (here[i].is_armor())
                        tutorial_message(LESSON_GOT_ARMOR);
                    else if (here[i].is_gun())
                        tutorial_message(LESSON_GOT_GUN);
                    else if (here[i].is_weap())
                        tutorial_message(LESSON_GOT_WEAPON);
                    else if (here[i].is_ammo())
                        tutorial_message(LESSON_GOT_AMMO);
                    else if (here[i].is_tool())
                        tutorial_message(LESSON_GOT_TOOL);
                    else if (here[i].is_food() || here[i].is_food_container())
                        tutorial_message(LESSON_GOT_FOOD);
                }
            }
        }
        curmit++;
    }

    if (got_water)
        add_msg("You can't pick up a liquid!");
    if (weight_is_okay && u.weight_carried() >= u.weight_capacity() * .25)
        add_msg("You're overburdened!");
    if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
        add_msg("You struggle to carry such a large volume!");
        if (in_tutorial)
            tutorial_message(LESSON_OVERLOADED);
    }
    werase(w_pickup);
    wrefresh(w_pickup);
    delwin(w_pickup);
}

void Game::drop()
{
    char ch = inv("Drop item:");
    if (ch == KEY_ESCAPE) {
        add_msg("Never mind.");
        return;
    }
    if (ch == u.weapon.invlet && u.weapon.type->id > num_items) {
        add_msg("You cannot drop your %s.", u.weapon.tname().c_str());
        return;
    }
    Item tmp = u.i_rem(ch);
    if (tmp.type->name == "none") {
        add_msg("You do not have that item.");
        return;
    }
    m.add_item(u.posx, u.posy, tmp);
    add_msg("You drop your %s.", tmp.tname().c_str());
}

// Display current inventory.
char Game::inv(std::string title)
{
    WINDOW* w_inv = newwin(25, 80, 0, 0);
    int maxitems = 20; // Number of items to show at one time.
    char ch = '.';
    int start = 0, cur_it;
    if (!u.inv_sorted)
        u.sort_inv();
    int first_gun = -1, first_ammo = -1, first_weap = -1, first_armor = -1, first_food = -1,
        first_tool = -1, first_book = -1, first_other = -1;

    for (std::size_t i {0}; i < u.inv.size(); ++i) {
        if (first_gun == -1 && u.inv[i].is_gun()) {
            first_gun = static_cast<int>(i);
        } else if (first_ammo == -1 && u.inv[i].is_ammo()) {
            first_ammo = static_cast<int>(i);
        } else if (first_armor == -1 && u.inv[i].is_armor()) {
            first_armor = static_cast<int>(i);
        } else if (first_food == -1 && (u.inv[i].is_food() || u.inv[i].is_food_container())) {
            first_food = static_cast<int>(i);
        } else if (first_tool == -1 && u.inv[i].is_tool()) {
            first_tool = static_cast<int>(i);
        } else if (first_book == -1 && u.inv[i].is_book()) {
            first_book = static_cast<int>(i);
        } else if (first_weap == -1 && u.inv[i].is_weap()) {
            first_weap = static_cast<int>(i);
        } else if (first_other == -1 && !u.inv[i].is_food() && !u.inv[i].is_ammo()
                   && !u.inv[i].is_gun() && !u.inv[i].is_armor() && !u.inv[i].is_book()
                   && !u.inv[i].is_tool() && !u.inv[i].is_weap() && !u.inv[i].is_food_container()) {
            first_other = static_cast<int>(i);
        }
    }

    mvwprintw(w_inv, 0, 0, "%s", title.c_str());
    mvwprintw(w_inv, 0, 40, "Weight: ");
    if (u.weight_carried() >= u.weight_capacity() * .25)
        wprintz(w_inv, c_red, "%d", u.weight_carried());
    else
        wprintz(w_inv, c_ltgray, "%d", u.weight_carried());
    wprintz(w_inv, c_ltgray, "/%d", u.weight_capacity());
    mvwprintw(w_inv, 0, 60, "Volume: ");
    if (u.volume_carried() > u.volume_capacity() - 2)
        wprintz(w_inv, c_red, "%d", u.volume_carried());
    else
        wprintz(w_inv, c_ltgray, "%d", u.volume_carried());
    wprintw(w_inv, "/%d", u.volume_capacity() - 2);
    mvwprintz(w_inv, 2, 40, c_magenta, "WEAPON:");
    mvwprintw(w_inv, 3, 42, "%s", u.weapname().c_str());

    if (u.is_armed())
        mvwputch(w_inv, 3, 40, c_white, u.weapon.invlet);
    if (u.worn.size() > 0)
        mvwprintz(w_inv, 5, 40, c_magenta, "ITEMS WORN:");

    for (std::size_t i {0}; i < u.worn.size(); ++i) {
        mvwputch(w_inv, 6 + static_cast<int>(i), 40, nc_color::c_white, u.worn[i].invlet);
        mvwprintw(w_inv, 6 + static_cast<int>(i), 42, " %s", u.worn[i].tname().c_str());
    }

    do {
        if (ch == '<' && start > 0) {
            for (int i = 1; i < 25; i++)
                mvwprintz(w_inv, i, 0, c_black, "                                        ");
            start -= maxitems;
            if (start < 0)
                start = 0;
            mvwprintw(w_inv, maxitems + 2, 0, "         ");
        }

        if (ch == '>' && cur_it < static_cast<int>(u.inv.size())) {
            start = cur_it;
            mvwprintw(w_inv, maxitems + 2, 12, "            ");

            for (int i {1}; i < 25; ++i) {
                mvwprintz(w_inv, i, 0, nc_color::c_black,
                          "                                        ");
            }
        }

        int cur_line = 2;
        for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
            // Clear the current line;
            mvwprintw(w_inv, cur_line, 0, "                                    ");
            // Print category header
            if (cur_it == first_gun) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "FIREARMS:");
                cur_line++;
            } else if (cur_it == first_ammo) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "AMMUNITION:");
                cur_line++;
            } else if (cur_it == first_weap) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "WEAPONS:");
                cur_line++;
            } else if (cur_it == first_armor) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "CLOTHING:");
                cur_line++;
            } else if (cur_it == first_food) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "COMESTIBLES:");
                cur_line++;
            } else if (cur_it == first_tool) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "TOOLS:");
                cur_line++;
            } else if (cur_it == first_book) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "BOOKS:");
                cur_line++;
            } else if (cur_it == first_other) {
                mvwprintz(w_inv, cur_line, 0, c_magenta, "OTHER:");
                cur_line++;
            }

            if (cur_it < static_cast<int>(u.inv.size())) {
                mvwputch(w_inv, cur_line, 0, nc_color::c_white, u.inv[cur_it].invlet);
                mvwprintw(w_inv, cur_line, 1, " %s", u.inv[cur_it].tname().c_str());

                if (u.inv[cur_it].charges > 0) {
                    wprintw(w_inv, " (%d)", u.inv[cur_it].charges);
                }
            }

            cur_line++;
        }
        if (start > 0)
            mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");

        if (cur_it < static_cast<int>(u.inv.size())) {
            mvwprintw(w_inv, maxitems + 4, 12, "> More items");
        }

        wrefresh(w_inv);
        ch = getch();
    } while (ch == '<' || ch == '>');
    werase(w_inv);
    delwin(w_inv);
    erase();
    refresh_all();
    return ch;
}

void Game::plthrow()
{
    char ch = inv("Throw item:");
    int range = u.throw_range(ch);
    if (range < 0) {
        add_msg("You don't have that item.");
        return;
    } else if (range == 0) {
        add_msg("That is too heavy to throw.");
        return;
    }
    Item thrown = u.i_at(ch);

    int x = u.posx, y = u.posy;
    int x0 = x - range;
    int y0 = y - range;
    int x1 = x + range;
    int y1 = y + range;
    int junk;

    for (int j = u.posx - SEEX; j <= u.posx + SEEX; j++) {
        for (int k = u.posy - SEEY; k <= u.posy + SEEY; k++) {
            if (u_see(j, k, junk)) {
                if (k >= y0 && k <= y1 && j >= x0 && j <= x1)
                    m.drawsq(w_terrain, u, j, k, false, true);
                else
                    mvwputch(w_terrain, k + SEEY - u.posy, j + SEEX - u.posx, c_dkgray, '#');
            }
        }
    }

    std::vector<Monster> mon_targets;
    std::vector<int> targetindices;
    int passtarget = -1;

    for (std::size_t i {0}; i < z.size(); ++i) {
        if (u_see(&z[i], junk) && z[i].posx >= x0 && z[i].posx <= x1 && z[i].posy >= y0
            && z[i].posy <= y1) {
            mon_targets.push_back(z[i]);
            targetindices.push_back(static_cast<int>(i));

            if (static_cast<char>(i) == last_target) {
                passtarget = static_cast<int>(mon_targets.size()) - 1;
            }

            z[i].draw(w_terrain, u.posx, u.posy, /*inv=*/true);
        }
    }

    // target() sets x and y, or returns false if we canceled (by pressing Esc)
    std::vector<Point> trajectory = target(x, y, x0, y0, x1, y1, mon_targets, passtarget, &thrown);
    if (trajectory.size() == 0)
        return;
    if (passtarget != -1)
        last_target = targetindices[passtarget];

    u.i_rem(ch);
    u.moves -= 125;
    u.practice(sk_throw, 10);

    throw_item(u, x, y, thrown, trajectory);
}

void Game::throw_item(player& p, int tarx, int tary, Item& thrown, std::vector<Point>& trajectory)
{
    int deviation = 0;
    int trange = 1.5 * trig_dist(p.posx, p.posy, tarx, tary);
    if (p.sklevel[sk_throw] < 8)
        deviation += rng(0, 8 - p.sklevel[sk_throw]);
    else
        deviation -= p.sklevel[sk_throw] - 6;
    if (p.per_cur < 6)
        deviation += rng(0, 8 - p.per_cur);
    else if (p.per_cur > 8)
        deviation -= p.per_cur - 8;
    if (p.dex_cur < 11)
        deviation += rng(0, 11 - p.dex_cur);
    else
        deviation -= p.dex_cur - 11;
    deviation += rng(0, p.encumb(bp_torso) + p.encumb(bp_eyes) + 1);
    if (thrown.volume() > 5)
        deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
    if (thrown.volume() == 0)
        deviation += rng(0, 3);

    deviation += rng(0, 1 + std::abs(p.str_cur - static_cast<int>(thrown.weight())));

    double missed_by = .01 * deviation * trange;
    bool missed = false;
    int tart;

    if (missed_by >= 1) {
        // We missed D:
        // Shoot a random nearby space?
        if (missed_by > 9)
            missed_by = 9;
        tarx += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
        tary += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
        if (m.sees(p.posx, p.posy, tarx, tary, -1, tart))
            trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
        else
            trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
        missed = true;
        if (!p.is_npc())
            add_msg("You miss!");
    } else if (missed_by >= .6) {
        // Hit the space, but not necessarily the monster there
        missed = true;
        if (!p.is_npc())
            add_msg("You barely miss!");
    }

    std::string message;
    int dam = (thrown.weight() + thrown.type->melee_dam + p.str_cur)
        / double(2 + double(thrown.volume() / 6));
    std::size_t index {0};
    int tx {0};
    int ty {0};

    for (index = 0; index < trajectory.size() && dam > 0; ++index) {
        message = "";
        double goodhit = missed_by;
        tx = trajectory[index].x;
        ty = trajectory[index].y;
        // If there's a monster in the path of our item, and either our aim was true,
        //  OR it's not the monster we were aiming at and we were lucky enough to hit it
        if (mon_at(tx, ty) != -1 && (!missed || one_in(7 - int(z[mon_at(tx, ty)].type->size)))) {
            if (rng(0, 100) < 20 + p.sklevel[sk_throw] * 12 && thrown.type->melee_cut > 0) {
                if (!p.is_npc()) {
                    message += " You cut the ";
                    message += z[mon_at(tx, ty)].name();
                    message += "!";
                }
                if (thrown.type->melee_cut > z[mon_at(tx, ty)].armor())
                    dam += (thrown.type->melee_cut - z[mon_at(tx, ty)].armor());
            }
            if (thrown.made_of(GLASS) && !thrown.active && // active = molotov, etc.
                rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
                if (u_see(tx, ty, tart))
                    add_msg("The %s shatters!", thrown.tname().c_str());

                for (const auto& content : thrown.contents) {
                    m.add_item(tx, ty, content);
                }

                sound(tx, ty, 16, "glass breaking!");
                int glassdam = rng(0, thrown.volume() * 2);
                if (glassdam > z[mon_at(tx, ty)].armor())
                    dam += (glassdam - z[mon_at(tx, ty)].armor());
            } else
                m.add_item(tx, ty, thrown);

            if (index < trajectory.size() - 1) {
                goodhit = static_cast<double>(rand() / RAND_MAX) / 2;
            }

            if (goodhit < .1 && !z[mon_at(tx, ty)].has_flag(MF_NOHEAD)) {
                message = "Headshot!";
                dam = rng(dam, dam * 3);
                p.practice(sk_throw, 5);
            } else if (goodhit < .2) {
                message = "Critical!";
                dam = rng(dam, dam * 2);
                p.practice(sk_throw, 2);
            } else if (goodhit < .4)
                dam = rng(int(dam / 2), int(dam * 1.5));
            else if (goodhit < .5) {
                message = "Grazing hit.";
                dam = rng(0, dam);
            }
            if (!p.is_npc())
                add_msg("%s You hit the %s for %d damage.", message.c_str(),
                        z[mon_at(tx, ty)].name().c_str(), dam);
            else if (u_see(tx, ty, tart))
                add_msg("%s hits the %s for %d damage.", z[mon_at(tx, ty)].name().c_str(), dam);
            if (z[mon_at(tx, ty)].hurt(dam))
                kill_mon(mon_at(tx, ty));
            return;
        } else // No monster hit, but the terrain might be.
            m.shoot(this, tx, ty, dam, false);
        if (m.move_cost(tx, ty) == 0) {
            tx = trajectory[index - 1].x;
            ty = trajectory[index - 1].y;
            index = trajectory.size();
        }
    }
    if (m.move_cost(tx, ty) == 0) {
        tx = trajectory[index - 2].x;
        ty = trajectory[index - 2].y;
    }
    if (thrown.made_of(GLASS) && !thrown.active && // active means molotov, etc
        rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
        if (u_see(tx, ty, tart))
            add_msg("The %s shatters!", thrown.tname().c_str());

        for (const auto& content : thrown.contents) {
            m.add_item(tx, ty, content);
        }

        sound(tx, ty, 16, "glass breaking!");
    } else {
        sound(tx, ty, 8, "thud.");
        m.add_item(tx, ty, thrown);
    }
}

void Game::plfire(bool burst)
{
    if (!u.weapon.is_gun())
        return;
    if (u.weapon.charges == 0) {
        add_msg("You need to reload!");
        return;
    }

    int junk;
    int range = u.weapon.curammo->range;
    int x = u.posx, y = u.posy;
    int x0 = x - range;
    int y0 = y - range;
    int x1 = x + range;
    int y1 = y + range;
    for (int j = x - SEEX; j <= x + SEEX; j++) {
        for (int k = y - SEEY; k <= y + SEEY; k++) {
            if (u_see(j, k, junk)) {
                if (k >= y0 && k <= y1 && j >= x0 && j <= x1)
                    m.drawsq(w_terrain, u, j, k, false, true);
                else
                    mvwputch(w_terrain, k + SEEY - y, j + SEEX - x, c_dkgray, '#');
            }
        }
    }
    // Populate a list of targets with the zombies in range and visible
    std::vector<Monster> mon_targets;
    std::vector<int> targetindices;
    int passtarget = -1;

    for (std::size_t i {0}; i < z.size(); ++i) {
        if (z[i].posx >= x0 && z[i].posx <= x1 && z[i].posy >= y0 && z[i].posy <= y1
            && z[i].friendly == 0 && u_see(&z[i], junk)) {
            mon_targets.push_back(z[i]);
            targetindices.push_back(static_cast<int>(i));

            if (static_cast<char>(i) == last_target) {
                passtarget = static_cast<int>(mon_targets.size()) - 1;
            }

            z[i].draw(w_terrain, u.posx, u.posy, /*inv=*/true);
        }
    }

    // target() sets x and y, or returns false if we canceled (by pressing Esc)
    std::vector<Point> trajectory
        = target(x, y, x0, y0, x1, y1, mon_targets, passtarget, &u.weapon);
    if (trajectory.size() == 0)
        return;
    if (passtarget != -1)
        last_target = targetindices[passtarget];

    // Train up our skill
    it_gun* firing = dynamic_cast<it_gun*>(u.weapon.type);
    int num_shots = 1;
    if (burst)
        num_shots = u.weapon.burst_size();
    if (num_shots > u.weapon.charges)
        num_shots = u.weapon.charges;
    if (u.sklevel[firing->skill_used] == 0 || firing->ammo != AT_BB)
        u.practice(firing->skill_used, 4 + num_shots);
    if (u.sklevel[sk_gun] == 0 || firing->ammo != AT_BB)
        u.practice(sk_gun, 8);

    fire(u, x, y, trajectory, burst);
    if (in_tutorial && u.recoil >= 5)
        tutorial_message(LESSON_RECOIL);
}

void Game::fire(player& p, int tarx, int tary, std::vector<Point>& trajectory, bool burst)
{
    // If we aren't wielding a loaded gun, we can't shoot!
    Item ammotmp = Item(p.weapon.curammo, 0);
    ammotmp.charges = 1;
    if (!p.weapon.is_gun()) {
        debugmsg("%s tried to fire a non-gun (%s).", p.name.c_str(), p.weapon.tname().c_str());
        return;
    }
    bool is_bolt = false;
    if (p.weapon.curammo->type == AT_BOLT) // Bolts are silent
        is_bolt = true;

    int x = p.posx, y = p.posy;
    it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
    if (p.has_trait(PF_TRIGGERHAPPY) && one_in(40))
        burst = true;
    if (burst && p.weapon.burst_size() < 2)
        burst = false; // Can't burst fire a semi-auto

    int junk = 0;
    bool u_see_shooter = u_see(p.posx, p.posy, junk);
    // Use different amounts of time depending on the type of gun and our skill
    switch (firing->skill_used) {
    case (sk_pistol):
        if (p.sklevel[sk_pistol] > 8)
            p.moves -= 10;
        else
            p.moves -= (50 - 5 * p.sklevel[sk_pistol]);
        break;
    case (sk_shotgun):
        if (p.sklevel[sk_shotgun] > 3)
            p.moves -= 70;
        else
            p.moves -= (150 - 25 * p.sklevel[sk_shotgun]);
        break;
    case (sk_smg):
        if (p.sklevel[sk_smg] > 5)
            p.moves -= 20;
        else
            p.moves -= (80 - 10 * p.sklevel[sk_smg]);
        break;
    case (sk_rifle):
        if (p.sklevel[sk_rifle] > 8)
            p.moves -= 30;
        else
            p.moves -= (150 - 15 * p.sklevel[sk_rifle]);
        break;
    default:
        debugmsg("Why is shooting %s using %s skill?", (firing->name).c_str(),
                 skill_name(firing->skill_used).c_str());
    }

    // Decide how many shots to fire
    int num_shots = 1;
    if (burst)
        num_shots = p.weapon.burst_size();
    if (num_shots > p.weapon.charges)
        num_shots = p.weapon.charges;

    if (num_shots == 0)
        debugmsg("game::fire() - num_shots = 0!");

    // Make a sound at our location - Zombies will chase it
    std::string gunsound;
    int noise = p.weapon.noise();
    if (noise < 5) {
        if (burst)
            gunsound = "Brrrip!";
        else
            gunsound = "plink!";
    } else if (noise < 25) {
        if (burst)
            gunsound = "Brrrap!";
        else
            gunsound = "bang!";
    } else if (noise < 60) {
        if (burst)
            gunsound = "P-p-p-pow!";
        else
            gunsound = "blam!";
    } else {
        if (burst)
            gunsound = "Kaboom!!";
        else
            gunsound = "kerblam!";
    }
    if (p.weapon.curammo->type == AT_FUSION || p.weapon.curammo->type == AT_BATT
        || p.weapon.curammo->type == AT_PLUT)
        sound(p.posx, u.posy, 8, "Fzzt!");
    else if (!is_bolt)
        sound(p.posx, p.posy, noise, gunsound);
    // Set up a timespec for use in the nanosleep function below
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = BULLET_SPEED;

    // Use up some ammunition
    p.weapon.charges -= num_shots;
    double deviation;
    double missed_by;
    int trange = trig_dist(p.posx, p.posy, tarx, tary);
    if (trange < int(firing->volume / 3) && firing->ammo != AT_SHOT)
        trange = int(firing->volume / 3);
    else if (p.has_bionic(bio_targeting)) {
        if (trange > LONG_RANGE)
            trange = int(trange * .65);
        else
            trange = int(trange * .8);
    }
    if (firing->skill_used == sk_rifle && trange > LONG_RANGE)
        trange = LONG_RANGE + .6 * (trange - LONG_RANGE);
    std::string message = "";
    bool missed;
    int tart;
    for (int curshot = 0; curshot < num_shots; curshot++) {
        // Calculate deviation from intended target (assuming we shoot for the head)
        deviation = 0;
        // Up to 2 degrees for each skill point < 4; up to 1 removed for each pt > 4
        if (p.sklevel[firing->skill_used] < 5)
            deviation += rng(0, 6 * (5 - p.sklevel[firing->skill_used]));
        else
            deviation -= rng(0, 5 * (p.sklevel[firing->skill_used] - 5));

        if (p.sklevel[sk_gun] < 3)
            deviation += rng(0, 3 * (3 - p.sklevel[sk_gun]));
        else
            deviation -= rng(0, 2 * (p.sklevel[sk_gun] - 3));

        if (p.per_cur < 8)
            deviation += rng(0, 4 * (9 - p.per_cur));
        else {
            deviation -= (p.per_cur > 16 ? 8 : p.per_cur - 8);
            if (one_in(p.per_cur))
                deviation += rng(0, (p.per_cur > 16 ? 8 : p.per_cur - 8));
        }

        if (p.dex_cur < 6)
            deviation += rng(0, 8 * (6 - p.dex_cur));
        else if (p.dex_cur < 8)
            deviation += rng(0, 2 * (8 - p.dex_cur));
        else if (p.dex_cur > 8)
            deviation -= rng(0, p.dex_cur - 8);

        deviation += rng(0, 2 * p.encumb(bp_torso)) + rng(0, 4 * p.encumb(bp_eyes));

        deviation += rng(0, p.weapon.curammo->accuracy);
        deviation += rng(0, p.weapon.accuracy());
        deviation += rng(int(p.recoil / 4), p.recoil);
        int recoil_add = p.weapon.recoil();
        recoil_add -= rng(p.str_cur / 2, p.str_cur);
        recoil_add -= rng(0, p.sklevel[firing->skill_used] / 2);
        if (recoil_add > 0)
            p.recoil += recoil_add;

        // .013 * trange is a computationally cheap version of finding the tangent.
        // (note that .00325 * 4 = .013; .00325 is used because deviation is a number
        //  of quarter-degrees)
        // It's also generous; missed_by will be rather short.
        missed_by = .00325 * deviation * trange;
        missed = false;
        if (debugmon)
            debugmsg("\
missed_by %f deviation %f trange %d charges %d myx %d myy %d monx %d mony %d",
                     missed_by, deviation, trange, p.weapon.charges, p.posx, p.posy, tarx, tary);
        if (missed_by >= 1) {
            // We missed D:
            // Shoot a random nearby space?
            tarx += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
            tary += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
            if (m.sees(p.posx, p.posy, x, y, -1, tart))
                trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
            else
                trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
            missed = true;
            if (!burst) {
                if (&p == &u)
                    add_msg("You miss!");
                else if (u_see_shooter)
                    add_msg("%s misses!", p.name.c_str());
            }
        } else if (missed_by >= .7) {
            // Hit the space, but not necessarily the monster there
            missed = true;
            if (!burst) {
                if (&p == &u)
                    add_msg("You barely miss!");
                else if (u_see_shooter)
                    add_msg("%s barely misses!", p.name.c_str());
            }
        }
        int dam = p.weapon.gun_damage();

        for (std::size_t i {0}; i < trajectory.size() && dam > 0; ++i) {
            if (i > 0)
                m.drawsq(w_terrain, u, trajectory[i - 1].x, trajectory[i - 1].y, false, true);
            // Drawing the bullet uses player u, and not player p, because it's drawn
            // relative to YOUR position, which may not be the gunman's position.
            if (u_see(trajectory[i].x, trajectory[i].y, junk)) {
                mvwputch(w_terrain, trajectory[i].y + SEEY - u.posy,
                         trajectory[i].x + SEEX - u.posx, c_red, '`');
                wrefresh(w_terrain);
                nanosleep(&ts, NULL);
            }

            if (dam <= 0) {
                if (is_bolt
                    && ((u.weapon.curammo->m1 == WOOD && !one_in(5))
                        || (u.weapon.curammo->m1 != WOOD && !one_in(15))))
                    m.add_item(trajectory[i].x, trajectory[i].y, ammotmp);
                return;
            }
            message = "";
            int tx = trajectory[i].x;
            int ty = trajectory[i].y;
            // If there's a monster in the path of our bullet, and either our aim was true,
            //  OR it's not the monster we were aiming at and we were lucky enough to hit it
            int mondex = mon_at(tx, ty);
            if (mondex != -1
                && (!z[mondex].has_flag(MF_DIGS)
                    || rl_dist(p.posx, p.posy, z[mondex].posx, z[mondex].posy) <= 1)
                && (!missed || one_in((7 - int(z[mon_at(tx, ty)].type->size))))) {
                // If we shot us a monster...
                // Calculate damage depending on how good our hit was
                double goodhit = missed_by;
                bool u_see_mon = u_see(&(z[mondex]), junk);
                if (i < trajectory.size() - 1)
                    goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;
                if (z[mondex].has_flag(MF_HARDTOSHOOT) && !one_in(6)
                    && p.weapon.curammo->accuracy >= 4) { // Shot hits anyway
                    if (u_see_mon)
                        add_msg("The shot passes through the %s without hitting.",
                                z[mondex].name().c_str());
                    goodhit = 1;
                } else {
                    // Armor blocks BEFORE any critical effects.
                    int zarm = z[mondex].armor();
                    zarm -= p.weapon.curammo->pierce;
                    if (p.weapon.curammo->accuracy < 4) // Shot doesn't penetrate armor well
                        zarm *= rng(2, 4);
                    if (zarm > 0)
                        dam -= zarm;
                    if (goodhit <= .5 && dam < 0) {
                        if (u_see_mon)
                            add_msg("The shot reflects off the %s%s!", z[mondex].name().c_str(),
                                    z[mondex].made_of(FLESH) ? "'s armored hide" : "");
                        dam = 0;
                        goodhit = 1;
                    }
                    if (goodhit < .1 && !z[mondex].has_flag(MF_NOHEAD)) {
                        message = "Headshot!";
                        dam = rng(5 * dam, 8 * dam);
                        p.practice(firing->skill_used, 5);
                    } else if (goodhit < .2) {
                        message = "Critical!";
                        dam = rng(dam * 2, dam * 3);
                        p.practice(firing->skill_used, 2);
                    } else if (goodhit < .4) {
                        dam = rng(int(dam * .9), int(dam * 1.5));
                        p.practice(firing->skill_used, rng(0, 2));
                    } else if (goodhit <= .7) {
                        message = "Grazing hit.";
                        dam = rng(0, dam);
                    } else
                        dam = 0;
                    // Find the zombie at (x, y) and hurt them, MAYBE kill them!
                    if (dam > 0) {
                        z[mondex].moves -= dam * 8;
                        if (&p == &u && u_see_mon)
                            add_msg("%s You hit the %s for %d damage.", message.c_str(),
                                    z[mondex].name().c_str(), dam);
                        else if (u_see_mon)
                            add_msg("%s %s shoots the %s.", message.c_str(), p.name.c_str(),
                                    z[mondex].name().c_str());
                        if (z[mondex].hurt(dam))
                            kill_mon(mondex);
                        dam = 0;
                    }
                }
            } else if ((!missed || one_in(5))
                       && (npc_at(tx, ty) != -1 || (u.posx == tx && u.posy == ty))) {
                double goodhit = missed_by;
                body_part hit;
                player* h;
                if (u.posx == tx && u.posy == ty)
                    h = &u;
                else
                    h = &(active_npc[npc_at(tx, ty)]);
                int side = rng(0, 1);
                if (i < trajectory.size() - 1)
                    goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;
                if (goodhit < .05) {
                    hit = bp_eyes;
                    dam = rng(3 * dam, 5 * dam);
                    p.practice(firing->skill_used, 5);
                } else if (goodhit < .1) {
                    if (one_in(6))
                        hit = bp_eyes;
                    else if (one_in(4))
                        hit = bp_mouth;
                    else
                        hit = bp_head;
                    dam = rng(2 * dam, 5 * dam);
                    p.practice(firing->skill_used, 5);
                } else if (goodhit < .2) {
                    hit = bp_torso;
                    dam = rng(1.5 * dam, 3 * dam);
                    p.practice(firing->skill_used, 2);
                } else if (goodhit < .4) {
                    if (one_in(3))
                        hit = bp_torso;
                    else if (one_in(2))
                        hit = bp_arms;
                    else
                        hit = bp_legs;
                    dam = rng(int(dam * .9), int(dam * 1.5));
                    p.practice(firing->skill_used, rng(0, 1));
                } else if (goodhit < .5) {
                    if (one_in(2))
                        hit = bp_arms;
                    else
                        hit = bp_legs;
                    dam = rng(dam / 2, dam);
                } else {
                    dam = 0;
                }
                if (dam > 0) {
                    if (h == &u) {
                        add_msg("%s %s shoots your %s!", message.c_str(), p.name.c_str(),
                                body_part_name(hit, side).c_str());
                    } else {
                        h->moves -= dam * 4;
                        if (hit == bp_legs)
                            h->moves -= dam * 4;
                        if (&p == &u)
                            add_msg("%s You hit %s's %s for %d damage.", message.c_str(),
                                    h->name.c_str(), body_part_name(hit, side).c_str(), dam);
                        else if (u_see(tx, ty, junk))
                            add_msg("%s %s shoots %s's %s for %d damage.", message.c_str(),
                                    p.name.c_str(), h->name.c_str(),
                                    body_part_name(hit, side).c_str(), dam);
                    }
                    h->hit(this, hit, side, 0, dam);
                    if (h != &u) {
                        int npcdex = npc_at(tx, ty);
                        if (active_npc[npcdex].hp_cur[hp_head] <= 0
                            || active_npc[npcdex].hp_cur[hp_torso] <= 0) {
                            active_npc[npcdex].die(this);
                            active_npc.erase(active_npc.begin() + npcdex);
                        }
                    }
                }
            } else {
                m.shoot(this, tx, ty, dam, i == trajectory.size() - 1);
            }
        }

        if (is_bolt
            && ((u.weapon.curammo->m1 == WOOD && !one_in(5))
                || (u.weapon.curammo->m1 != WOOD && !one_in(15)))) {
            int lastx = trajectory[trajectory.size() - 1].x;
            int lasty = trajectory[trajectory.size() - 1].y;
            if (m.move_cost(lastx, lasty) == 0) {
                lastx = trajectory[trajectory.size() - 2].x;
                lasty = trajectory[trajectory.size() - 2].y;
            }
            m.add_item(lastx, lasty, ammotmp);
        }
    }

    if (p.weapon.charges == 0)
        p.weapon.curammo = NULL;
}

void Game::butcher()
{
    std::vector<int> corpses;

    for (std::size_t i {0}; i < m.i_at(u.posx, u.posy).size(); ++i) {
        if (m.i_at(u.posx, u.posy)[i].type->id == itype_id::itm_corpse) {
            corpses.push_back(static_cast<int>(i));
        }
    }

    if (corpses.size() == 0) {
        add_msg("There are no corpses here to butcher.");
        return;
    }
    int factor = u.butcher_factor();
    if (factor == 999) {
        add_msg("You don't have a sharp item to butcher with.");
        return;
    }
    // We do it backwards to prevent the deletion of a corpse from corrupting our
    // vector of indices.
    for (int i = corpses.size() - 1; i >= 0; i--) {
        MonsterType* corpse = m.i_at(u.posx, u.posy)[corpses[i]].corpse;
        if (query_yn("Butcher the %s corpse?", corpse->name.c_str())) {
            int age = m.i_at(u.posx, u.posy)[corpses[i]].bday;
            m.i_rem(u.posx, u.posy, corpses[i]);
            int time_to_cut {0};
            int pieces {0};
            int pelts {0};
            double skill_shift = 0.;
            switch (corpse->size) {
            case MS_TINY:
                time_to_cut = 50;
                pieces = 1;
                pelts = 1;
                break;
            case MS_SMALL:
                time_to_cut = 100;
                pieces = 2;
                pelts = 3;
                break;
            case MS_MEDIUM:
                time_to_cut = 200;
                pieces = 4;
                pelts = 6;
                break;
            case MS_LARGE:
                time_to_cut = 400;
                pieces = 8;
                pelts = 10;
                break;
            case MS_HUGE:
                time_to_cut = 800;
                pieces = 16;
                pelts = 18;
                break;
            }
            time_to_cut += factor * 3;
            if (time_to_cut > 0)
                u.moves -= time_to_cut;
            if (u.sklevel[sk_butcher] < 3)
                skill_shift -= rng(0, 8 - u.sklevel[sk_butcher]);
            else
                skill_shift += rng(0, u.sklevel[sk_butcher]);
            if (u.dex_cur < 8)
                skill_shift -= rng(0, 8 - u.dex_cur) / 4;
            else
                skill_shift += rng(0, u.dex_cur - 8) / 4;
            if (u.str_cur < 4)
                skill_shift -= rng(0, 5 * (4 - u.str_cur)) / 4;
            if (factor > 0)
                skill_shift -= rng(0, factor / 5);

            u.practice(sk_butcher, pieces);

            pieces += int(skill_shift);
            if (skill_shift < 3)
                pelts += int(skill_shift - 3);
            if ((corpse->flags & flag_to_bit_position(MF_FUR)
                 || corpse->flags & flag_to_bit_position(MF_LEATHER))
                && pelts > 0) {
                add_msg("You manage to skin the %s!", corpse->name.c_str());
                for (int i = 0; i < pelts; i++) {
                    itype* pelt;
                    if (corpse->flags & flag_to_bit_position(MF_FUR)
                        && corpse->flags & flag_to_bit_position(MF_LEATHER)) {
                        if (one_in(2))
                            pelt = itypes[itm_fur];
                        else
                            pelt = itypes[itm_leather];
                    } else if (corpse->flags & flag_to_bit_position(MF_FUR))
                        pelt = itypes[itm_fur];
                    else
                        pelt = itypes[itm_leather];
                    m.add_item(u.posx, u.posy, pelt, age);
                }
            }
            if (pieces <= 0)
                add_msg("Your clumsy butchering destroys the meat!");
            else {
                itype* meat;
                if (corpse->flags & flag_to_bit_position(MF_POISON)) {
                    if (rng(3, 10) < skill_shift) {
                        add_msg("Your skillful butchering eliminates the poison!");
                        if (corpse->mat == FLESH)
                            meat = itypes[itm_meat];
                        else
                            meat = itypes[itm_veggy];
                    } else {
                        if (corpse->mat == FLESH)
                            meat = itypes[itm_meat_tainted];
                        else
                            meat = itypes[itm_veggy_tainted];
                    }
                } else {
                    if (corpse->mat == FLESH)
                        meat = itypes[itm_meat];
                    else
                        meat = itypes[itm_veggy];
                }
                for (int i = 0; i < pieces; i++)
                    m.add_item(u.posx, u.posy, meat, age);
                add_msg("You butcher the corpse.");
            }
        }
    }
}

void Game::eat()
{
    u.moves -= 250; // TODO: Set this to a variable?
    char ch = inv("Consume item:");
    if (ch == KEY_ESCAPE) {
        add_msg("Never mind.");
        return;
    }
    if (in_tutorial) {
        if (u.i_at(ch).type->id == itm_codeine)
            tutorial_message(LESSON_TOOK_PAINKILLER);
        else if (u.i_at(ch).type->id == itm_cig)
            tutorial_message(LESSON_TOOK_CIG);
        else if (u.i_at(ch).type->id == itm_bottle_plastic)
            tutorial_message(LESSON_DRANK_WATER);
    }
    if (!u.eat(this, ch)) {
        add_msg("You can't eat that!");
        u.moves += 250;
    } else if (u.has_trait(PF_GOURMAND))
        u.moves += 150;
}

void Game::wear()
{
    if (u.wear(this, inv("Wear item:"))) {
        u.moves -= 350; // TODO: Make this variable
        if (in_tutorial) {
            it_armor* armor = dynamic_cast<it_armor*>(u.worn[u.worn.size() - 1].type);
            if (armor->dmg_resist >= 2 || armor->cut_resist >= 4)
                tutorial_message(LESSON_WORE_ARMOR);
            else if (armor->storage >= 20)
                tutorial_message(LESSON_WORE_STORAGE);
            else if (armor->env_resist >= 2)
                tutorial_message(LESSON_WORE_MASK);
        }
    }
}

void Game::takeoff()
{
    if (u.takeoff(this, inv("Take off item:")))
        u.moves -= 250; // TODO: Make this variable
    else
        add_msg("Invalid selection.");
}

void Game::reload()
{
    if (u.weapon.is_gun()) {
        if (u.weapon.charges == u.weapon.clip_size()) {
            add_msg("Your %s is fully loaded!", u.weapon.tname().c_str());
            return;
        }
        int index = u.weapon.pick_reload_ammo(u, true);
        if (index == -1) {
            add_msg("Out of ammo!");
            return;
        }
        u.activity = player_activity(ACT_RELOAD, u.weapon.reload_time(u), index);
        u.moves = 0;
    } else if (u.weapon.is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(u.weapon.type);
        if (tool->ammo == AT_NULL) {
            add_msg("You can't reload a %s!", u.weapon.tname().c_str());
            return;
        }
        int index = u.weapon.pick_reload_ammo(u, true);
        if (index == -1) {
            // Reload failed
            add_msg("Out of %s!", ammo_name(tool->ammo).c_str());
            return;
        }
        u.activity = player_activity(ACT_RELOAD, u.weapon.reload_time(u), index);
        u.moves = 0;
    } else if (!u.is_armed())
        add_msg("You're not wielding anything.");
    else
        add_msg("You can't reload a %s!", u.weapon.tname().c_str());
    refresh_all();
}

void Game::unload()
{
    if (!u.weapon.is_gun() && (!u.weapon.is_tool() || u.weapon.ammo() == AT_NULL)) {
        add_msg("You can't unload a %s!", u.weapon.tname().c_str());
        return;
    } else if (u.weapon.charges == 0) {
        if (u.weapon.contents.size() == 0) {
            if (u.weapon.is_gun())
                add_msg("Your %s isn't loaded, and is not modified.", u.weapon.tname().c_str());
            else
                add_msg("Your %s isn't charged.", u.weapon.tname().c_str());
            return;
        }
        while (u.weapon.is_gun() && u.weapon.contents.size() > 0) {
            Item mod = u.weapon.contents[0];
            int iter = 0;
            while ((mod.invlet == 0 || u.has_item(mod.invlet)) && iter < 52) {
                mod.invlet = nextinv;
                advance_nextinv();
                iter++;
            }

            if (u.volume_carried() + static_cast<int>(mod.volume()) <= u.volume_capacity()
                && u.weight_carried() + static_cast<int>(mod.weight()) <= u.weight_capacity()
                && iter < 52) {
                add_msg("You put the %s in your inventory.", mod.tname().c_str());
                u.i_add(mod);
            } else {
                add_msg("You drop the %s on the ground.", mod.tname().c_str());
                m.add_item(u.posx, u.posy, mod);
            }
            u.moves -= 50;
            u.weapon.contents.erase(u.weapon.contents.begin());
        }
        return;
    }
    u.moves -= int(u.weapon.reload_time(u) / 2);
    it_ammo* tmpammo;
    if (u.weapon.is_gun()) { // Gun ammo is combined with existing items
        for (std::size_t i {0}; i < u.inv.size() && u.weapon.charges > 0; ++i) {
            if (u.inv[i].is_ammo()) {
                tmpammo = dynamic_cast<it_ammo*>(u.inv[i].type);

                if (tmpammo->id == u.weapon.curammo->id && u.inv[i].charges < tmpammo->count) {
                    u.weapon.charges -= tmpammo->count - u.inv[i].charges;
                    u.inv[i].charges = tmpammo->count;

                    if (u.weapon.charges < 0) {
                        u.inv[i].charges += u.weapon.charges;
                        u.weapon.charges = 0;
                    }
                }
            }
        }
    }
    Item newam;
    int iter;
    while (u.weapon.charges > 0) {
        if (u.weapon.curammo != NULL)
            newam = Item(u.weapon.curammo, turn);
        else
            newam = Item(itypes[default_ammo(u.weapon.ammo())], turn);
        iter = 0;
        while ((newam.invlet == 0 || u.has_item(newam.invlet)) && iter < 52) {
            newam.invlet = nextinv;
            advance_nextinv();
            iter++;
        }
        if (newam.made_of(LIQUID))
            newam.charges = u.weapon.charges;
        u.weapon.charges -= newam.charges;
        if (u.weapon.charges < 0) {
            newam.charges += u.weapon.charges;
            u.weapon.charges = 0;
        }

        if (u.weight_carried() + static_cast<int>(newam.weight()) < u.weight_capacity()
            && u.volume_carried() + static_cast<int>(newam.volume()) < u.volume_capacity()
            && iter < 52) {
            if (newam.made_of(LIQUID)) { // Liquids need a container
                if (query_yn("Pour %s on the ground?", newam.tname().c_str()))
                    m.add_item(u.posx, u.posy, newam);
                else {
                    std::stringstream text;
                    ammotype type = u.weapon.ammo();
                    text << "Container for " << newam.tname();
                    char ch = inv(text.str().c_str());
                    Item* cont = &(u.i_at(ch));

                    if (cont->is_tool() && (dynamic_cast<it_tool*>(cont->type))->ammo == type
                        && (cont->charges == 0 || cont->curammo->id == newam.type->id)) {
                        add_msg("You pour %s into your %s.", ammo_name(type).c_str(),
                                cont->tname().c_str());
                        cont->curammo = dynamic_cast<it_ammo*>(newam.type);
                        cont->charges += newam.charges;

                        if (cont->charges
                            > static_cast<int>(dynamic_cast<it_tool*>(cont->type)->max_charges)) {
                            int extra = 0 - cont->charges;
                            cont->charges
                                = static_cast<int>(dynamic_cast<it_tool*>(cont->type)->max_charges);
                            u.weapon.charges += extra;
                            add_msg("There's some left over!");
                            return;
                        }
                    } else if (cont->type->id == itm_null) {
                        add_msg("Never mind.");
                        u.weapon.charges += newam.charges;
                        return;
                    } else if (!cont->is_container()) {
                        add_msg("That %s won't hold %s.", cont->tname().c_str(),
                                newam.tname().c_str());
                        u.weapon.charges += newam.charges;
                        return;
                    } else if (!cont->contents.empty()) {
                        add_msg("Your %s is not empty.", cont->tname().c_str());
                        u.weapon.charges += newam.charges;
                        return;
                    } else {
                        it_container* container = dynamic_cast<it_container*>(cont->type);
                        if (newam.charges > container->contains) {
                            add_msg("You fill the %s with some of the %s.", cont->tname().c_str(),
                                    newam.tname().c_str());
                            u.weapon.charges += newam.charges - container->contains;
                            newam.charges = container->contains;
                        }
                        cont->put_in(newam);
                        return;
                    }
                }
            } else
                u.i_add(newam);
        } else
            m.add_item(u.posx, u.posy, newam);
    }
    u.weapon.curammo = NULL;
}

void Game::wield()
{
    if (u.weapon.type->id > num_items) {
        add_msg("You cannot unwield your %s.", u.weapon.tname().c_str());
        return;
    }
    if (u.wield(this, inv("Wield item:"))) {
        u.moves -= 30;
        u.recoil = 0;
    }
    if (in_tutorial && u.weapon.is_gun())
        tutorial_message(LESSON_GUN_LOAD);
}

void Game::read()
{
    char ch = inv("Read:");
    u.read(this, ch);
}

void Game::chat()
{
    if (active_npc.size() == 0) {
        add_msg("You talk to yourself for a moment.");
        return;
    }
    std::vector<npc*> available;
    int junk;

    for (auto& npc : active_npc) {
        if (u_see(npc.posx, npc.posy, junk)
            && trig_dist(u.posx, u.posy, npc.posx, npc.posy) <= 12) {
            available.push_back(&npc);
        }
    }

    if (available.size() == 1)
        available[0]->talk_to_u(this);
    else {
        WINDOW* w = newwin(available.size() + 3, 40, 10, 20);
        wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO,
                LINE_XOOX);

        for (std::size_t i {0}; i < available.size(); ++i) {
            mvwprintz(w, static_cast<int>(i) + 1, 1, c_white, "%d: %s", static_cast<int>(i) + 1,
                      available[i]->name.c_str());
        }

        mvwprintz(w, available.size() + 1, 1, c_white, "%d: Cancel", available.size() + 1);
        wrefresh(w);
        int ch {0};

        do {
            ch = getch();
        } while (ch < '1' || ch > '1' + static_cast<int>(available.size()));

        ch -= '1';

        if (ch == static_cast<int>(available.size())) {
            return;
        }

        delwin(w);
        available[ch]->talk_to_u(this);
    }
    u.moves -= 100;
}

void Game::plmove(int x, int y)
{
    if (run_mode == 2) { // Monsters around and we don't wanna run
        add_msg("Monster spotted--run mode is on! (Press '!' to turn it off.)");
        return;
    }
    x += u.posx;
    y += u.posy;
    // Check if our movement is actually an attack on a monster
    int mondex = mon_at(x, y);
    bool displace = false; // Are we displacing a monster?
    if (mondex != -1) {
        if (z[mondex].friendly == 0) {
            int udam = u.hit_mon(this, &z[mondex]);
            if (z[mondex].hurt(udam))
                kill_mon(mondex);
            else if (udam > 0) // Stun them
                z[mondex].moves -= udam + int((udam / z[mondex].hp) * z[mondex].speed);
            return;
        } else
            displace = true;
    }
    // If not a monster, maybe there's an NPC there
    int npcdex = npc_at(x, y);
    if (npcdex != -1
        && ((!active_npc[npcdex].is_friend() && !active_npc[npcdex].is_following())
            || query_yn("Really attack %s?", active_npc[npcdex].name.c_str()))) {
        body_part bphit;
        int hitdam = 0, hitcut = 0;
        bool success = u.hit_player(active_npc[npcdex], bphit, hitdam, hitcut);
        if (u.recoil <= 30)
            u.recoil += 6;
        u.moves -= (80 + u.weapon.volume() * 2 + u.weapon.weight() + u.encumb(bp_torso));
        if (!success) {
            int stumble_pen = u.weapon.volume() + int(u.weapon.weight() / 2);
            if (u.has_trait(PF_DEFT))
                stumble_pen = int(stumble_pen * .4) - 10;
            if (stumble_pen < 0)
                stumble_pen = 0;
            if (stumble_pen > 0 && (one_in(16 - u.str_cur) || one_in(22 - u.dex_cur)))
                stumble_pen = rng(0, stumble_pen);
            if (stumble_pen >= 30)
                add_msg("You miss and stumble with the momentum.");
            else if (stumble_pen >= 10)
                add_msg("You swing wildly and miss.");
            else
                add_msg("You miss.");
            u.moves -= stumble_pen;
        } else { // We hit!
            int side = rng(0, 1);
            add_msg("You hit %s's %s.", active_npc[npcdex].name.c_str(),
                    body_part_name(bphit, side).c_str());
            if (u.has_bionic(bio_shock) && u.power_level >= 2 && one_in(3)
                && (!u.is_armed() || u.weapon.type->id > num_items)) {
                add_msg("You shock %s!", active_npc[npcdex].name.c_str());
                int shock = rng(2, 5);
                hitdam += shock;
                active_npc[npcdex].moves -= shock * 300;
                u.power_level -= 2;
            }
            if (u.has_bionic(bio_heat_absorb) && u.power_level >= 1 && !u.is_armed()) {
                u.power_level--;
                if (one_in(2)) {
                    add_msg("You drain %s's body heat!", active_npc[npcdex].name.c_str());
                    u.charge_power(rng(3, 5));
                    hitdam += rng(2, 6);
                } else {
                    add_msg("You attempt to drain body heat, but fail.");
                }
            }
            if (u.has_trait(PF_FANGS) && one_in(20 - u.dex_cur)) {
                add_msg("You sink your fangs into %s!", active_npc[npcdex].name.c_str());
                hitcut += 18;
            }
            sound(u.posx, u.posy, 6, "");
            if (u.weapon.made_of(GLASS)
                && rng(0, u.weapon.volume() + 8) < u.weapon.volume() + u.str_cur) {
                // Glass weapon shattered
                add_msg("Your %s shatters!", u.weapon.tname().c_str());

                for (const auto& content : u.weapon.contents) {
                    m.add_item(x, y, content);
                }

                sound(u.posx, u.posy, 16, "");
                u.hit(this, bp_hands, 1, 0, rng(0, u.weapon.volume() * 2));
                if (u.weapon.is_two_handed(&u)) // Hurt left arm too, if it was big
                    u.hit(this, bp_hands, 0, 0, rng(0, u.weapon.volume()));
                hitcut += rng(0, int(u.weapon.volume() * 1.5)); // Hurt the monster
                u.remove_weapon();
            }
            active_npc[npcdex].hit(this, bphit, side, hitdam, hitcut);
            if (active_npc[npcdex].hp_cur[hp_head] <= 0
                || active_npc[npcdex].hp_cur[hp_torso] <= 0) {
                active_npc[npcdex].die(this);
                active_npc.erase(active_npc.begin() + npcdex);
            }
        }
        return;
    }

    // Otherwise, actual movement, zomg
    if (u.has_disease(DI_IN_PIT)) {
        if (rng(0, 40) > u.str_cur + int(u.dex_cur / 2)) {
            add_msg("You try to escape the pit, but slip back in.");
            return;
        } else {
            add_msg("You escape the pit!");
            u.rem_disease(DI_IN_PIT);
        }
    }
    if (m.move_cost(x, y) > 0) { // move_cost() of 0 = impassible (e.g. a wall)
        if (u.underwater)
            u.underwater = false;
        int movecost;
        if (m.field_at(x, y).is_dangerous()
            && !query_yn("Really step into that %s?", m.field_at(x, y).name().c_str()))
            return;
        if (m.tr_at(x, y) != tr_null
            && u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(x, y)]->visibility
            && !query_yn("Really step onto that %s?", traps[m.tr_at(x, y)]->name.c_str()))
            return;
        if (u.has_trait(PF_PARKOUR) && m.move_cost(x, y) <= 4)
            movecost = 100 + u.encumb(bp_feet) * 5 + u.encumb(bp_legs) * 3;
        else
            movecost = m.move_cost(x, y) * 50 + u.encumb(bp_feet) * 5 + u.encumb(bp_legs) * 3;
        if (u.has_trait(PF_FLEET) && m.move_cost(x, y) == 2)
            movecost = int(movecost * .85);
        movecost += u.encumb(bp_mouth) * 5;
        if (!u.wearing_something_on(bp_feet))
            movecost += 15;
        if (u.recoil > 0) {
            if ((u.str_cur / 2) + u.sklevel[skill::sk_gun] >= static_cast<int>(u.recoil)) {
                u.recoil = 0;
            } else {
                u.recoil -= int(u.str_cur / 2) + u.sklevel[sk_gun];
                u.recoil = int(u.recoil / 2);
            }
        }
        u.moves -= movecost;
        if (m.move_cost(x, y) > 2 && (!u.has_trait(PF_PARKOUR) || m.move_cost(x, y) > 4))
            add_msg("Moving past this %s is slow!", m.tername(x, y).c_str());
        if (m.has_flag(rough, x, y)) {
            if (one_in(5) && u.armor_bash(bp_feet) < rng(1, 5)) {
                add_msg("You hurt your feet on the %s!", m.tername(x, y).c_str());
                u.hit(this, bp_feet, 0, 0, 1);
                u.hit(this, bp_feet, 1, 0, 1);
            }
        }
        if (m.has_flag(sharp, x, y) && !one_in(3) && !one_in(40 - int(u.dex_cur / 2))) {
            add_msg("You cut yourself on the %s!", m.tername(x, y).c_str());
            u.hit(this, bp_torso, 0, 0, rng(1, 4));
        }
        if (u.has_trait(PF_LIGHTSTEP))
            sound(x, y, 2, ""); // Sound of footsteps may awaken nearby monsters
        else
            sound(x, y, 6, ""); // Sound of footsteps may awaken nearby monsters
        // If we moved out of the nonant, we need update our map data
        if (m.has_flag(swimmable, x, y) && u.has_disease(DI_ONFIRE)) {
            add_msg("The water puts out the flames!");
            u.rem_disease(DI_ONFIRE);
        }
        if (x < SEEX || y < SEEY || x >= SEEX * 2 || y >= SEEY * 2)
            update_map(x, y);
        if (displace) { // We displaced a friendly monster!
            z[mondex].move_to(this, u.posx, u.posy);
            add_msg("You displace the %s.", z[mondex].name().c_str());
        }
        u.posx = x;
        u.posy = y;
        if (m.tr_at(x, y) != tr_null) { // We stepped on a trap!
            trap* tr = traps[m.tr_at(x, y)];
            if (!u.avoid_trap(tr)) {
                trapfunc f;
                (f.*(tr->act))(this, x, y);
            }
        }

        // Special tutorial messages
        if (in_tutorial) {
            bool showed_message = false;
            for (int i = u.posx - 1; i <= u.posx + 1 && !showed_message; i++) {
                for (int j = u.posy - 1; j <= u.posy + 1 && !showed_message; j++) {
                    if (m.ter(i, j) == t_door_c) {
                        showed_message = true;
                        tutorial_message(LESSON_OPEN);
                    } else if (m.ter(i, j) == t_door_o) {
                        showed_message = true;
                        tutorial_message(LESSON_CLOSE);
                    } else if (m.ter(i, j) == t_window) {
                        showed_message = true;
                        tutorial_message(LESSON_WINDOW);
                    } else if (m.ter(i, j) == t_rack && m.i_at(i, j).size() > 0) {
                        showed_message = true;
                        tutorial_message(LESSON_EXAMINE);
                    } else if (m.ter(i, j) == t_stairs_down) {
                        showed_message = true;
                        tutorial_message(LESSON_STAIRS);
                    } else if (m.ter(i, j) == t_water_sh) {
                        showed_message = true;
                        tutorial_message(LESSON_PICKUP_WATER);
                    }
                }
            }
        }
        // List items here
        if (!u.has_disease(DI_BLIND) && m.i_at(x, y).size() <= 3 && m.i_at(x, y).size() != 0) {
            if (in_tutorial)
                tutorial_message(LESSON_PICKUP);
            std::string buff = "You see here ";

            for (std::size_t i {0}; i < m.i_at(x, y).size(); ++i) {
                buff += m.i_at(x, y)[i].tname();

                if (i + 2 < m.i_at(x, y).size()) {
                    buff += ", ";
                } else if (i + 1 < m.i_at(x, y).size()) {
                    buff += ", and ";
                }
            }

            buff += ".";
            add_msg(buff.c_str());
        } else if (m.i_at(x, y).size() != 0)
            add_msg("There are many items here.");
    } else if (m.has_flag(swimmable, x, y)) { // Dive into water!
        // Requires confirmation if we were on dry land previously
        if ((m.has_flag(swimmable, u.posx, u.posy) && m.move_cost(u.posx, u.posy) == 0)
            || query_yn("Dive into the water?")) {
            if (m.move_cost(u.posx, u.posy) > 0 && u.swim_speed() < 500)
                add_msg("You start swimming.  Press '>' to dive underwater.");
            plswim(x, y);
            if (u.has_disease(DI_ONFIRE)) {
                add_msg("The water puts out the flames!");
                u.rem_disease(DI_ONFIRE);
            }
        }
    } else { // Invalid move
        if (u.has_disease(DI_BLIND)) {
            add_msg("You bump into something!");
            u.moves -= 100;
        } else if (m.open_door(x, y, m.ter(u.posx, u.posy) == t_floor))
            u.moves -= 100;
        else if (m.ter(x, y) == t_door_locked) {
            u.moves -= 100;
            add_msg("That door is locked!");
            if (in_tutorial)
                tutorial_message(LESSON_SMASH);
        }
    }
}

void Game::plswim(int x, int y)
{
    if (x < SEEX || y < SEEY || x >= SEEX * 2 || y >= SEEY * 2)
        update_map(x, y);
    u.posx = x;
    u.posy = y;
    if (!m.has_flag(swimmable, x, y)) {
        debugmsg("Tried to swim in %s!", m.tername(x, y).c_str());
        return;
    }
    int movecost = u.swim_speed();
    u.practice(sk_swimming, 1);
    if (movecost >= 500) {
        if (!u.underwater) {
            add_msg("You sink%s!", (movecost >= 400 ? " like a rock" : ""));
            u.underwater = true;
            u.oxygen = 20 + u.str_cur;
        }
    }
    if (u.oxygen <= 5 && u.underwater) {
        if (movecost < 500)
            popup("You need to breathe! (Press '<' to surface.)");
        else
            popup("You need to breathe but you can't swim!  Get to dry land, quick!");
    }
    u.moves -= movecost;

    for (auto& item : u.inv) {
        if (item.type->m1 == material::IRON && item.damage < 5 && one_in(3)) {
            ++item.damage;
        }
    }
}

void Game::vertical_move(int movez, bool force)
{
    // > and < are used for diving underwater.
    if (m.move_cost(u.posx, u.posy) == 0 && m.has_flag(swimmable, u.posx, u.posy)) {
        if (movez == -1) {
            u.underwater = true;
            u.oxygen = 20 + u.str_cur;
            add_msg("You dive underwater!");
        } else {
            if (u.swim_speed() < 500) {
                u.underwater = false;
                add_msg("You surface.");
            } else
                add_msg("You can't surface!");
        }
        return;
    }
    // Force means we're going down, even if there's no staircase, etc.
    // This happens with sinkholes and the like.
    if (!force
        && ((movez == -1 && !m.has_flag(goes_down, u.posx, u.posy))
            || (movez == 1 && !m.has_flag(goes_up, u.posx, u.posy)))) {
        add_msg("You can't go %s here!", (movez == -1 ? "down" : "up"));
        return;
    }
    levz += movez;

    if (levz == monbuffz
        && monbuff_turn + 120
            <= static_cast<int>(turn) - trig_dist(levx, levy, monbuffx, monbuffy)) {

        for (const auto& monster : monbuff) {
            z.push_back(monster);
        }

        monbuff.clear();
    }

    u.moves -= 100;
    if (force) {                                                 // Basically, we fell.
        int dam = int((u.str_max / 4) + rng(5, 10)) * rng(1, 3); // The bigger they are
        dam -= rng(u.dodge(), u.dodge() * 3);
        if (dam <= 0)
            add_msg("You fall expertly and take no damage.");
        else {
            add_msg("You fall heavily, taking %d damage.", dam);
            u.hurtall(dam);
        }
    }
    int group, junk;
    monbuffx = levx;
    monbuffy = levy;
    monbuffz = levz - movez;

    for (std::size_t i {0}; i < z.size(); ++i) {
        if (trig_dist(u.posx, u.posy, z[i].posx, z[i].posy) > 3) {
            monbuff.push_back(z[i]);
            group = valid_group((mon_id)(z[i].type->id), levx, levy);
            if (group != -1) {
                cur_om.zg[group].population++;
                if (cur_om.zg[group].population / pow(cur_om.zg[group].radius, 2) > 5)
                    cur_om.zg[group].radius++;
            } else if (mt_to_mc((mon_id)(z[i].type->id)) != mcat_null)
                cur_om.zg.push_back(mongroup(mt_to_mc(mon_id(z[i].type->id)), levx, levy, 1, 1));

            for (auto& npc : active_npc) {
                if (npc.target == &z[i]) {
                    npc.target = nullptr;
                }
            }

            z.erase(z.begin() + i);
            i--;
        } else if (u_see(&(z[i]), junk))
            add_msg("The %s follows you %s.", z[i].name().c_str(), (movez == -1 ? "down" : "up"));
    }

    cur_om.save();
    m.save(&cur_om, turn, levx, levy);
    cur_om = overmap(this, cur_om.posx, cur_om.posy, cur_om.posz + movez);
    m.init(this, levx, levy);
    // Move the player to the corresponding up-route. (If one exists.)
    for (int i = 0; i < SEEX * 3; i++) {
        for (int j = 0; j < SEEY * 3; j++) {
            if ((movez == -1 && m.has_flag(goes_up, i, j))
                || (movez == 1 && m.has_flag(goes_down, i, j))) {
                u.posx = i;
                u.posy = j;
                j = SEEY * 3;
                i = SEEX * 3;
            } else if (m.ter(i, j) == t_manhole_cover) {
                m.add_item(i + rng(-1, 1), j + rng(-1, 1), itypes[itm_manhole_cover], 0);
                m.ter(i, j) = t_manhole;
                u.posx = i;
                u.posy = j;
                j = SEEY * 3;
                i = SEEX * 3;
            }
        }
    }
    update_map(u.posx, u.posy);
    for (int i = 0; i < SEEX * 3; i++) {
        for (int j = 0; j < SEEY * 3; j++) {
            if ((movez == -1 && m.has_flag(goes_up, i, j))
                || (movez == 1 && m.has_flag(goes_down, i, j))) {
                u.posx = i;
                u.posy = j;
                j = SEEY * 3;
                i = SEEX * 3;
            }
        }
    }
    refresh_all();
}

void Game::update_map(int& x, int& y)
{
    int shiftx = 0, shifty = 0;
    int group;
    int olevx = 0, olevy = 0;
    if (x < SEEX) {
        x = SEEX * 2 - 1;
        shiftx = -1;
    } else if (x >= SEEX * 2) {
        x = SEEX;
        shiftx = 1;
    }
    if (y < SEEY) {
        y = SEEY * 2 - 1;
        shifty = -1;
    } else if (y >= SEEY * 2) {
        y = SEEY;
        shifty = 1;
    }
    // Before we shift/save the map, check if we need to move monsters back to
    // their spawn locations.
    m.shift(this, levx, levy, shiftx, shifty);
    levx += shiftx;
    levy += shifty;
    if (levx < 0) {
        levx += OMAPX * 2;
        olevx = -1;
    } else if (levx > OMAPX * 2 - 1) {
        levx -= OMAPX * 2;
        olevx = 1;
    }
    if (levy < 0) {
        levy += OMAPY * 2;
        olevy = -1;
    } else if (levy > OMAPY * 2 - 1) {
        levy -= OMAPY * 2;
        olevy = 1;
    }
    if (olevx != 0 || olevy != 0) {
        cur_om.save();
        cur_om = overmap(this, cur_om.posx + olevx, cur_om.posy + olevy, cur_om.posz);
    }

    // Shift monsters.
    for (std::size_t i {0}; i < z.size(); ++i) {
        z[i].shift(shiftx, shifty);
        if (z[i].posx < 0 - SEEX * 3 || z[i].posy < 0 - SEEX * 3 || z[i].posx > SEEX * 6
            || z[i].posy > SEEY * 6) {
            if (z[i].spawnmapx != -1) { // Static spawn, move them back there
                Map tmp;
                tmp.init(this, z[i].spawnmapx, z[i].spawnmapy);
                tmp.add_spawn(mon_id(z[i].type->id), 1, z[i].spawnposx, z[i].spawnposy);
                tmp.save(&cur_om, turn, z[i].spawnmapx, z[i].spawnmapy);
            } else { // Absorb them back into a group
                group = valid_group((mon_id)(z[i].type->id), levx + shiftx, levy + shifty);
                if (group != -1) {
                    cur_om.zg[group].population++;
                    if (cur_om.zg[group].population / pow(cur_om.zg[group].radius, 2) > 5)
                        cur_om.zg[group].radius++;
                } else if (mt_to_mc((mon_id)(z[i].type->id)) != mcat_null)
                    cur_om.zg.push_back(mongroup(mt_to_mc((mon_id)(z[i].type->id)), levx + shiftx,
                                                 levy + shifty, 1, 1));
            }

            for (auto& npc : active_npc) {
                if (npc.target == &z[i]) {
                    npc.target = nullptr;
                }
            }

            z.erase(z.begin() + i);
            i--;
        }
    }

    // Shift NPCs.
    for (std::size_t i {0}; i < active_npc.size(); ++i) {
        active_npc[i].shift(shiftx, shifty);
        if (active_npc[i].posx < 0 - SEEX * 3 || active_npc[i].posy < 0 - SEEX * 3
            || active_npc[i].posx > SEEX * 6 || active_npc[i].posy > SEEY * 6) {
            cur_om.npcs.push_back(active_npc[i]);
            active_npc.erase(active_npc.begin() + i);
            i--;
        }
    }

    // Spawn NPCs?
    if (!in_tutorial) {
        npc temp;

        for (std::size_t i {0}; i < cur_om.npcs.size(); ++i) {
            if (rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy) <= 2) {
                int dx = cur_om.npcs[i].mapx - levx, dy = cur_om.npcs[i].mapy - levy;
                if (debugmon)
                    debugmsg("Spawning static NPC, %d:%d (%d:%d)", levx, levy, cur_om.npcs[i].mapx,
                             cur_om.npcs[i].mapy);
                temp = cur_om.npcs[i];
                if (temp.posx == -1 || temp.posy == -1) {
                    debugmsg("Static NPC with no fine location data.");
                    temp.posx = SEEX * 2 * (temp.mapx - levx) + rng(0 - SEEX, SEEX);
                    temp.posy = SEEY * 2 * (temp.mapy - levy) + rng(0 - SEEY, SEEY);
                } else {
                    if (debugmon)
                        debugmsg("Static NPC fine location %d:%d (%d:%d)", temp.posx, temp.posy,
                                 temp.posx + dx * SEEX, temp.posy + dy * SEEY);
                    temp.posx += dx * SEEX;
                    temp.posy += dy * SEEY;
                }
                active_npc.push_back(temp);
                cur_om.npcs.erase(cur_om.npcs.begin() + i);
                i--;
            }
        }
    }
    // Spawn monsters if appropriate
    m.spawn_monsters(this); // Static monsters

    if (static_cast<int>(turn) >= nextspawn) {
        spawn_mon(shiftx, shifty);
    }

    // Shift scent
    unsigned int newscent[SEEX * 3][SEEY * 3];
    for (int i = 0; i < SEEX * 3; i++) {
        for (int j = 0; j < SEEY * 3; j++)
            newscent[i][j] = scent(i + (shiftx * SEEX), j + (shifty * SEEY));
    }
    for (int i = 0; i < SEEX * 3; i++) {
        for (int j = 0; j < SEEY * 3; j++)
            scent(i, j) = newscent[i][j];
    }
    draw_minimap();
}

void Game::spawn_mon(int shiftx, int shifty)
{
    int nlevx = levx + shiftx;
    int nlevy = levy + shifty;
    int group;
    int monx {0};
    int mony {0};
    int dist;
    int pop, rad;
    int iter;
    // Create a new NPC?
    if (one_in(50 + 5 * cur_om.npcs.size())) {
        npc temp;
        temp.randomize(this);
        temp.spawn_at(&cur_om, levx + (1 * rng(-2, 2)), levy + (1 * rng(-2, 2)));
        temp.posx = SEEX * 2 * (temp.mapx - levx) + rng(0 - SEEX, SEEX);
        temp.posy = SEEY * 2 * (temp.mapy - levy) + rng(0 - SEEY, SEEY);
        temp.attitude = NPCATT_TALK;
        active_npc.push_back(temp);
    }

    // Now, spawn monsters (perhaps)
    Monster zom;

    for (std::size_t i {0}; i < cur_om.zg.size(); ++i) {
        group = 0;
        dist = trig_dist(nlevx, nlevy, cur_om.zg[i].posx, cur_om.zg[i].posy);
        pop = cur_om.zg[i].population;
        rad = cur_om.zg[i].radius;
        if (dist <= rad) { // We're in an existing group's territory!
            while (rng(0, long((3.8 - double(dist / rad)) * pop)) > pow(rad, 2) && group < pop
                   && group < 16)
                // (The area of the group's territory) in (population/square at this range)
                // chance of adding one monster; cap at the population OR 16
                group++;
            cur_om.zg[i].population -= group;
            if (group > 0) // If we spawned some zombies, advance the timer
                nextspawn += rng(group * 3, group * 8 + z.size() * 2);
            for (int j = 0; j < group; j++) { // For each monster in the group...
                mon_id type = valid_monster_from(moncats[cur_om.zg[i].type]);
                if (type == mon_null)
                    j = group; // No monsters may be spawned; not soon enough?
                else {
                    zom = Monster(mtypes[type]);
                    iter = 0;
                    do {
                        monx = rng(0, SEEX * 3 - 1);
                        mony = rng(0, SEEY * 3 - 1);
                        if (shiftx == -1)
                            monx = 0 - SEEX * 2;
                        if (shiftx == 1)
                            monx = SEEX * 4;
                        if (shifty == -1)
                            mony = 0 - SEEX * 2;
                        if (shifty == 1)
                            mony = SEEY * 4;
                        monx += rng(-5, 10);
                        mony += rng(-5, 10);
                        iter++;
                    } while ((!zom.can_move_to(m, monx, mony) || mon_at(monx, mony) != -1)
                             && iter < 50); // Just in case it's totally invalid
                    zom.spawn(monx, mony);
                    z.push_back(zom);
                }
            }                                           // Placing monsters of this group is done!
            if (cur_om.zg[i].population <= 0) {         // Last monster in the group spawned...
                cur_om.zg.erase(cur_om.zg.begin() + i); // ...so remove that group
                i--;                                    // And don't increment i.
            }
        } // Check whether we're inside a group's radius is done!
    }
}

mon_id Game::valid_monster_from(std::vector<mon_id> group)
{
    std::vector<mon_id> valid;
    int rntype = 0;

    for (const auto& monster_id : group) {
        if (mtypes[monster_id]->frequency > 0
            && static_cast<int>(turn) + 900 >= mtypes[monster_id]->difficulty * 300) {
            valid.push_back(monster_id);
            rntype += mtypes[monster_id]->frequency;
        }
    }

    if (valid.size() == 0)
        return mon_null;
    int curmon = -1;
    if (rntype > 0)
        rntype = rng(0, rntype - 1); // rntype set to [0, rntype)
    do {
        curmon++;
        rntype -= mtypes[valid[curmon]]->frequency;
    } while (rntype > 0);
    return valid[curmon];
}

int Game::valid_group(mon_id type, int x, int y)
{
    std::vector<int> valid_groups;
    std::vector<int> semi_valid; // Groups that're ALMOST big enough
    int dist;

    for (std::size_t i {0}; i < cur_om.zg.size(); ++i) {
        dist = trig_dist(x, y, cur_om.zg[i].posx, cur_om.zg[i].posy);

        if (dist < cur_om.zg[i].radius) {
            for (const auto monster_id : moncats[cur_om.zg[i].type]) {
                if (type == monster_id) {
                    valid_groups.push_back(static_cast<int>(i));
                    break;
                }
            }
        } else if (dist < cur_om.zg[i].radius + 1) {
            for (const auto monster_id : moncats[cur_om.zg[i].type]) {
                if (type == monster_id) {
                    semi_valid.push_back(static_cast<int>(i));
                    break;
                }
            }
        }
    }

    if (valid_groups.size() == 0) {
        if (semi_valid.size() == 0) {
            return -1;
        } else {
            // If there's a group that's ALMOST big enough, expand that group's radius
            // by one and absorb into that group.
            int semi = rng(0, semi_valid.size() - 1);
            cur_om.zg[semi_valid[semi]].radius++;
            return semi_valid[semi];
        }
    }
    return valid_groups[rng(0, valid_groups.size() - 1)];
}

void Game::wait()
{
    char ch = menu("Wait for how long?", "5 Minutes", "30 Minutes", "1 hour", "2 hours", "3 hours",
                   "6 hours", "Exit", NULL);
    int time {0};

    if (ch == 7)
        return;
    switch (ch) {
    case 1:
        time = 5000;
        break;
    case 2:
        time = 30000;
        break;
    case 3:
        time = 60000;
        break;
    case 4:
        time = 120000;
        break;
    case 5:
        time = 180000;
        break;
    case 6:
        time = 360000;
        break;
    }
    u.activity = player_activity(ACT_WAIT, time, 0);
    u.moves = 0;
}

void Game::gameover()
{
    erase();
    mvprintw(0, 35, "GAME OVER");
    inv();
}

void Game::write_msg()
{
    werase(w_messages);
    int size = 7;
    for (int i = size; i > 0; i--) {
        if (static_cast<int>(messages.size()) >= i) {
            if (curmes >= i) {
                mvwprintz(w_messages, size - (i - 1), 0, nc_color::c_ltred,
                          messages[messages.size() - i].c_str());
            } else {
                mvwprintz(w_messages, size - (i - 1), 0, nc_color::c_dkgray,
                          messages[messages.size() - i].c_str());
            }
        }
    }
    curmes = 0;
    wrefresh(w_messages);
}

void Game::teleport()
{
    int newx, newy, tries = 0;
    u.add_disease(DI_TELEGLOW, 300, this);
    do {
        newx = u.posx + rng(0, SEEX * 2) - SEEX;
        newy = u.posy + rng(0, SEEY * 2) - SEEY;
        tries++;
    } while (tries < 15 && (m.move_cost(newx, newy) == 0 || mon_at(newx, newy) != -1));
    u.posx = newx;
    u.posy = newy;
    if (tries == 15) {
        if (m.move_cost(newx, newy) == 0) { // TODO: If we land in water, swim
            add_msg("You teleport into the middle of a %s!", m.tername(newx, newy).c_str());
            u.hurt(this, bp_torso, 0, 500);
        } else if (mon_at(newx, newy) != -1) {
            int i = mon_at(newx, newy);
            add_msg("You teleport into the middle of a %s!", z[i].name().c_str());
            kill_mon(i);
        }
    }
    update_map(u.posx, u.posy);
}

std::vector<Point> Game::target(int& x,
                                int& y,
                                int lowx,
                                int lowy,
                                int hix,
                                int hiy,
                                std::vector<Monster> t,
                                int& target,
                                Item* relevent)
{
    std::vector<Point> ret;
    int tarx, tary, tart, junk;
    int sight_dist = u.sight_range(light_level());

    // First, decide on a target among the monsters, if there are any in range
    if (t.size() > 0) {
        // Check for previous target
        if (target == -1) {
            // If no previous target, target the closest there is
            double closest = -1;
            double dist;

            for (std::size_t i {0}; i < t.size(); ++i) {
                dist = trig_dist(t[i].posx, t[i].posy, u.posx, u.posy);

                if (closest < 0 || dist < closest) {
                    closest = dist;
                    target = static_cast<int>(i);
                }
            }
        }
        x = t[target].posx;
        y = t[target].posy;
    } else
        target = -1; // No monsters in range, don't use target, reset to -1

    WINDOW* w_target = newwin(13, 48, 12, SEEX * 2 + 8);
    wborder(w_target, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO,
            LINE_XOOX);
    if (relevent == &u.weapon && relevent->is_gun())
        mvwprintz(w_target, 1, 1, c_red, "Firing %s - %s (%d)", u.weapon.tname().c_str(),
                  u.weapon.curammo->name.c_str(), u.weapon.charges);
    else
        mvwprintz(w_target, 1, 1, c_red, "Throwing %s", relevent->tname().c_str());
    mvwprintz(w_target, 2, 1, c_white, "Move cursor to target with directional keys.");
    mvwprintz(w_target, 3, 1, c_white, "'<' '>' Cycle targets; 'f' or '.' to fire.");
    wrefresh(w_target);
    char ch;
    // The main loop.
    do {
        // Clear the target window.
        for (int i = 4; i < 12; i++) {
            for (int j = 1; j < 46; j++)
                mvwputch(w_target, i, j, c_white, ' ');
        }
        if (x != u.posx || y != u.posy) {
            // Calculate the return vector (and draw it too).
            for (const auto& point : ret) {
                m.drawsq(w_terrain, u, point.x, point.y, /*invert=*/false, /*show_items=*/true);
            }

            // Draw the player
            mvwputch(w_terrain, SEEX, SEEY, u.color(), '@');

            // Draw the monsters.
            for (auto& monster : z) {
                if (u_see(&monster, tart) && monster.posx >= lowx && monster.posy >= lowy
                    && monster.posx <= hix && monster.posy <= hiy) {
                    monster.draw(w_terrain, u.posx, u.posy, /*inv=*/false);
                }
            }

            // Draw the NPCs.
            for (auto& npc : active_npc) {
                if (u_see(npc.posx, npc.posy, tart)) {
                    npc.draw(w_terrain, u.posx, u.posy, /*inv=*/false);
                }
            }

            if (m.sees(u.posx, u.posy, x, y, -1, tart)) {  // Selects a valid line-of-sight
                ret = line_to(u.posx, u.posy, x, y, tart); // Sets the vector to that LOS

                for (auto& point : ret) {
                    if (std::abs(point.x - u.posx) <= sight_dist
                        && std::abs(point.y - u.posy) <= sight_dist) {
                        if (mon_at(point.x, point.y) != -1
                            && u_see(&z[mon_at(point.x, point.y)], tart)) {
                            z[mon_at(point.x, point.y)].draw(w_terrain, u.posx, u.posy,
                                                             /*inv=*/true);
                        } else if (npc_at(point.x, point.y) != -1) {
                            active_npc[npc_at(point.x, point.y)].draw(w_terrain, u.posx, u.posy,
                                                                      /*inv=*/true);
                        } else {
                            m.drawsq(w_terrain, u, point.x, point.y, /*invert=*/true,
                                     /*show_items=*/true);
                        }
                    }
                }
            }
            if (mon_at(x, y) == -1) {
                mvwputch(w_terrain, y + SEEY - u.posy, x + SEEX - u.posx, c_red, '*');
                mvwprintw(w_status, 0, 9, "                             ");
            } else if (u_see(&(z[mon_at(x, y)]), tart))
                z[mon_at(x, y)].print_info(this, w_target);
            wrefresh(w_target);
        }
        wrefresh(w_terrain);
        wrefresh(w_status);
        refresh();
        ch = input();
        get_direction(tarx, tary, ch);
        if (tarx != -2 && tary != -2) { // Direction character pressed
            if (m.sees(u.posx, u.posy, x, y, -1, junk))
                m.drawsq(w_terrain, u, x, y, false, true);
            else
                mvwputch(w_terrain, y + SEEY - u.posy, x + SEEX - u.posx, c_black, 'X');
            x += tarx;
            y += tary;
            if (x < lowx)
                x = lowx;
            else if (x > hix)
                x = hix;
            if (y < lowy)
                y = lowy;
            else if (y > hiy)
                y = hiy;
        } else if ((ch == '<') && (target != -1)) {
            target--;
            if (target == -1)
                target = t.size() - 1;
            x = t[target].posx;
            y = t[target].posy;
        } else if ((ch == '>') && (target != -1)) {
            target++;

            if (target == static_cast<int>(t.size())) {
                target = 0;
            }

            x = t[target].posx;
            y = t[target].posy;
        } else if (ch == '.' || ch == 'f') {
            for (std::size_t i {0}; i < t.size(); ++i) {
                if (t[i].posx == x && t[i].posy == y) {
                    target = static_cast<int>(i);
                }
            }

            return ret;
        } else if (ch == KEY_ESCAPE) { // ESC hit - return false, e.g. CANCEL
            ret.clear();
            return ret;
        }
    } while (true);
}

oter_id Game::ter_at(int omx, int omy, bool& mark_as_seen)
{
    oter_id ret;
    int sx = 0, sy = 0;
    if (omx >= OMAPX)
        sx = 1;
    if (omx < 0)
        sx = -1;
    if (omy >= OMAPY)
        sy = 1;
    if (omy < 0)
        sy = -1;
    if (sx != 0 || sy != 0) {
        omx -= sx * OMAPX;
        omy -= sy * OMAPY;
        overmap tmp(this, cur_om.posx + sx, cur_om.posy + sy, 0);
        if (mark_as_seen) {
            tmp.seen(omx, omy) = true;
            tmp.save(tmp.posx, tmp.posy, cur_om.posz);
        } else {
            mark_as_seen = tmp.seen(omx, omy);
        }
        ret = tmp.ter(omx, omy);
    } else {
        ret = cur_om.ter(omx, omy);
        if (mark_as_seen)
            cur_om.seen(omx, omy) = true;
        else
            mark_as_seen = cur_om.seen(omx, omy);
    }
    return ret;
}

moncat_id Game::mt_to_mc(mon_id type)
{
    for (int i = 0; i < num_moncats; i++) {
        for (const auto& monster_id : moncats[i]) {
            if (monster_id == type) {
                return static_cast<moncat_id>(i);
            }
        }
    }

    return mcat_null;
}

nc_color sev(int a)
{
    switch (a) {
    case 0:
        return c_cyan;
    case 1:
        return c_blue;
    case 2:
        return c_green;
    case 3:
        return c_yellow;
    case 4:
        return c_ltred;
    case 5:
        return c_red;
    case 6:
        return c_magenta;
    }
    return c_dkgray;
}

void intro()
{
    int maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);
    while (maxy < 25 || maxx < 80) {
        erase();
        printw("\
Whoa. Whoa. Hey. This game requires a minimum terminal size of 80x25. I don't\n\
know why certain graphical terminal emulators decided to take the old standard\n\
size of 80x25 and toss it out the window, making their terminal 80x24 by\n\
default, but that just won't work here.  Now stretch the bottom of your window\n\
downward so you get an extra line.\n");
        getch();
        getmaxyx(stdscr, maxy, maxx);
    }
    erase();
}
} // namespace oocdda
