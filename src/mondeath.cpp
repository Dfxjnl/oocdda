#include <cmath>
#include <string>
#include <vector>

#include "mondeath.hpp"

#include "bodypart.hpp"
#include "enums.hpp"
#include "game.hpp"
#include "item.hpp"
#include "itype.hpp"
#include "map.hpp"
#include "mapdata.hpp"
#include "monster.hpp"
#include "monster_type.hpp"
#include "player.hpp"
#include "pldata.hpp"
#include "point.hpp"
#include "rng.hpp"

namespace oocdda {
void mdeath::normal(Game* g, Monster* z)
{
    int junk;
    if (g->u_see(z, junk))
        g->add_msg("It dies!");
    if (z->made_of(FLESH) && z->has_flag(MF_WARM)) {
        if (g->m.field_at(z->posx, z->posy).type == fd_blood
            && g->m.field_at(z->posx, z->posy).density < 3)
            g->m.field_at(z->posx, z->posy).density++;
        else
            g->m.add_field(g, z->posx, z->posy, fd_blood, 1);
    }
    // Drop a dang ol' corpse
    // If their hp is less than -50, we destroyed them so badly no corpse was left
    if ((z->hp >= -50 || z->hp >= 0 - z->type->hp) && (z->made_of(FLESH) || z->made_of(VEGGY))) {
        Item tmp;
        tmp.make_corpse(g->itypes[itm_corpse], z->type, g->turn);
        g->m.add_item(z->posx, z->posy, tmp);
    }
}

void mdeath::acid(Game* g, Monster* z)
{
    int tmp;
    if (g->u_see(z, tmp))
        g->add_msg("The %s's corpse melts into a pool of acid.", z->name().c_str());
    g->m.add_field(g, z->posx, z->posy, fd_acid, 3);
}

void mdeath::boomer(Game* g, Monster* z)
{
    std::string tmp;
    g->sound(z->posx, z->posy, 24, "a boomer explode!");
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->m.bash(z->posx + i, z->posy + j, 14, tmp);
            if (g->m.field_at(z->posx + i, z->posy + j).type == fd_bile
                && g->m.field_at(z->posx + i, z->posy + j).density < 3)
                g->m.field_at(z->posx + i, z->posy + j).density++;
            else
                g->m.add_field(g, z->posx + i, z->posy + j, fd_bile, 1);
        }
    }
    if (abs(z->posx - g->u.posx) <= 1 && abs(z->posy - g->u.posy) <= 1)
        g->u.infect(DI_BOOMERED, bp_eyes, 2, 24, g);
}

void mdeath::fungus(Game* g, Monster* z)
{
    Monster spore(g->mtypes[mon_spore]);
    int sporex, sporey;
    g->sound(z->posx, z->posy, 10, "Pouf!");
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            sporex = z->posx + i;
            sporey = z->posy + j;
            if (g->m.move_cost(sporex, sporey) > 0 && one_in(5)) {
                if (g->mon_at(sporex, sporey) >= 0) { // Spores hit a monster
                    if (g->u_see(sporex, sporey, j))
                        g->add_msg("The %s is covered in tiny spores!",
                                   g->z[g->mon_at(sporex, sporey)].name().c_str());
                    if (!g->z[g->mon_at(sporex, sporey)].make_fungus(g))
                        g->kill_mon(g->mon_at(sporex, sporey));
                } else if (g->u.posx == sporex && g->u.posy == sporey)
                    g->u.infect(DI_SPORES, bp_mouth, 4, 30, g);
                else {
                    spore.spawn(sporex, sporey);
                    g->z.push_back(spore);
                }
            }
        }
    }
}

void mdeath::fungusawake(Game* g, Monster* z)
{
    Monster newfung(g->mtypes[mon_fungaloid]);
    newfung.spawn(z->posx, z->posy);
    g->z.push_back(newfung);
}

void mdeath::worm(Game* g, Monster* z)
{
    int j;
    if (g->u_see(z, j))
        g->add_msg("The %s splits in two!", z->name().c_str());

    std::vector<Point> wormspots;
    int wormx, wormy;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            wormx = z->posx + i;
            wormy = z->posy + i;
            if (g->m.has_flag(diggable, wormx, wormy) && g->mon_at(wormx, wormy) == -1
                && !(g->u.posx == wormx && g->u.posy == wormy)) {
                wormspots.push_back(Point(wormx, wormy));
            }
        }
    }
    int rn;
    Monster worm(g->mtypes[mon_halfworm]);
    for (int worms = 0; worms < 2 && wormspots.size() > 0; worms++) {
        rn = rng(0, wormspots.size() - 1);
        worm.spawn(wormspots[rn].x, wormspots[rn].y);
        g->z.push_back(worm);
        wormspots.erase(wormspots.begin() + rn);
    }
}

void mdeath::disappear(Game* g, Monster* z)
{
    g->add_msg("The %s disappears!  Was it in your head?", z->name().c_str());
}

void mdeath::guilt(Game* g, Monster* z)
{
    if (g->u.has_trait(PF_HEARTLESS))
        return; // We don't give a shit!
    g->add_msg("You feel terrible for killing %s!", z->name().c_str());
    g->u.morale -= 5;
}

void mdeath::blobsplit(Game* g, Monster* z)
{
    int j;
    int speed = z->speed - rng(30, 50);
    if (speed <= 0) {
        if (g->u_see(z, j))
            g->add_msg("The %s splatters into tiny, dead pieces.", z->name().c_str());
        return;
    }
    Monster blob(g->mtypes[(speed < 50 ? mon_blob_small : mon_blob)]);
    blob.speed = speed;
    if (g->u_see(z, j))
        g->add_msg("The %s splits!", z->name().c_str());
    blob.hp = blob.speed;
    std::vector<Point> valid;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (g->m.move_cost(z->posx + i, z->posy + j) > 0
                && g->mon_at(z->posx + i, z->posy + j) == -1
                && (g->u.posx != z->posx + i || g->u.posy != z->posy + j))
                valid.push_back(Point(z->posx + i, z->posy + j));
        }
    }

    int rn;
    for (int s = 0; s < 2 && valid.size() > 0; s++) {
        rn = rng(0, valid.size() - 1);
        blob.spawn(valid[rn].x, valid[rn].y);
        g->z.push_back(blob);
        valid.erase(valid.begin() + rn);
    }
}

void mdeath::melt(Game* g, Monster* z)
{
    int j;
    if (g->u_see(z, j))
        g->add_msg("The %s melts away!", z->name().c_str());
}

void mdeath::explode(Game* g, Monster* z)
{
    int size {0};

    switch (z->type->size) {
    case MS_TINY:
        size = 4;
        break;
    case MS_SMALL:
        size = 8;
        break;
    case MS_MEDIUM:
        size = 14;
        break;
    case MS_LARGE:
        size = 20;
        break;
    case MS_HUGE:
        size = 26;
        break;
    }
    g->explosion(z->posx, z->posy, size, 0, false);
}
} // namespace oocdda
