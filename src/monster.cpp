#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "monster.hpp"

#include <ncurses/curses.h>

#include "color.hpp"
#include "game.hpp"
#include "itype.hpp"
#include "map.hpp"
#include "mapdata.hpp"
#include "mapitems.hpp"
#include "mondeath.hpp"
#include "output.hpp"
#include "player.hpp"
#include "pldata.hpp"
#include "rng.hpp"

namespace oocdda {
#define SGN(a) (((a) < 0) ? -1 : 1)
#define SQR(a) (a * a)

Monster::Monster()
{
    /*
     mvprintz(10, 30, c_red, "INVALID MONSTER!");
     getch();
    */
    posx = 20;
    posy = 10;
    wandx = -1;
    wandy = -1;
    wandf = 0;
    hp = 60;
    moves = 0;
    sp_timeout = 0;
    spawnmapx = -1;
    spawnmapy = -1;
    spawnposx = -1;
    spawnposy = -1;
    friendly = 0;
    dead = false;
}

Monster::Monster(MonsterType* t)
{
    posx = 20;
    posy = 10;
    wandx = -1;
    wandy = -1;
    wandf = 0;
    type = t;
    moves = type->speed;
    speed = type->speed;
    hp = type->hp;
    sp_timeout = rng(0, type->sp_freq);
    spawnmapx = -1;
    spawnmapy = -1;
    spawnposx = -1;
    spawnposy = -1;
    friendly = 0;
    dead = false;
}

Monster::Monster(MonsterType* t, int x, int y)
{
    posx = x;
    posy = y;
    wandx = -1;
    wandy = -1;
    wandf = 0;
    type = t;
    moves = type->speed;
    speed = type->speed;
    hp = type->hp;
    sp_timeout = type->sp_freq;
    spawnmapx = -1;
    spawnmapy = -1;
    spawnposx = -1;
    spawnposy = -1;
    friendly = 0;
    dead = false;
}

void Monster::poly(MonsterType* t)
{
    type = t;
    moves = 0;
    speed = type->speed;
    hp = type->hp;
    sp_timeout = type->sp_freq;
}

void Monster::spawn(int x, int y)
{
    posx = x;
    posy = y;
}

std::string Monster::name() { return type->name; }

void Monster::print_info(Game* g, WINDOW* w)
{
    // First line of w is the border; the next two are terrain info, and after that
    // is a blank line. w is 13 characters tall, and we can't use the last one
    // because it's a border as well; so we have lines 4 through 11.
    // w is also 48 characters wide - 2 characters for border = 46 characters for us
    mvwprintz(w, 6, 1, type->color, type->name.c_str());
    if (friendly != 0) {
        wprintz(w, c_white, " ");
        wprintz(w, h_white, "Friendly!");
    }
    std::string damage_info;
    nc_color col;
    if (hp == type->hp) {
        damage_info = "It is uninjured";
        col = c_green;
    } else if (hp >= type->hp * .8) {
        damage_info = "It is lightly injured";
        col = c_ltgreen;
    } else if (hp >= type->hp * .6) {
        damage_info = "It is moderately injured";
        col = c_yellow;
    } else if (hp >= type->hp * .3) {
        damage_info = "It is heavily injured";
        col = c_yellow;
    } else if (hp >= type->hp * .1) {
        damage_info = "It is severly injured";
        col = c_ltred;
    } else {
        damage_info = "it is nearly dead";
        col = c_red;
    }
    mvwprintz(w, 7, 1, col, damage_info.c_str());
    if (is_fleeing(g->u))
        wprintz(w, c_white, ", and it is fleeing.");
    else
        wprintz(w, col, ".");

    std::string tmp = type->description;
    std::string out;
    size_t pos;
    int line = 8;
    do {
        pos = tmp.find_first_of('\n');
        out = tmp.substr(0, pos);
        mvwprintz(w, line, 1, c_white, out.c_str());
        tmp = tmp.substr(pos + 1);
        line++;
    } while (pos != std::string::npos && line < 12);
}

char Monster::symbol() { return type->sym; }

void Monster::draw(WINDOW* w, int plx, int ply, bool inv)
{
    int x = SEEX + posx - plx;
    int y = SEEY + posy - ply;
    nc_color color = type->color;
    if (friendly != 0 && !inv)
        color = hilite(color);
    if (inv)
        mvwputch_inv(w, y, x, color, type->sym);
    else
        mvwputch(w, y, x, color, type->sym);
}

bool Monster::has_flag(m_flags f) { return type->flags & flag_to_bit_position(f); }

bool Monster::made_of(material m)
{
    if (type->mat == m)
        return true;
    return false;
}

void Monster::load_info(std::string data, std::vector<MonsterType*>* mtypes)
{
    std::stringstream dump;
    int idtmp, plansize;
    dump << data;
    dump >> idtmp >> posx >> posy >> wandx >> wandy >> wandf >> moves >> speed >> hp >> sp_timeout
        >> plansize >> friendly >> dead;
    type = (*mtypes)[idtmp];
    Point ptmp;
    for (int i = 0; i < plansize; i++) {
        dump >> ptmp.x >> ptmp.y;
        plans.push_back(ptmp);
    }
}

std::string Monster::save_info()
{
    std::stringstream pack;
    pack << int(type->id) << " " << posx << " " << posy << " " << wandx << " " << wandy << " "
         << wandf << " " << moves << " " << speed << " " << hp << " " << sp_timeout << " "
         << plans.size() << " " << friendly << " " << dead;
    for (int i = 0; i < plans.size(); i++) {
        pack << " " << plans[i].x << " " << plans[i].y;
    }
    return pack.str();
}

void Monster::debug(player& u)
{
    char buff[2];
    debugmsg("%s has %d steps planned.", name().c_str(), plans.size());
    debugmsg("%s Moves %d Speed %d HP %d", name().c_str(), moves, speed, hp);
    for (int i = 0; i < plans.size(); i++) {
        sprintf(buff, "%d", i);
        if (i < 10)
            mvaddch(plans[i].y - SEEY + u.posy, plans[i].x - SEEX + u.posx, buff[0]);
        else
            mvaddch(plans[i].y - SEEY + u.posy, plans[i].x - SEEX + u.posx, buff[1]);
    }
    getch();
}

void Monster::shift(int sx, int sy)
{
    posx -= sx * SEEX;
    posy -= sy * SEEY;
    for (int i = 0; i < plans.size(); i++) {
        plans[i].x -= sx * SEEX;
        plans[i].y -= sy * SEEY;
    }
}

bool Monster::is_fleeing(player& u)
{
    // fleefactor is by default the agressiveness of the animal, minus the
    //  percentage of remaining HP times five.  So, aggresiveness of 5 has a
    //  fleefactor of 2 AT MINIMUM.
    if (type->hp == 0)
        debugmsg("%s has type->hp of 0!", type->name.c_str());

    if (friendly != 0)
        return false;

    int fleefactor = type->agro - ((4 * (type->hp - hp)) / type->hp);
    if (u.has_trait(PF_ANIMALEMPATH) && has_flag(MF_ANIMAL)) {
        if (type->agro > 0) // Agressive animals flee instead
            fleefactor -= 5;
        else // Scared animals approach you
            return false;
    }
    if (u.has_trait(PF_TERRIFYING))
        fleefactor -= 1;
    if (fleefactor > 0)
        return false;
    return true;
}

int Monster::hit(player& p, body_part& bp_hit)
{
    int numdice = type->melee_skill;
    if (dice(numdice, 10) <= dice(p.dodge(), 10))
        return 0; // We missed!
    int ret = 0;
    int highest_hit;
    switch (type->size) {
    case MS_TINY:
        highest_hit = 3;
        break;
    case MS_SMALL:
        highest_hit = 12;
        break;
    case MS_MEDIUM:
        highest_hit = 20;
        break;
    case MS_LARGE:
        highest_hit = 28;
        break;
    case MS_HUGE:
        highest_hit = 35;
        break;
    }

    if (has_flag(MF_DIGS))
        highest_hit -= 8;
    if (has_flag(MF_FLIES))
        highest_hit += 15;
    if (highest_hit <= 1)
        highest_hit = 2;
    if (highest_hit > 20)
        highest_hit = 20;

    int bp_rand = rng(0, highest_hit - 1);
    if (bp_rand <= 2)
        bp_hit = bp_legs;
    else if (bp_rand <= 10)
        bp_hit = bp_torso;
    else if (bp_rand <= 14)
        bp_hit = bp_arms;
    else if (bp_rand <= 16)
        bp_hit = bp_mouth;
    else if (bp_rand == 17)
        bp_hit = bp_eyes;
    else
        bp_hit = bp_head;
    ret += dice(type->melee_dice, type->melee_sides);
    return ret;
}

void Monster::hit_monster(Game* g, int i)
{
    int junk;
    Monster* target = &(g->z[i]);

    int numdice = type->melee_skill;
    int dodgedice = target->type->sk_dodge;
    switch (target->type->size) {
    case MS_TINY:
        dodgedice += 2;
        break;
    case MS_SMALL:
        dodgedice += 1;
        break;
    case MS_LARGE:
        numdice += 1;
        break;
    case MS_HUGE:
        numdice += 3;
        break;
    }

    if (dice(numdice, 10) <= dice(dodgedice, 10)) {
        if (g->u_see(this, junk))
            g->add_msg("The %s misses the %s!", name().c_str(), target->name().c_str());
        return;
    }
    if (g->u_see(this, junk))
        g->add_msg("The %s hits the %s!", name().c_str(), target->name().c_str());
    int damage = dice(type->melee_dice, type->melee_sides);
    if (target->hurt(damage))
        g->kill_mon(i);
}

bool Monster::hurt(int dam)
{
    hp -= dam;
    if (hp < 1)
        return true;
    return false;
}

int Monster::armor()
{
    // TODO: Add support for worn armor?
    return int(type->armor);
}

void Monster::die(Game* g)
{
    // Drop goodies
    int total_chance = 0, total_it_chance, cur_chance, selected_location, selected_item;
    bool animal_done = false;
    std::vector<items_location_and_chance> it = g->monitems[type->id];
    std::vector<itype_id> mapit;
    if (type->item_chance != 0 && g->monitems[type->id].size() == 0)
        debugmsg("Type %s has item_chance %d but no items assigned!", type->name.c_str(),
                 type->item_chance);
    for (int i = 0; i < it.size(); i++)
        total_chance += it[i].chance;

    while (rng(0, 99) < abs(type->item_chance) && !animal_done) {
        cur_chance = rng(1, total_chance);
        selected_location = -1;
        while (cur_chance > 0) {
            selected_location++;
            cur_chance -= it[selected_location].chance;
        }
        total_it_chance = 0;
        mapit = g->mapitems[it[selected_location].loc];
        for (int i = 0; i < mapit.size(); i++)
            total_it_chance += g->itypes[mapit[i]]->rarity;
        cur_chance = rng(1, total_it_chance);
        selected_item = -1;
        while (cur_chance > 0) {
            selected_item++;
            cur_chance -= g->itypes[mapit[selected_item]]->rarity;
        }
        g->m.add_item(posx, posy, g->itypes[mapit[selected_item]], 0);
        if (type->item_chance < 0)
            animal_done = true; // Only drop ONE item.
    }

    // Also, perform our death function
    mdeath md;
    (md.*type->dies)(g, this);
}

bool Monster::make_fungus(Game* g)
{
    switch (mon_id(type->id)) {
    case mon_ant:
    case mon_ant_soldier:
    case mon_bee:
        poly(g->mtypes[mon_ant_fungus]);
        return true;
    case mon_zombie:
    case mon_zombie_shrieker:
    case mon_zombie_electric:
    case mon_zombie_spitter:
    case mon_zombie_fast:
    case mon_zombie_brute:
    case mon_zombie_hulk:
        poly(g->mtypes[mon_zombie_fungus]);
        return true;
    case mon_boomer:
        poly(g->mtypes[mon_boomer_fungus]);
        return true;
    case mon_triffid:
    case mon_triffid_young:
    case mon_triffid_queen:
        poly(g->mtypes[mon_fungaloid]);
        return true;
    case mon_fungaloid:
    case mon_spore:
    case mon_ant_fungus:
    case mon_zombie_fungus:
    case mon_boomer_fungus:
        return true;
    default:
        return false; // All others die
    }
    return false;
}

void Monster::make_friendly()
{
    plans.clear();
    friendly = rng(5, 30) + rng(0, 20);
}
} // namespace oocdda
