#include <array>
#include <cstddef>
#include <vector>

#include "bionics.hpp"
#include "bodypart.hpp"
#include "enums.hpp"
#include "game.hpp"
#include "item.hpp"
#include "itype.hpp"
#include "map.hpp"
#include "mapdata.hpp"
#include "monster.hpp"
#include "monster_type.hpp"
#include "output.hpp"
#include "player.hpp"
#include "pldata.hpp"
#include "point.hpp"
#include "rng.hpp"

namespace oocdda {
bool Map::process_fields(Game* g)
{
    bool found_field = false;
    field* cur;
    field_id curtype;
    for (int x = 0; x < SEEX * 3; x++) {
        for (int y = 0; y < SEEY * 3; y++) {
            cur = &field_at(x, y);
            curtype = cur->type;
            if (!found_field && curtype != fd_null)
                found_field = true;
            if (cur->density > 3)
                debugmsg("Whoooooa density of %d", cur->density);

            if (cur->age == 0) // Don't process "newborn" fields
                curtype = fd_null;

            switch (curtype) {
            case fd_null:
            default:
                break;

            case fd_blood:
            case fd_bile:
                if (has_flag(swimmable, x, y)) // Dissipate faster in water
                    cur->age += 250;
                break;

            case fd_acid:
                if (has_flag(swimmable, x, y)) // Dissipate faster in water
                    cur->age += 20;

                for (std::size_t i {0}; i < i_at(x, y).size(); ++i) {
                    if (i_at(x, y)[i].made_of(material::LIQUID)
                        || i_at(x, y)[i].made_of(material::VEGGY)
                        || i_at(x, y)[i].made_of(material::FLESH)
                        || i_at(x, y)[i].made_of(material::POWDER)
                        || i_at(x, y)[i].made_of(material::COTTON)
                        || i_at(x, y)[i].made_of(material::WOOL)
                        || i_at(x, y)[i].made_of(material::PAPER)
                        || i_at(x, y)[i].made_of(material::PLASTIC)
                        || i_at(x, y)[i].made_of(material::GLASS)) {
                        // Acid-destructable.
                        cur->age += static_cast<int>(i_at(x, y)[i].volume());

                        for (const auto& content : i_at(x, y)[i].contents) {
                            i_at(x, y).push_back(content);
                        }

                        i_at(x, y).erase(i_at(x, y).begin() + static_cast<std::ptrdiff_t>(i));
                        --i;
                    }
                }

                break;

            case fd_fire:
                // Consume items as fuel to help us grow/last longer.
                bool destroyed;
                int vol;

                for (std::size_t i {0}; i < i_at(x, y).size(); ++i) {
                    destroyed = false;
                    vol = i_at(x, y)[i].volume();
                    if (i_at(x, y)[i].is_ammo()) {
                        cur->age /= 2;
                        cur->age -= 300;
                        destroyed = true;
                    } else if (i_at(x, y)[i].made_of(PAPER)) {
                        cur->age -= vol * 10;
                        destroyed = true;
                    } else if ((i_at(x, y)[i].made_of(WOOD) || i_at(x, y)[i].made_of(VEGGY))
                               && (vol <= cur->density * 10
                                           - (cur->age > 0 ? rng(0, cur->age / 10) : 0)
                                   || cur->density == 3)) {
                        cur->age -= vol * 10;
                        destroyed = true;
                    } else if ((i_at(x, y)[i].made_of(COTTON) || i_at(x, y)[i].made_of(FLESH)
                                || i_at(x, y)[i].made_of(WOOL))
                               && (vol <= cur->density * 2 || (cur->density == 3 && one_in(vol)))) {
                        cur->age -= vol * 5;
                        destroyed = true;
                    } else if (i_at(x, y)[i].made_of(LIQUID) || i_at(x, y)[i].made_of(POWDER)
                               || i_at(x, y)[i].made_of(PLASTIC)
                               || (cur->density >= 2 && i_at(x, y)[i].made_of(GLASS))
                               || (cur->density == 3 && i_at(x, y)[i].made_of(IRON))) {
                        switch (i_at(x, y)[i].type->id) { // TODO: Make this be not a hack.
                        case itm_whiskey:
                        case itm_vodka:
                        case itm_rum:
                        case itm_tequila:
                            cur->age -= 220;
                            break;
                        }
                        destroyed = true;
                    }
                    if (destroyed) {
                        for (const auto& content : i_at(x, y)[i].contents) {
                            i_at(x, y).push_back(content);
                        }

                        i_at(x, y).erase(i_at(x, y).begin() + i);
                        i--;
                    }
                }

                // Consume the terrain we're on
                if (terlist[ter(x, y)].flags & flag_to_bit_position(flammable)
                    && one_in(8 - cur->density)) {
                    cur->age -= cur->density * cur->density * 40;
                    if (cur->density == 3)
                        ter(x, y) = t_rubble;
                } else if (terlist[ter(x, y)].flags & flag_to_bit_position(explodes)) {
                    ter(x, y) = t_gas_pump_smashed;
                    cur->age = 0;
                    cur->density = 3;
                    g->explosion(x, y, 40, 0, true);
                } else if (terlist[ter(x, y)].flags & flag_to_bit_position(swimmable))
                    cur->age += 800; // Flames die quickly on water
                // If we consumed a lot, the flames grow higher
                while (cur->density < 3 && cur->age < 0) {
                    cur->age += 300;
                    cur->density++;
                }
                // If the flames are REALLY big, they contribute to adjacent flames
                if (cur->density == 3 && cur->age < 0) {
                    // Randomly offset our x/y shifts by 0-2, to randomly pick a square to spread to
                    int starti = rng(0, 2);
                    int startj = rng(0, 2);
                    for (int i = 0; i < 3 && cur->age < 0; i++) {
                        for (int j = 0; j < 3 && cur->age < 0; j++) {
                            if (field_at(x + ((i + starti) % 3), y + ((j + startj) % 3)).type
                                    == fd_fire
                                && field_at(x + ((i + starti) % 3), y + ((j + startj) % 3)).density
                                    < 3) {
                                field_at(x + ((i + starti) % 3), y + ((j + startj) % 3)).density++;
                                field_at(x + ((i + starti) % 3), y + ((j + startj) % 3)).age = 0;
                                cur->age = 0;
                            }
                        }
                    }
                }
                // Consume adjacent fuel / terrain to spread.
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (x + i >= 0 && y + j >= 0 && x + i < SEEX * 3 && y + j <= SEEY * 3) {
                            if (has_flag(explodes, x + i, y + j) && one_in(8 - cur->density)) {
                                ter(x + i, y + j) = t_gas_pump_smashed;
                                g->explosion(x + i, y + j, 40, 0, true);
                            } else if ((i != 0 || j != 0)
                                       && (i_at(x + i, y + j).size() > 0
                                           || rng(15, 120) < cur->density * 10)) {
                                if (field_at(x + i, y + j).type == fd_smoke)
                                    field_at(x + i, y + j) = field(fd_fire, 1, 0);
                                else
                                    add_field(g, x + i, y + j, fd_fire, 1);
                                // If we're not spreading, maybe we'll stick out some smoke, huh?
                            } else if (move_cost(x + i, y + j) > 0 && rng(7, 40) < cur->density * 10
                                       && cur->age < 1000) {
                                add_field(g, x + i, y + j, fd_smoke, rng(1, cur->density));
                            }
                        }
                    }
                }
                break;

            case fd_smoke:
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++)
                        g->scent(x + i, y + j) = 0;
                }
                if (one_in(3)) {
                    std::vector<Point> spread;
                    for (int a = -1; a <= 1; a++) {
                        for (int b = -1; b <= 1; b++) {
                            if ((field_at(x + a, y + b).type == fd_smoke
                                 && field_at(x + a, y + b).density < 3)
                                || (field_at(x + a, y + b).is_null()
                                    && move_cost(x + a, y + b) > 0))
                                spread.push_back(Point(x + a, y + b));
                        }
                    }
                    if (cur->density > 0 && cur->age > 0 && spread.size() > 0) {
                        Point p = spread[rng(0, spread.size() - 1)];
                        if (field_at(p.x, p.y).type == fd_smoke && field_at(p.x, p.y).density < 3) {
                            field_at(p.x, p.y).density++;
                            cur->density--;
                        } else if (cur->density > 0 && move_cost(p.x, p.y) > 0
                                   && add_field(g, p.x, p.y, fd_smoke, 1)) {
                            cur->density--;
                            field_at(p.x, p.y).age = cur->age;
                        }
                    }
                }
                break;

