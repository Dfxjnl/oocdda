#ifndef OOCDDA_TRAP_HPP
#define OOCDDA_TRAP_HPP

#include <string>

#include "color.hpp"
#include "itype.hpp"
#include "monster.hpp"

namespace oocdda {
enum trap_id {
    tr_null,
    tr_bubblewrap,
    tr_beartrap,
    tr_beartrap_buried,
    tr_snare,
    tr_nailboard,
    tr_tripwire,
    tr_crossbow,
    tr_shotgun_2,
    tr_shotgun_1,
    tr_engine,
    tr_blade,
    tr_landmine,
    tr_telepad,
    tr_goo,
    tr_dissector,
    tr_sinkhole,
    tr_pit,
    tr_spike_pit,
    tr_portal,
    num_trap_types
};

struct trap;

struct trapfunc {
    void none(Game* g, int x, int y) {};
    void bubble(Game* g, int x, int y);
    void beartrap(Game* g, int x, int y);
    void snare(Game* g, int x, int y) {};
    void board(Game* g, int x, int y);
    void tripwire(Game* g, int x, int y);
    void crossbow(Game* g, int x, int y);
    void shotgun(Game* g, int x, int y);
    void blade(Game* g, int x, int y);
    void landmine(Game* g, int x, int y);
    void telepad(Game* g, int x, int y);
    void goo(Game* g, int x, int y);
    void dissector(Game* g, int x, int y);
    void sinkhole(Game* g, int x, int y);
    void pit(Game* g, int x, int y);
    void pit_spikes(Game* g, int x, int y);
    void portal(Game* g, int x, int y) {};
};

struct trapfuncm {
    void none(Game* g, monster* z, int x, int y) {};
    void bubble(Game* g, monster* z, int x, int y);
    void beartrap(Game* g, monster* z, int x, int y);
    void board(Game* g, monster* z, int x, int y);
    void tripwire(Game* g, monster* z, int x, int y);
    void crossbow(Game* g, monster* z, int x, int y);
    void shotgun(Game* g, monster* z, int x, int y);
    void blade(Game* g, monster* z, int x, int y);
    void snare(Game* g, monster* z, int x, int y) {};
    void landmine(Game* g, monster* z, int x, int y);
    void telepad(Game* g, monster* z, int x, int y);
    void goo(Game* g, monster* z, int x, int y);
    void dissector(Game* g, monster* z, int x, int y);
    void sinkhole(Game* g, monster* z, int x, int y) {};
    void pit(Game* g, monster* z, int x, int y);
    void pit_spikes(Game* g, monster* z, int x, int y);
    void portal(Game* g, monster* z, int x, int y) {};
};

struct trap {
    int id;
    char sym;
    nc_color color;
    std::string name;

    int visibility; // 1 to ??, affects detection
    int avoidance; // 0 to ??, affects avoidance
    int difficulty; // 0 to ??, difficulty of assembly & disassembly
    std::vector<itype_id> components; // For disassembly?

    // You stepped on it
    void (trapfunc::*act)(Game*, int x, int y);
    // Monster stepped on it
    void (trapfuncm::*actm)(Game*, monster*, int x, int y);

    trap(int pid, char psym, nc_color pcolor, std::string pname, int pvisibility, int pavoidance,
        int pdifficulty, void (trapfunc::*pact)(Game*, int x, int y),
        void (trapfuncm::*pactm)(Game*, monster*, int x, int y), ...)
    {
        id = pid;
        sym = psym;
        color = pcolor;
        name = pname;
        visibility = pvisibility;
        avoidance = pavoidance;
        difficulty = pdifficulty;
        act = pact;
        actm = pactm;
        va_list ap;
        va_start(ap, pactm);
        itype_id tmp;
        while (tmp = (itype_id)va_arg(ap, int))
            components.push_back(tmp);
        va_end(ap);
    }
};
} // namespace oocdda

#endif // OOCDDA_TRAP_HPP
