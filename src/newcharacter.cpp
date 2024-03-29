#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <ncurses/curses.h>

#include "bionics.hpp"
#include "color.hpp"
#include "game.hpp"
#include "item.hpp"
#include "itype.hpp"
#include "keypress.hpp"
#include "output.hpp"
#include "player.hpp"
#include "pldata.hpp"
#include "rng.hpp"
#include "skill.hpp"

namespace oocdda {
#define LINE_XOXO 4194424
#define LINE_OXOX 4194417
#define LINE_XXOO 4194413
#define LINE_OXXO 4194412
#define LINE_OOXX 4194411
#define LINE_XOOX 4194410
#define LINE_XXXO 4194420
#define LINE_XXOX 4194422
#define LINE_XOXX 4194421
#define LINE_OXXX 4194423
#define LINE_XXXX 4194414

void draw_tabs(WINDOW* w);
int set_stats(WINDOW* w, player* u, int& points);
int set_traits(WINDOW* w, player* u, int& points);
int set_skills(WINDOW* w, player* u, int& points);
int set_description(WINDOW* w, player* u, int& points);

int random_good_trait(character_type type);
int random_bad_trait(character_type type);
int random_skill(character_type type);

bool player::create(Game* g, character_type type)
{
    WINDOW* w = newwin(25, 80, 0, 0);
    int tab = 0, points = 36;
    if (type != PLTYPE_CUSTOM) {
        switch (type) {
        case PLTYPE_RANDOM:
        default: {
            str_max = static_cast<int>(rng(6, 12));
            dex_max = static_cast<int>(rng(6, 12));
            int_max = static_cast<int>(rng(6, 12));
            per_max = static_cast<int>(rng(6, 12));
        } break;

        case PLTYPE_STUDENT:
            str_max = dice(2, 5) + 3;
            dex_max = dice(2, 5) + 3;
            int_max = dice(3, 4) + 4;
            per_max = dice(2, 5) + 3;
            break;
        case PLTYPE_FARMER:
            str_max = dice(4, 4) + 3;
            dex_max = dice(2, 5) + 4;
            int_max = dice(2, 5) + 3;
            per_max = dice(2, 5) + 3;
            break;
        case PLTYPE_MECHANIC:
            str_max = rng(6, 14);
            dex_max = dice(2, 6) + 3;
            int_max = dice(2, 6) + 3;
            per_max = dice(2, 7) + 3;
            sklevel[sk_mechanics] = 2;
            points--;
            break;
        case PLTYPE_CLERK:
            str_max = dice(2, 6) + 3;
            dex_max = dice(2, 6) + 3;
            int_max = dice(2, 6) + 3;
            per_max = dice(2, 6) + 3;
            sklevel[sk_barter] = 2;
            points--;
            break;
        case PLTYPE_COP:
            str_max = dice(3, 5) + 3;
            dex_max = dice(3, 5) + 3;
            int_max = dice(3, 3) + 2;
            per_max = dice(2, 6) + 4;
            break;
        case PLTYPE_SURVIVALIST:
            str_max = dice(2, 6) + 4;
            dex_max = dice(2, 6) + 4;
            int_max = dice(2, 6) + 3;
            per_max = dice(2, 6) + 4;
            break;
        case PLTYPE_PROGRAMMER:
            str_max = rng(5, 8);
            dex_max = dice(2, 4) + 3;
            int_max = dice(4, 4) + 4;
            per_max = dice(2, 5) + 3;
            sklevel[sk_computer] = 2;
            points--;
            break;
        case PLTYPE_DOCTOR:
            str_max = dice(2, 6) + 4;
            dex_max = dice(2, 6) + 4;
            int_max = dice(2, 7) + 5;
            per_max = dice(2, 6) + 4;
            sklevel[sk_firstaid] = 2;
            points--;
            break;
        }

        points = points - str_max - dex_max - int_max - per_max;
        int num_gtraits = 0, num_btraits = 0, rn, tries;
        while (points < 0 || rng(-3, 20) > points) {
            if (num_btraits < 5 && one_in(3)) {
                num_btraits++;
                tries = 0;
                do {
                    rn = random_bad_trait(type);
                    tries++;
                } while (has_trait(rn) && tries < 5);
                if (tries < 5) {
                    toggle_trait(rn);
                    points -= traits[rn].points;
                }
            } else {
                switch (rng(1, 4)) {
                case 1:
                    if (str_max > 5) {
                        str_max--;
                        points++;
                    }
                    break;
                case 2:
                    if (dex_max > 5) {
                        dex_max--;
                        points++;
                    }
                    break;
                case 3:
                    if (int_max > 5) {
                        int_max--;
                        points++;
                    }
                    break;
                case 4:
                    if (per_max > 5) {
                        per_max--;
                        points++;
                    }
                    break;
                }
            }
        }
        while (points > 0) {
            switch (rng((num_gtraits < 5 ? 1 : 5), 9)) {
            case 1:
            case 2:
            case 3:
            case 4:
                rn = random_good_trait(type);
                if (!has_trait(rn) && points >= traits[rn].points) {
                    toggle_trait(rn);
                    points -= traits[rn].points;
                }
                break;
            case 5:
                points--;
                switch (rng(1, 4)) {
                case 1:
                    str_max++;
                    break;
                case 2:
                    dex_max++;
                    break;
                case 3:
                    int_max++;
                    break;
                case 4:
                    per_max++;
                    break;
                }
                break;
            case 6:
            case 7:
            case 8:
            case 9:
                rn = random_skill(type);
                if (points >= sklevel[rn] + 1) {
                    points -= sklevel[rn] + 1;
                    sklevel[rn] += 2;
                }
                break;
            }
        }
        tab = 3;
    } else
        points = 4;

    do {
        werase(w);
        draw_tabs(w);
        wrefresh(w);
        switch (tab) {
        case 0:
            tab += set_stats(w, this, points);
            break;
        case 1:
            tab += set_traits(w, this, points);
            break;
        case 2:
            tab += set_skills(w, this, points);
            break;
        case 3:
            tab += set_description(w, this, points);
            break;
        }
    } while (tab >= 0 && tab < 4);
    delwin(w);

    if (tab < 0)
        return false;

    // Character is finalized.  Now just set up HP, &c
    for (int i = 0; i < num_hp_parts; i++) {
        hp_max[i] = 60 + str_max * 3;
        if (has_trait(PF_TOUGH))
            hp_max[i] = int(hp_max[i] * 1.2);
        hp_cur[i] = hp_max[i];
    }
    if (has_trait(PF_GLASSJAW)) {
        hp_max[hp_head] = int(hp_max[hp_head] * .85);
        hp_cur[hp_head] = hp_max[hp_head];
    }
    if (has_trait(PF_SMELLY))
        scent = 800;
    if (has_trait(PF_ANDROID)) {
        add_bionic(bionic_id(rng(bio_memory, max_bio_start - 1))); // Other
        if (bionics[my_bionics[0].id].power_cost > 0) {
            add_bionic(bionic_id(rng(0, bio_ethanol))); // Power Source
            max_power_level = 10;
            power_level = 10;
        } else {
            bionic_id tmpbio;
            do
                tmpbio = bionic_id(rng(bio_ethanol + 1, bio_armor_legs));
            while (bionics[tmpbio].power_cost > 0);
            add_bionic(tmpbio);
            max_power_level = 0;
            power_level = 0;
        }

        /* CHEATER'S STUFF
          add_bionic(bionic_id(rng(0, bio_ethanol)));	// Power Source
          for (int i = 0; i < 5; i++)
           add_bionic(bionic_id(rng(bio_memory, max_bio_start - 1)));// Other
          max_power_level = 80;
          power_level = 80;
        End of cheatery */
    }
    ret_null = Item(g->itypes[0], 0);
    weapon = Item(g->itypes[0], 0);
    // Nice to start out less than naked.
    worn.push_back(Item(g->itypes[itm_jeans], 0, 'a'));
    worn.push_back(Item(g->itypes[itm_tshirt], 0, 'b'));
    worn.push_back(Item(g->itypes[itm_sneakers], 0, 'c'));
    // The near-sighted get to start with glasses.
    if (has_trait(PF_MYOPIC))
        worn.push_back(Item(g->itypes[itm_glasses_eye], 0, 'd'));
    // Likewise, the asthmatic start with their medication.
    if (has_trait(PF_ASTHMA))
        inv.push_back(Item(g->itypes[itm_inhaler], 0, worn[worn.size() - 1].invlet + 1));
    return true;
}

void draw_tabs(WINDOW* w)
{
    for (int i = 0; i < 16; i++) {
        if (i < 9)
            mvwputch(w, 0, i + 5, c_ltgray, LINE_OXOX);
        if (i < 10) {
            mvwputch(w, 0, i + 22, c_ltgray, LINE_OXOX);
            mvwputch(w, 0, i + 40, c_ltgray, LINE_OXOX);
        }
        mvwputch(w, 0, i + 57, c_ltgray, LINE_OXOX);
    }
    mvwputch(w, 0, 4, c_ltgray, LINE_OXXO);
    mvwputch(w, 0, 21, c_ltgray, LINE_OXXO);
    mvwputch(w, 0, 39, c_ltgray, LINE_OXXO);
    mvwputch(w, 0, 57, c_ltgray, LINE_OXXO);

    mvwputch(w, 0, 14, c_ltgray, LINE_OOXX);
    mvwputch(w, 0, 32, c_ltgray, LINE_OOXX);
    mvwputch(w, 0, 50, c_ltgray, LINE_OOXX);
    mvwputch(w, 0, 73, c_ltgray, LINE_OOXX);

    mvwprintz(w, 1, 4, c_ltgray, "\
   STATS            TRAITS            SKILLS            DESCRIPTION   ");

    mvwputch(w, 1, 4, c_ltgray, LINE_XOXO);
    mvwputch(w, 1, 14, c_ltgray, LINE_XOXO);
    mvwputch(w, 1, 21, c_ltgray, LINE_XOXO);
    mvwputch(w, 1, 32, c_ltgray, LINE_XOXO);
    mvwputch(w, 1, 39, c_ltgray, LINE_XOXO);
    mvwputch(w, 1, 50, c_ltgray, LINE_XOXO);
    mvwputch(w, 1, 57, c_ltgray, LINE_XOXO);
    mvwputch(w, 1, 73, c_ltgray, LINE_XOXO);
}

int set_stats(WINDOW* w, player* u, int& points)
{
    unsigned char sel = 1;
    char ch;
    // Draw horizontal lines, with a gap for the active tab
    for (int i = 0; i < 80; i++) {
        if (i < 4 || i > 14)
            mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 4, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
    }
    // Attaching lines for tabs
    mvwputch(w, 1, 2, h_ltgray, '<');
    mvwputch(w, 1, 16, h_ltgray, '>');
    mvwputch(w, 2, 4, c_ltgray, LINE_XOOX);
    mvwputch(w, 2, 14, c_ltgray, LINE_XXOO);
    mvwputch(w, 2, 21, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 32, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 39, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 50, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 57, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 73, c_ltgray, LINE_XXOX);
    mvwprintz(w, 1, 5, h_ltgray, "  STATS  ");

    mvwprintz(w, 11, 0, c_ltgray, "\
   j/k, 8/2, or arrows select\n\
    a statistic.\n\
\n\
   l, 6, or right arrow\n\
    increases the statistic.\n\
\n\
   h, 4, or left arrow\n\
    decreases the statistic.\n\
\n\
   > Takes you to the next tab.");

    do {
        mvwprintz(w, 3, 2, c_ltgray, "Points left: %d  ", points);
        switch (sel) {
        case 1:
            mvwprintz(w, 6, 2, c_ltred, "Strength:     %d  ", u->str_max);
            mvwprintz(w, 7, 2, c_ltgray, "Dexterity:    %d  ", u->dex_max);
            mvwprintz(w, 8, 2, c_ltgray, "Intelligence: %d  ", u->int_max);
            mvwprintz(w, 9, 2, c_ltgray, "Perception:   %d  ", u->per_max);
            mvwprintz(w, 6, 33, c_ltred, " Strength affects your melee damage, the    ");
            mvwprintz(w, 7, 33, c_ltred, "amount of weight you can carry, your total  ");
            mvwprintz(w, 8, 33, c_ltred, "HP, your resistance to many diseases, and   ");
            mvwprintz(w, 9, 33, c_ltred, "the effectiveness of actions which require  ");
            mvwprintz(w, 10, 33, c_ltred, "brute force.");
            break;
        case 2:
            mvwprintz(w, 6, 2, c_ltgray, "Strength:     %d  ", u->str_max);
            mvwprintz(w, 7, 2, c_ltred, "Dexterity:    %d  ", u->dex_max);
            mvwprintz(w, 8, 2, c_ltgray, "Intelligence: %d  ", u->int_max);
            mvwprintz(w, 9, 2, c_ltgray, "Perception:   %d  ", u->per_max);
            mvwprintz(w, 6, 33, c_ltred, " Dexterity affects your chance to hit in    ");
            mvwprintz(w, 7, 33, c_ltred, "melee combat, helps you steady your gun for ");
            mvwprintz(w, 8, 33, c_ltred, "ranged combat, and enhances many actions    ");
            mvwprintz(w, 9, 33, c_ltred, "that require finesse.                       ");
            mvwprintz(w, 10, 33, c_ltred, "                                            ");
            break;
        case 3:
            mvwprintz(w, 6, 2, c_ltgray, "Strength:     %d  ", u->str_max);
            mvwprintz(w, 7, 2, c_ltgray, "Dexterity:    %d  ", u->dex_max);
            mvwprintz(w, 8, 2, c_ltred, "Intelligence: %d  ", u->int_max);
            mvwprintz(w, 9, 2, c_ltgray, "Perception:   %d  ", u->per_max);
            mvwprintz(w, 6, 33, c_ltred, " Intelligence is less important in most     ");
            mvwprintz(w, 7, 33, c_ltred, "situations, but it is vital for more complex");
            mvwprintz(w, 8, 33, c_ltred, "tasks like electronics crafting. It also    ");
            mvwprintz(w, 9, 33, c_ltred, "affects how much skill you can pick up from ");
            mvwprintz(w, 10, 33, c_ltred, "reading a book.                             ");
            break;
        case 4:
            mvwprintz(w, 6, 2, c_ltgray, "Strength:     %d  ", u->str_max);
            mvwprintz(w, 7, 2, c_ltgray, "Dexterity:    %d  ", u->dex_max);
            mvwprintz(w, 8, 2, c_ltgray, "Intelligence: %d  ", u->int_max);
            mvwprintz(w, 9, 2, c_ltred, "Perception:   %d  ", u->per_max);
            mvwprintz(w, 6, 33, c_ltred, " Perception is the most important trait for ");
            mvwprintz(w, 7, 33, c_ltred, "ranged combat. It's also used for detecting ");
            mvwprintz(w, 8, 33, c_ltred, "traps and other things of interest.         ");
            mvwprintz(w, 9, 33, c_ltred, "                                            ");
            mvwprintz(w, 10, 33, c_ltred, "                                            ");
            break;
        }

        wrefresh(w);
        ch = input();
        if (ch == 'j' && sel < 4)
            sel++;
        if (ch == 'k' && sel > 1)
            sel--;
        if (ch == 'h') {
            if (sel == 1 && u->str_max > 1) {
                u->str_max--;
                points++;
            } else if (sel == 2 && u->dex_max > 1) {
                u->dex_max--;
                points++;
            } else if (sel == 3 && u->int_max > 1) {
                u->int_max--;
                points++;
            } else if (sel == 4 && u->per_max > 1) {
                u->per_max--;
                points++;
            }
        }
        if (ch == 'l' && points > 0) {
            if (sel == 1 && u->str_max < 20) {
                points--;
                u->str_max++;
            } else if (sel == 2 && u->dex_max < 20) {
                points--;
                u->dex_max++;
            } else if (sel == 3 && u->int_max < 20) {
                points--;
                u->int_max++;
            } else if (sel == 4 && u->per_max < 20) {
                points--;
                u->per_max++;
            }
        }
        if (ch == '<' && query_yn("Return to main menu?"))
            return -1;
        if (ch == '>')
            return 1;
    } while (true);
}

int set_traits(WINDOW* w, player* u, int& points)
{
    // Draw horizontal lines, with a gap for the active tab
    for (int i = 0; i < 80; i++) {
        if (i < 21 || i > 32)
            mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 4, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
    }
    // Attaching lines for tabs
    mvwputch(w, 1, 19, h_ltgray, '<');
    mvwputch(w, 1, 34, h_ltgray, '>');
    mvwputch(w, 2, 4, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 14, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 21, c_ltgray, LINE_XOOX);
    mvwputch(w, 2, 32, c_ltgray, LINE_XXOO);
    mvwputch(w, 2, 39, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 50, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 57, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 73, c_ltgray, LINE_XXOX);
    mvwprintz(w, 1, 22, h_ltgray, "  TRAITS  ");

    for (int i = 0; i < 16; i++) {
        mvwprintz(w, 5 + i, 40, c_dkgray, "\
                                   ");
        mvwprintz(w, 5 + i, 40, c_dkgray, traits[PF_SPLIT + 1 + i].name.c_str());
    }
    mvwprintz(w, 11, 32, c_ltgray, "h   l");
    mvwprintz(w, 12, 32, c_ltgray, "<   >");
    mvwprintz(w, 13, 32, c_ltgray, "4   6");
    mvwprintz(w, 15, 32, c_ltgray, "Space");
    mvwprintz(w, 16, 31, c_ltgray, "Toggles");

    int cur_adv = 0, cur_dis = PF_SPLIT + 1, cur_trait, traitmin, traitmax, xoff;
    nc_color col_on, col_off, hi_on, hi_off;
    bool using_adv = true; // True if we're selecting advantages, false if we're
                           // selecting disadvantages

    do {
        mvwprintz(w, 3, 2, c_ltgray, "Points left: %d  ", points);
        mvwprintz(w, 3, 20, c_ltgray, "< h  Spacebar  l >");
        // Clear the bottom of the screen.
        mvwprintz(w, 22, 0, c_ltgray, "\
                                                                             ");
        mvwprintz(w, 23, 0, c_ltgray, "\
                                                                             ");
        mvwprintz(w, 24, 0, c_ltgray, "\
                                                                             ");
        if (using_adv) {
            col_on = c_green;
            col_off = c_ltgreen;
            hi_on = h_green;
            hi_off = h_ltgreen;
            xoff = 0;
            cur_trait = cur_adv;
            traitmin = 0;
            traitmax = PF_SPLIT;
            mvwprintz(w, 3, 40, c_ltgray, "                                       ");
            mvwprintz(w, 3, 40, c_ltgreen, "%s costs %d points", traits[cur_adv].name.c_str(),
                      traits[cur_adv].points);
            mvwprintz(w, 22, 0, (u->has_trait(cur_adv) ? c_green : c_ltgreen), "%s",
                      traits[cur_adv].description.c_str());
        } else {
            col_on = c_red;
            col_off = c_ltred;
            hi_on = h_red;
            hi_off = h_ltred;
            xoff = 40;
            cur_trait = cur_dis;
            traitmin = PF_SPLIT + 1;
            traitmax = PF_MAX;
            mvwprintz(w, 3, 40, c_ltgray, "                                       ");
            mvwprintz(w, 3, 40, c_ltred, "%s costs %d points", traits[cur_dis].name.c_str(),
                      traits[cur_dis].points);
            mvwprintz(w, 22, 0, (u->has_trait(cur_dis) ? c_red : c_ltred), "%s",
                      traits[cur_dis].description.c_str());
        }
        if (cur_trait <= traitmin + 7) {
            for (int i = traitmin; i < traitmin + 16; i++) {
                mvwprintz(w, 5 + i - traitmin, xoff, c_ltgray, "\
                                       "); // Clear the line
                if (i == cur_trait) {
                    if (u->has_trait(i))
                        mvwprintz(w, 5 + i - traitmin, xoff, hi_on, traits[i].name.c_str());
                    else
                        mvwprintz(w, 5 + i - traitmin, xoff, hi_off, traits[i].name.c_str());
                } else {
                    if (u->has_trait(i))
                        mvwprintz(w, 5 + i - traitmin, xoff, col_on, traits[i].name.c_str());
                    else
                        mvwprintz(w, 5 + i - traitmin, xoff, col_off, traits[i].name.c_str());
                }
            }
        } else if (cur_trait >= traitmax - 9) {
            for (int i = traitmax - 16; i < traitmax; i++) {
                mvwprintz(w, 21 + i - traitmax, xoff, c_ltgray, "\
                                       "); // Clear the line
                if (i == cur_trait) {
                    if (u->has_trait(i))
                        mvwprintz(w, 21 + i - traitmax, xoff, hi_on, traits[i].name.c_str());
                    else
                        mvwprintz(w, 21 + i - traitmax, xoff, hi_off, traits[i].name.c_str());
                } else {
                    if (u->has_trait(i))
                        mvwprintz(w, 21 + i - traitmax, xoff, col_on, traits[i].name.c_str());
                    else
                        mvwprintz(w, 21 + i - traitmax, xoff, col_off, traits[i].name.c_str());
                }
            }
        } else {
            for (int i = cur_trait - 7; i < cur_trait + 9; i++) {
                mvwprintz(w, 12 + i - cur_trait, xoff, c_ltgray, "\
                                      "); // Clear the line
                if (i == cur_trait) {
                    if (u->has_trait(i))
                        mvwprintz(w, 12 + i - cur_trait, xoff, hi_on, traits[i].name.c_str());
                    else
                        mvwprintz(w, 12 + i - cur_trait, xoff, hi_off, traits[i].name.c_str());
                } else {
                    if (u->has_trait(i))
                        mvwprintz(w, 12 + i - cur_trait, xoff, col_on, traits[i].name.c_str());
                    else
                        mvwprintz(w, 12 + i - cur_trait, xoff, col_off, traits[i].name.c_str());
                }
            }
        }

        wrefresh(w);
        switch (input()) {
        case 'h':
        case 'l':
        case '\t':
            if (!using_adv) {
                for (int i = 0; i < 16; i++) {
                    mvwprintz(w, 5 + i, 40, c_dkgray, "\
                                       ");
                    mvwprintz(w, 5 + i, 40, c_dkgray, traits[PF_SPLIT + 1 + i].name.c_str());
                }
            } else {
                for (int i = 0; i < 16; i++) {
                    mvwprintz(w, 5 + i, 0, c_dkgray, "\
                                       ");
                    mvwprintz(w, 5 + i, 0, c_dkgray, traits[i].name.c_str());
                }
            }
            using_adv = !using_adv;
            wrefresh(w);
            break;
        case 'k':
            if (using_adv) {
                if (cur_adv > 0)
                    cur_adv--;
            } else {
                if (cur_dis > PF_SPLIT + 1)
                    cur_dis--;
            }
            break;
        case 'j':
            if (using_adv) {
                if (cur_adv < PF_SPLIT - 1)
                    cur_adv++;
            } else {
                if (cur_dis < PF_MAX - 1)
                    cur_dis++;
            }
            break;
        case ' ':
        case '\n':
            if (u->has_trait(cur_trait)) {
                if (points + traits[cur_trait].points >= 0) {
                    u->toggle_trait(cur_trait);
                    points += traits[cur_trait].points;
                } else {
                    mvwprintz(w, 3, 2, c_red, "Points left: %d  ", points);
                }
            } else if (points >= traits[cur_trait].points) {
                u->toggle_trait(cur_trait);
                points -= traits[cur_trait].points;
            }
            break;
        case '<':
            return -1;
        case '>':
            return 1;
        }
    } while (true);
}

int set_skills(WINDOW* w, player* u, int& points)
{
    // Draw horizontal lines, with a gap for the active tab
    for (int i = 0; i < 80; i++) {
        if (i < 39 || i > 50)
            mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 4, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
    }
    // Attaching lines for tabs
    mvwputch(w, 1, 37, h_ltgray, '<');
    mvwputch(w, 1, 52, h_ltgray, '>');
    mvwputch(w, 2, 4, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 14, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 21, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 32, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 39, c_ltgray, LINE_XOOX);
    mvwputch(w, 2, 50, c_ltgray, LINE_XXOO);
    mvwputch(w, 2, 57, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 73, c_ltgray, LINE_XXOX);
    mvwprintz(w, 1, 40, h_ltgray, "  SKILLS  ");

    int cur_sk = 1;

    do {
        mvwprintz(w, 3, 2, c_ltgray, "Points left: %d  ", points);
        // Clear the bottom of the screen.
        mvwprintz(w, 22, 0, c_ltgray, "\
                                                                             ");
        mvwprintz(w, 23, 0, c_ltgray, "\
                                                                             ");
        mvwprintz(w, 24, 0, c_ltgray, "\
                                                                             ");
        if (points >= u->sklevel[cur_sk] + 1)
            mvwprintz(w, 3, 30, c_ltgreen, "Upgrading %s costs %d points         ",
                      skill_name(cur_sk).c_str(), u->sklevel[cur_sk] + 1);
        else
            mvwprintz(w, 3, 30, c_ltred, "Upgrading %s costs %d points         ",
                      skill_name(cur_sk).c_str(), u->sklevel[cur_sk] + 1);
        mvwprintz(w, 22, 0, c_ltgreen, skill_description(cur_sk).c_str());

        if (cur_sk <= 7) {
            for (int i = 1; i < 17; i++) {
                mvwprintz(w, 4 + i, 0, c_ltgray, "\
                                             "); // Clear the line
                if (u->sklevel[i] == 0) {
                    mvwprintz(w, 4 + i, 0, (i == cur_sk ? h_ltgray : c_ltgray),
                              skill_name(i).c_str());
                } else {
                    mvwprintz(w, 4 + i, 0, (i == cur_sk ? h_ltgreen : c_ltgreen), "%s ",
                              skill_name(i).c_str());
                    for (int j = 0; j < u->sklevel[i]; j++)
                        wprintz(w, (i == cur_sk ? h_ltgreen : c_ltgreen), "*");
                }
            }
        } else if (cur_sk >= num_skill_types - 9) {
            for (int i = num_skill_types - 16; i < num_skill_types; i++) {
                mvwprintz(w, 21 + i - num_skill_types, 0, c_ltgray, "\
                                             "); // Clear the line
                if (u->sklevel[i] == 0) {
                    mvwprintz(w, 21 + i - num_skill_types, 0, (i == cur_sk ? h_ltgray : c_ltgray),
                              skill_name(i).c_str());
                } else {
                    mvwprintz(w, 21 + i - num_skill_types, 0, (i == cur_sk ? h_ltgreen : c_ltgreen),
                              "%s ", skill_name(i).c_str());
                    for (int j = 0; j < u->sklevel[i]; j++)
                        wprintz(w, (i == cur_sk ? h_ltgreen : c_ltgreen), "*");
                }
            }
        } else {
            for (int i = cur_sk - 7; i < cur_sk + 9; i++) {
                mvwprintz(w, 12 + i - cur_sk, 0, c_ltgray, "\
                                             "); // Clear the line
                if (u->sklevel[i] == 0) {
                    mvwprintz(w, 12 + i - cur_sk, 0, (i == cur_sk ? h_ltgray : c_ltgray),
                              skill_name(i).c_str());
                } else {
                    mvwprintz(w, 12 + i - cur_sk, 0, (i == cur_sk ? h_ltgreen : c_ltgreen), "%s ",
                              skill_name(i).c_str());
                    for (int j = 0; j < u->sklevel[i]; j++)
                        wprintz(w, (i == cur_sk ? h_ltgreen : c_ltgreen), "*");
                }
            }
        }

        wrefresh(w);
        switch (input()) {
        case 'j':
            if (cur_sk < num_skill_types - 1)
                cur_sk++;
            break;
        case 'k':
            if (cur_sk > 1)
                cur_sk--;
            break;
        case 'h':
            if (u->sklevel[cur_sk] > 0) {
                points += u->sklevel[cur_sk] - 1;
                u->sklevel[cur_sk] -= 2;
            }
            break;
        case 'l':
            if (points >= u->sklevel[cur_sk] + 1) {
                points -= u->sklevel[cur_sk] + 1;
                u->sklevel[cur_sk] += 2;
            }
            break;
        case '<':
            return -1;
        case '>':
            return 1;
        }
    } while (true);
}

int set_description(WINDOW* w, player* u, int& points)
{
    // Draw horizontal lines, with a gap for the active tab
    for (int i = 0; i < 80; i++) {
        if (i < 57 || i > 73)
            mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 4, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
    }
    // Attaching lines for tabs
    mvwputch(w, 1, 55, h_ltgray, '<');
    mvwputch(w, 1, 75, h_ltgray, '>');
    mvwputch(w, 2, 4, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 14, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 21, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 32, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 39, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 50, c_ltgray, LINE_XXOX);
    mvwputch(w, 2, 57, c_ltgray, LINE_XOOX);
    mvwputch(w, 2, 73, c_ltgray, LINE_XXOO);
    mvwprintz(w, 1, 58, h_ltgray, "  DESCRIPTION  ");
    mvwprintz(w, 3, 2, c_ltgray, "Points left: %d  ", points);

    mvwprintz(w, 6, 2, c_ltgray, "\
Name: ______________________________     (Press TAB to move off this line)");
    mvwprintz(w, 8, 2, c_ltgray, "\
Gender: Male Female                      (Press spacebar to toggle)");
    mvwprintz(w, 10, 2, c_ltgray, "\
When your character is finished and you're ready to start playing, press '>'.");

    int line = 1;
    bool noname = false;
    long ch;

    do {
        if (u->male) {
            mvwprintz(w, 8, 10, c_ltred, "Male");
            mvwprintz(w, 8, 15, c_ltgray, "Female");
        } else {
            mvwprintz(w, 8, 10, c_ltgray, "Male");
            mvwprintz(w, 8, 15, c_ltred, "Female");
        }

        if (!noname) {
            mvwprintz(w, 6, 8, c_ltgray, u->name.c_str());
            if (line == 1)
                wprintz(w, h_ltgray, "_");
        }
        if (line == 2)
            mvwprintz(w, 8, 2, h_ltgray, "Gender:");
        else
            mvwprintz(w, 8, 2, c_ltgray, "Gender:");

        wrefresh(w);
        ch = input();
        if (noname) {
            mvwprintz(w, 6, 8, c_ltgray, "______________________________");
            noname = false;
        }

        if (ch == '>') {
            if (points > 0)
                mvwprintz(w, 3, 2, c_red, "\
Points left: %d    You must use the rest of your points!",
                          points);
            else if (u->name.size() == 0) {
                mvwprintz(w, 6, 8, h_ltgray, "______NO NAME ENTERED!!!!_____");
                noname = true;
                wrefresh(w);
            } else if (query_yn("Are you SURE you're finished?"))
                return 1;
            else
                refresh();
        } else if (ch == '<') {
            return -1;
        } else {
            switch (line) {
            case 1:
                if ((ch == KEY_BACKSPACE || ch == 127) && u->name.size() > 0) {
                    mvwprintz(w, 6, 8 + u->name.size(), c_ltgray, "_");
                    u->name.erase(u->name.end() - 1);
                } else if (ch == '\t') {
                    line = 2;
                    mvwprintz(w, 6, 8 + u->name.size(), c_ltgray, "_");
                } else if (((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == ' ')
                           && u->name.size() < 30) {
                    u->name.push_back(ch);
                }
                break;
            case 2:
                if (ch == ' ')
                    u->male = !u->male;
                else if (ch == 'k' || ch == '\t') {
                    line = 1;
                    mvwprintz(w, 8, 8, c_ltgray, ":");
                }
                break;
            }
        }
    } while (true);
}

int player::random_good_trait(character_type type)
{
    switch (type) {
    case PLTYPE_RANDOM:
    default: {
        return static_cast<int>(rng(1, pl_flag::PF_SPLIT - 1));
    } break;

    case character_type::PLTYPE_STUDENT: {
        switch (rng(1, 12)) {
        case 1:
            return PF_LIGHTEATER;
        case 2:
        case 3:
        case 4:
            return PF_FASTREADER;
        case 5:
            return PF_PACKMULE;

        case 6:
        case 7:
        case 8:
        case 9: {
            if (!has_trait(pl_flag::PF_SAVANT)) {
                return pl_flag::PF_FASTLEARNER;
            }
        } break;

        case 10:
            return PF_INCONSPICUOUS;
        case 11:
            return PF_LIGHTSTEP;
        case 12:
            if (one_in(3))
                return PF_ANDROID;
            else
                return PF_FASTLEARNER;
        }
    } break;

    case PLTYPE_FARMER: {
        switch (rng(1, 18)) {
        case 1:
            return PF_OPTIMISTIC;
        case 2:
            return PF_FASTHEALER;
        case 3:
        case 4:
            return PF_PAINRESIST;
        case 5:
        case 6:
            return PF_POISRESIST;
        case 7:
        case 8:
        case 9:
            return PF_TOUGH;
        case 10:
        case 11:
        case 12:
            return PF_THICKSKIN;
        case 13:
            return PF_GOURMAND;
        case 14:
        case 15:
        case 16:
            return PF_ANIMALEMPATH;
        case 17:
        case 18:
            return PF_DISRESISTANT;
        }
    } break;

    case character_type::PLTYPE_MECHANIC: {
        switch (rng(1, 13)) {
        case 1:
            return PF_QUICK;
        case 2:
            return PF_FASTHEALER;
        case 3:
            return PF_PAINRESIST;
        case 4:
            return PF_NIGHTVISION;
        case 5:
            return PF_TOUGH;
        case 6:
        case 7:
            return PF_THICKSKIN;
        case 8:
            return PF_PACKMULE;

        case 9: {
            if (!has_trait(pl_flag::PF_SAVANT)) {
                return pl_flag::PF_FASTLEARNER;
            }
        } break;

        case 10:
            return PF_DEFT;
        case 11:
            return PF_TERRIFYING;
        case 12:
            return PF_MASOCHIST;
        case 13:
            return PF_ANDROID;
        }
    } break;

    case character_type::PLTYPE_CLERK: {
        switch (rng(1, 16)) {
        case 1:
        case 2:
            return PF_PARKOUR;
        case 3:
            return PF_QUICK;
        case 4:
            return PF_OPTIMISTIC;
        case 5:
            return PF_LIGHTEATER;
        case 6:
            return PF_NIGHTVISION;
        case 7:
        case 8:
        case 9:
        case 10:
            return PF_PACKMULE;

        case 11: {
            if (!has_trait(pl_flag::PF_SAVANT)) {
                return pl_flag::PF_FASTLEARNER;
            }
        } break;

        case 12:
            return PF_DEFT;
        case 13:
            return PF_SUPERTASTER;
        case 14:
            return PF_DISRESISTANT;
        case 15:
            return PF_INCONSPICUOUS;
        case 16:
            return PF_LIGHTSTEP;
        }
    } break;

    case character_type::PLTYPE_COP: {
        switch (rng(1, 27)) {
        case 1:
        case 2:
        case 3:
            return PF_FLEET;
        case 4:
        case 5:
        case 6:
            return PF_PARKOUR;
        case 7:
        case 8:
            return PF_QUICK;
        case 9:
            return PF_PAINRESIST;
        case 10:
        case 11:
            return PF_NIGHTVISION;

        case 12: {
            if (!has_trait(pl_flag::PF_SAVANT)) {
                return pl_flag::PF_FASTLEARNER;
            }
        } break;

        case 13:
        case 14:
        case 15:
            return PF_DEFT;
        case 16:
            return PF_ANIMALEMPATH;
        case 17:
        case 18:
        case 19:
            return PF_TERRIFYING;
        case 20:
        case 21:
            return PF_ADRENALINE;
        case 22:
            return PF_LIGHTSTEP;
        case 23:
        case 24:
        case 25:
            return PF_HEARTLESS;
        case 26:
        case 27:
            return PF_ANDROID;
        }
    } break;

    case character_type::PLTYPE_SURVIVALIST: {
        switch (rng(1, 31)) {
        case 1:
        case 2:
        case 3:
            return PF_FLEET;
        case 4:
            return PF_PARKOUR;
        case 5:
            return PF_QUICK;
        case 6:
        case 7:
            return PF_OPTIMISTIC;
        case 8:
            return PF_FASTHEALER;
        case 9:
            return PF_LIGHTEATER;
        case 10:
            return PF_PAINRESIST;
        case 11:
        case 12:
            return PF_NIGHTVISION;
        case 13:
            return PF_POISRESIST;
        case 14:
        case 15:
            return PF_TOUGH;
        case 16:
            return PF_THICKSKIN;
        case 17:
        case 18:
        case 19:
            return PF_SUPERTASTER;
        case 20:
        case 21:
            return PF_ANIMALEMPATH;
        case 22:
            return PF_TERRIFYING;
        case 23:
        case 24:
            return PF_DISRESISTANT;
        case 25:
            return PF_ADRENALINE;
        case 26:
        case 27:
            return PF_INCONSPICUOUS;
        case 28:
        case 29:
        case 30:
            return PF_LIGHTSTEP;
        case 31:
            return PF_HEARTLESS;
        }
    } break;

    case character_type::PLTYPE_PROGRAMMER: {
        switch (rng(1, 19)) {
        case 1:
            return PF_OPTIMISTIC;
        case 2:
        case 3:
        case 4:
            return PF_LIGHTEATER;
        case 5:
        case 6:
        case 7:
        case 8:
            return PF_FASTREADER;

        case 9:
        case 10:
        case 11:
        case 12: {
            if (!has_trait(pl_flag::PF_SAVANT)) {
                return pl_flag::PF_FASTLEARNER;
            }
        } break;

        case 13:
            return PF_GOURMAND;
        case 14:
            return PF_SUPERTASTER;
        case 15:
        case 16:
            return PF_INCONSPICUOUS;
        case 17:
        case 18:
        case 19:
            return PF_ANDROID;
        }
    } break;

    case character_type::PLTYPE_DOCTOR: {
        switch (rng(1, 14)) {
        case 1:
        case 2:
            return PF_QUICK;
        case 3:
        case 4:
            return PF_FASTHEALER;
        case 5:
            return PF_PAINRESIST;
        case 6:
            return PF_POISRESIST;
        case 7:
            return PF_FASTREADER;

        case 8: {
            if (!has_trait(pl_flag::PF_SAVANT)) {
                return pl_flag::PF_FASTLEARNER;
            }
        } break;

        case 9:
            return PF_SUPERTASTER;
        case 10:
        case 11:
        case 12:
            return PF_DISRESISTANT;
        case 13:
            return PF_ADRENALINE;
        case 14:
            return PF_ANDROID;
        }
    } break;
    }

    return static_cast<int>(rng(1, PF_SPLIT - 1));
}

int player::random_bad_trait(character_type type)
{
    switch (type) {
    case PLTYPE_RANDOM:
    default: {
        return static_cast<int>(
            rng(static_cast<std::int64_t>(pl_flag::PF_SPLIT) + 1, pl_flag::PF_MAX - 1));
    } break;

    case PLTYPE_STUDENT: {
        switch (rng(1, 19)) {
        case 1:
        case 2:
        case 3:
            return PF_MYOPIC;
        case 4:
        case 5:
        case 6:
            return PF_HEAVYSLEEPER;
        case 7:
            return PF_BADBACK;
        case 8:
        case 9:
            return PF_INSOMNIA;
        case 10:
        case 11:
            return PF_VEGETARIAN;
        case 12:
            return PF_GLASSJAW;
        case 13:
        case 14:
            return PF_ADDICTIVE;
        case 15:
            return PF_SMELLY;
        case 16:
            return PF_CHEMIMBALANCE;

        case 17:
        case 18: {
            if (!has_trait(pl_flag::PF_FASTLEARNER)) {
                return pl_flag::PF_SAVANT;
            }
        } break;

        case 19:
            return PF_MOODSWINGS;
        }
    } break;

    case PLTYPE_FARMER: {
        switch (rng(1, 7)) {
        case 1:
            return PF_MYOPIC;
        case 2:
            return PF_BADBACK;
        case 3:
            return PF_ILLITERATE;
        case 4:
            return PF_BADHEARING;
        case 5:
            return PF_FORGETFUL;

        case 6:
        case 7:
            return PF_SMELLY;
        }
    } break;

    case PLTYPE_MECHANIC: {
        switch (rng(1, 14)) {
        case 1:
            return PF_HEAVYSLEEPER;
        case 2:
            return PF_ILLITERATE;
        case 3:
        case 4:
        case 5:
            return PF_BADHEARING;
        case 6:
            return PF_INSOMNIA;
        case 7:
            return PF_FORGETFUL;
        case 8:
            return PF_ADDICTIVE;
        case 9:
        case 10:
            return PF_TRIGGERHAPPY;
        case 11:
        case 12:
            return PF_SMELLY;
        case 13:
            return PF_CHEMIMBALANCE;
        case 14:
            return PF_HOARDER;
        }
    } break;

    case PLTYPE_CLERK: {
        switch (rng(1, 15)) {
        case 1:
            return PF_MYOPIC;
        case 2:
        case 3:
        case 4:
            return PF_HEAVYSLEEPER;
        case 5:
            return PF_ASTHMA;
        case 6:
            return PF_BADHEARING;
        case 7:
        case 8:
            return PF_INSOMNIA;
        case 9:
            return PF_VEGETARIAN;
        case 10:
            return PF_GLASSJAW;
        case 11:
        case 12:
            return PF_FORGETFUL;
        case 13:
        case 14:
            return PF_HOARDER;
        case 15:
            return PF_MOODSWINGS;
        }
    } break;

    case PLTYPE_COP: {
        switch (rng(1, 11)) {
        case 1:
        case 2:
        case 3:
        case 4:
            return PF_INSOMNIA;
        case 5:
            return PF_GLASSJAW;
        case 6:
            return PF_LIGHTWEIGHT;
        case 7:
        case 8:
            return PF_TRIGGERHAPPY;
        case 9:
            if (one_in(3))
                return PF_SCHIZOPHRENIC;
            else
                return PF_INSOMNIA;
        case 10:
            return PF_JITTERY;
        case 11:
            return PF_HPIGNORANT;
        }
    } break;

    case PLTYPE_SURVIVALIST: {
        switch (rng(1, 18)) {
        case 1:
            return PF_BADBACK;
        case 2:
            return PF_ILLITERATE;
        case 3:
            return PF_BADHEARING;
        case 4:
            return PF_INSOMNIA;
        case 5:
            return PF_FORGETFUL;
        case 6:
        case 7:
            return PF_ADDICTIVE;
        case 8:
        case 9:
        case 10:
            return PF_TRIGGERHAPPY;
        case 11:
        case 12:
        case 13:
            return PF_SMELLY;
        case 14:
            return PF_CHEMIMBALANCE;
        case 15:
            return PF_SCHIZOPHRENIC;
        case 16:
        case 17:
            return PF_HOARDER;
        case 18:
            return PF_HPIGNORANT;
        }
    } break;

    case PLTYPE_PROGRAMMER: {
        switch (rng(1, 36)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            return PF_MYOPIC;
        case 6:
            return PF_HEAVYSLEEPER;
        case 7:
        case 8:
        case 9:
        case 10:
            return PF_ASTHMA;
        case 11:
        case 12:
        case 13:
            return PF_BADBACK;
        case 14:
            return PF_BADHEARING;
        case 15:
            return PF_INSOMNIA;
        case 16:
        case 17:
        case 18:
            return PF_GLASSJAW;
        case 19:
        case 20:
        case 21:
            return PF_LIGHTWEIGHT;
        case 22:
        case 23:
        case 24:
            return PF_SMELLY;
        case 25:
            return PF_CHEMIMBALANCE;

        case 26:
        case 27:
        case 28: {
            if (!has_trait(pl_flag::PF_FASTLEARNER)) {
                return pl_flag::PF_SAVANT;
            }
        } break;

        case 29:
        case 30:
        case 31:
            return PF_WEAKSTOMACH;
        case 32:
        case 33:
        case 34:
            return PF_WOOLALLERGY;
        case 35:
        case 36:
            return PF_HPIGNORANT;
        }
    } break;

    case PLTYPE_DOCTOR: {
        switch (rng(1, 14)) {
        case 1:
        case 2:
            return PF_MYOPIC;
        case 3:
            return PF_ASTHMA;
        case 4:
            return PF_BADBACK;
        case 5:
        case 6:
            return PF_INSOMNIA;
        case 7:
        case 8:
            return PF_VEGETARIAN;
        case 9:
            return PF_GLASSJAW;
        case 10:
        case 11:
        case 12:
            return PF_ADDICTIVE;

        case 13: {
            if (!has_trait(pl_flag::PF_FASTLEARNER)) {
                return pl_flag::PF_SAVANT;
            }
        } break;

        case 14:
            return PF_WEAKSTOMACH;
        }
    } break;
    }

    return static_cast<int>(rng(static_cast<std::int64_t>(PF_SPLIT) + 1, PF_MAX - 1));
}

int random_skill(character_type type)
{
    switch (type) {
    case PLTYPE_RANDOM:
    default: {
        return static_cast<int>(rng(1, skill::num_skill_types - 1));
    } break;

    case PLTYPE_STUDENT: {
        switch (rng(1, 8)) {
        case 1:
        case 2:
            return sk_computer;
        case 3:
            return sk_mechanics;
        case 4:
            return sk_electronics;
        case 5:
            return sk_firstaid;
        case 6:
        case 7:
        case 8:
            return sk_speech;
        }
    } break;

    case PLTYPE_FARMER: {
        switch (rng(1, 15)) {
        case 1:
        case 2:
            return sk_melee;
        case 3:
            return sk_unarmed;
        case 4:
            return sk_bashing;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return sk_cutting;
        case 10:
            return sk_gun;
        case 11:
            return sk_mechanics;
        case 12:
        case 13:
            return sk_cooking;
        case 14:
            return sk_traps;
        case 15:
            return sk_barter;
        }
    } break;

    case PLTYPE_MECHANIC: {
        switch (rng(1, 12)) {
        case 1:
            return sk_melee;
        case 2:
        case 3:
        case 4:
            return sk_bashing;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return sk_mechanics;
        case 10:
        case 11:
            return sk_electronics;
        case 12:
            return sk_barter;
        }
    } break;

    case PLTYPE_CLERK: {
        switch (rng(1, 9)) {
        case 1:
            return sk_computer;
        case 2:
            return sk_electronics;
        case 3:
        case 4:
        case 5:
            return sk_speech;
        case 6:
        case 7:
        case 8:
        case 9:
            return sk_barter;
        }
    } break;

    case PLTYPE_COP: {
        switch (rng(1, 18)) {
        case 1:
            return sk_dodge;
        case 2:
            return sk_melee;
        case 3:
            return sk_unarmed;
        case 4:
            return sk_bashing;
        case 5:
        case 6:
        case 7:
            return sk_gun;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            return sk_pistol;
        case 13:
        case 14:
            return sk_shotgun;
        case 15:
            return sk_smg;
        case 16:
            return sk_rifle;
        case 17:
            return sk_firstaid;
        case 18:
            return sk_speech;
        }
    } break;

    case PLTYPE_SURVIVALIST: {
        switch (rng(1, 16)) {
        case 1:
            return sk_melee;
        case 2:
        case 3:
            return sk_unarmed;
        case 4:
        case 5:
        case 6:
            return sk_gun;
        case 7:
        case 8:
        case 9:
            return sk_rifle;
        case 10:
        case 11:
            return sk_cooking;
        case 12:
        case 13:
        case 14:
            return sk_traps;
        case 15:
        case 16:
            return sk_firstaid;
        }
    } break;

    case PLTYPE_PROGRAMMER: {
        switch (rng(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            return sk_computer;
        case 6:
            return sk_mechanics;
        case 7:
        case 8:
        case 9:
            return sk_electronics;
        case 10:
            return sk_cooking;
        }
    } break;

    case PLTYPE_DOCTOR: {
        switch (rng(1, 14)) {
        case 1:
        case 2:
            return sk_computer;
        case 3:
        case 4:
        case 5:
            return sk_cooking;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            return sk_firstaid;
        case 13:
        case 14:
            return sk_speech;
        }
    } break;
    }

    return static_cast<int>(rng(1, num_skill_types - 1));
}
} // namespace oocdda