            case fd_tear_gas:
                // Reset nearby scents to zero
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++)
                        g->scent(x + i, y + j) = 0;
                }
                // One in three chance that it spreads (less than smoke!)
                if (one_in(3)) {
                    std::vector<Point> spread;
                    // Pick all eligible points to spread to
                    for (int a = -1; a <= 1; a++) {
                        for (int b = -1; b <= 1; b++) {
                            if (((field_at(x + a, y + b).type == fd_smoke
                                  || field_at(x + a, y + b).type == fd_tear_gas)
                                 && field_at(x + a, y + b).density < 3)
                                || (field_at(x + a, y + b).is_null()
                                    && move_cost(x + a, y + b) > 0))
                                spread.push_back(Point(x + a, y + b));
                        }
                    }
                    // Then, spread to a nearby point
                    if (cur->density > 0 && cur->age > 0 && spread.size() > 0) {
                        Point p = spread[rng(0, spread.size() - 1)];
                        // Nearby teargas grows thicker
                        if (field_at(p.x, p.y).type == fd_tear_gas
                            && field_at(p.x, p.y).density < 3) {
                            field_at(p.x, p.y).density++;
                            cur->density--;
                            // Nearby smoke is converted into teargas
                        } else if (field_at(p.x, p.y).type == fd_smoke) {
                            field_at(p.x, p.y).type = fd_tear_gas;
                            // Or, just create a new field.
                        } else if (cur->density > 0 && move_cost(p.x, p.y) > 0
                                   && add_field(g, p.x, p.y, fd_tear_gas, 1)) {
                            cur->density--;
                            field_at(p.x, p.y).age = cur->age;
                        }
                    }
                }
                break;

            case fd_nuke_gas:
                // Reset nearby scents to zero
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++)
                        g->scent(x + i, y + j) = 0;
                }
                // Increase long-term radiation in the land underneath
                radiation(x, y) += rng(0, cur->density);
                if (one_in(2)) {
                    std::vector<Point> spread;
                    // Pick all eligible points to spread to
                    for (int a = -1; a <= 1; a++) {
                        for (int b = -1; b <= 1; b++) {
                            if (((field_at(x + a, y + b).type == fd_smoke
                                  || field_at(x + a, y + b).type == fd_tear_gas
                                  || field_at(x + a, y + b).type == fd_nuke_gas)
                                 && field_at(x + a, y + b).density < 3)
                                || (field_at(x + a, y + b).is_null()
                                    && move_cost(x + a, y + b) > 0))
                                spread.push_back(Point(x + a, y + b));
                        }
                    }
                    // Then, spread to a nearby point
                    if (cur->density > 0 && cur->age > 0 && spread.size() > 0) {
                        Point p = spread[rng(0, spread.size() - 1)];
                        // Nearby nukegas grows thicker
                        if (field_at(p.x, p.y).type == fd_nuke_gas
                            && field_at(p.x, p.y).density < 3) {
                            field_at(p.x, p.y).density++;
                            cur->density--;
                            // Nearby smoke & teargas is converted into nukegas
                        } else if (field_at(p.x, p.y).type == fd_smoke
                                   || field_at(p.x, p.y).type == fd_tear_gas) {
                            field_at(p.x, p.y).type = fd_nuke_gas;
                            // Or, just create a new field.
                        } else if (cur->density > 0 && move_cost(p.x, p.y) > 0
                                   && add_field(g, p.x, p.y, fd_nuke_gas, 1)) {
                            cur->density--;
                            field_at(p.x, p.y).age = cur->age;
                        }
                    }
                }
                break;
            case fd_electricity:
                if (!one_in(5)) { // 4 in 5 chance to spread
                    std::vector<Point> valid;
                    if (move_cost(x, y) == 0 && cur->density > 1) { // We're grounded
                        int tries = 0;
                        while (tries < 10 && cur->age < 50) {
                            int cx = x + rng(-1, 1), cy = y + rng(-1, 1);
                            if (move_cost(cx, cy) != 0 && field_at(cx, cy).is_null()) {
                                add_field(g, cx, cy, fd_electricity, 1);
                                cur->density--;
                                tries = 0;
                            } else
                                tries++;
                        }
                    } else { // We're not grounded; attempt to ground
                        for (int a = -1; a <= 1; a++) {
                            for (int b = -1; b <= 1; b++) {
                                if (move_cost(x + a, y + b) == 0 && // Grounded tiles first
                                    field_at(x + a, y + b).is_null())
                                    valid.push_back(Point(x + a, y + b));
                            }
                        }
                        if (valid.size() == 0) { // Spread to adjacent space, then
                            int px = x + rng(-1, 1), py = y + rng(-1, 1);
                            if (move_cost(px, py) > 0 && field_at(px, py).type == fd_electricity
                                && field_at(px, py).density < 3)
                                field_at(px, py).density++;
                            else if (move_cost(px, py) > 0)
                                add_field(g, px, py, fd_electricity, 1);
                            cur->density--;
                        }
                        while (valid.size() > 0 && cur->density > 0) {
                            int index = rng(0, valid.size() - 1);
                            add_field(g, valid[index].x, valid[index].y, fd_electricity, 1);
                            cur->density--;
                            valid.erase(valid.begin() + index);
                        }
                    }
                }
                break;
            }

            if (cur->type != fd_null) {
                cur->age++;
                if (cur->age > 0 && dice(3, cur->age) > dice(3, fieldlist[cur->type].halflife)) {
                    cur->age = 0;
                    cur->density--;
                }
                if (cur->density <= 0)
                    field_at(x, y) = field();
            }
        }
    }
    return found_field;
}

void Map::step_in_field(int x, int y, Game* g)
{
    field* cur = &field_at(x, y);

    switch (cur->type) {
    case fd_null:
    case fd_blood: // It doesn't actually do anything
    case fd_bile:  // Ditto
    default:
        return;

    case fd_acid:
        if (cur->density == 3) {
            g->add_msg("The acid burns your legs and feet!");
            g->u.hit(g, bp_feet, 0, 0, rng(4, 10));
            g->u.hit(g, bp_feet, 1, 0, rng(4, 10));
            g->u.hit(g, bp_legs, 0, 0, rng(2, 8));
            g->u.hit(g, bp_legs, 1, 0, rng(2, 8));
        } else {
            g->add_msg("The acid burns your feet!");
            g->u.hit(g, bp_feet, 0, 0, rng(cur->density, 4 * cur->density));
            g->u.hit(g, bp_feet, 1, 0, rng(cur->density, 4 * cur->density));
        }
        break;

    case fd_fire:
        if (!g->u.has_active_bionic(bio_heatsink)) {
            if (cur->density == 1) {
                g->add_msg("You burn your legs and feet!");
                g->u.hit(g, bp_feet, 0, 0, rng(2, 6));
                g->u.hit(g, bp_feet, 1, 0, rng(2, 6));
                g->u.hit(g, bp_legs, 0, 0, rng(1, 4));
                g->u.hit(g, bp_legs, 1, 0, rng(1, 4));
            } else if (cur->density == 2) {
                g->add_msg("You're burning up!");
                g->u.hit(g, bp_legs, 0, 0, rng(2, 6));
                g->u.hit(g, bp_legs, 1, 0, rng(2, 6));
                g->u.hit(g, bp_torso, 0, 4, rng(4, 9));
                g->u.infect(DI_SMOKE, bp_mouth, 5, 20, g);
            } else if (cur->density == 3) {
                g->add_msg("You're set ablaze!");
                g->u.hit(g, bp_legs, 0, 0, rng(2, 6));
                g->u.hit(g, bp_legs, 1, 0, rng(2, 6));
                g->u.hit(g, bp_torso, 0, 4, rng(4, 9));
                g->u.add_disease(DI_ONFIRE, 5, g);
                g->u.infect(DI_SMOKE, bp_mouth, 7, 30, g);
            }
        }
        break;
    case fd_smoke:
        if (cur->density == 3)
            g->u.infect(DI_SMOKE, bp_mouth, 4, 15, g);
        break;
    case fd_tear_gas:
        if (cur->density > 1 || !one_in(3))
            g->u.infect(DI_TEARGAS, bp_mouth, 5, 20, g);
        break;
    case fd_nuke_gas:
        g->u.radiation += rng(0, cur->density * (cur->density + 1));
        if (cur->density == 3) {
            g->add_msg("This radioactive gas burns!");
            g->u.hurtall(rng(1, 3));
        }
        break;
    case fd_electricity:
        g->add_msg("You're electrocuted!");
        g->u.hurtall(rng(1, cur->density));
        if (one_in(8 - cur->density) && !one_in(30 - g->u.str_cur)) {
            g->add_msg("You're paralyzed!");
            g->u.moves -= cur->density * 150;
        }
        break;
    }
}

void Map::mon_in_field(int x, int y, Monster* z)
{
    if (z->has_flag(MF_DIGS))
        return; // Digging monsters are immune to fields
    field* cur = &field_at(x, y);
    int dam {0};

    switch (cur->type) {
    case fd_null:
    case fd_blood: // It doesn't actually do anything
    case fd_bile:  // Ditto
    default:
        break;

    case fd_acid:
        if (!z->has_flag(MF_DIGS) && !z->has_flag(MF_ACIDPROOF)) {
            if (cur->density == 3)
                dam = rng(4, 10) + rng(2, 8);
            else
                dam = rng(cur->density, cur->density * 4);
        }
        break;

    case fd_fire:
        if (z->made_of(FLESH))
            dam = 3;
        if (z->made_of(VEGGY))
            dam = 12;
        if (z->made_of(PAPER) || z->made_of(LIQUID) || z->made_of(POWDER) || z->made_of(WOOD)
            || z->made_of(COTTON) || z->made_of(WOOL))
            dam = 50;
        if (z->made_of(STONE) || z->made_of(KEVLAR) || z->made_of(STEEL))
            dam = -25;
        if (z->has_flag(MF_FLIES))
            dam -= 20;

        if (cur->density == 1)
            dam += rng(4, 12);
        else if (cur->density == 2) {
            dam += rng(6, 18);
            z->moves -= 50;
        } else if (cur->density == 3) {
            dam += rng(10, 30);
            z->moves -= 80;
        }
        break;

    case fd_smoke:
        if (cur->density == 3)
            z->speed -= rng(10, 20);
        if (z->made_of(VEGGY)) // Plants suffer from smoke even worse
            z->speed -= rng(1, cur->density * 12);
        break;

    case fd_tear_gas:
        if (cur->density == 3) {
            z->speed -= rng(30, 60);
            dam = rng(8, 20);
        } else if (cur->density == 2) {
            z->speed -= rng(10, 25);
            dam = rng(4, 10);
        } else
            z->speed -= rng(0, 6);
        if (z->made_of(VEGGY)) {
            z->speed -= rng(cur->density * 5, cur->density * 12);
            dam += cur->density * 10;
        }
        break;
    case fd_nuke_gas:
        if (cur->density == 3) {
            z->speed -= rng(60, 120);
            dam = rng(30, 50);
        } else if (cur->density == 2) {
            z->speed -= rng(20, 50);
            dam = rng(10, 25);
        } else {
            z->speed -= rng(0, 15);
            dam = rng(0, 12);
        }
        if (z->made_of(VEGGY)) {
            z->speed -= rng(cur->density * 5, cur->density * 12);
            dam *= cur->density;
        }
        break;
    case fd_electricity:
        dam = rng(1, cur->density);
        if (one_in(8 - cur->density) && one_in(z->armor()))
            z->moves -= cur->density * 150;
        break;
    }

    z->hurt(dam);
}
} // namespace oocdda
