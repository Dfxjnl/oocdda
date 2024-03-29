#ifndef OOCDDA_MAPDATA_HPP
#define OOCDDA_MAPDATA_HPP

#include <array>
#include <string>
#include <vector>

#include "color.hpp"
#include "enums.hpp"
#include "item.hpp"
#include "trap.hpp"

namespace oocdda {
class Game;
class Monster;

#ifndef SEEX // SEEX is how far the player can see in the X direction (at
// least, without scrolling).  All map segments will need to be at least
// this wide.  The map therefore needs to be 3 times as wide.
#    define SEEX 12
#endif
#ifndef SEEY // Same as SEEX
#    define SEEY                                                                                   \
        12 // Requires 2*SEEY+1= 25 vertical squares
           // Nuts to 80x24 terms. Mostly exists in graphical clients, and
           // those fatcats can resize.
#endif

enum t_flag {
    transparent, // Player & monsters can see through/past it
    bashable,    // Player & monsters can bash this & make it the next in the list
    container,   // Items on this square are hidden until looted by the player
    flammable,   // May be lit on fire
    explodes,    // Explodes when on fire
    diggable,    // Digging monsters, seeding monsters, digging w/ shovel, etc.
    swimmable,   // You (and monsters) swim here
    sharp,       // May do minor damage to players/monsters passing it
    rough,       // May hurt the player's feet
    sealed,      // Can't 'e' to retrieve items here
    noitem,      // Items "fall off" this space
    goes_down,   // Can '>' to go down a level
    goes_up,     // Can '<' to go up a level
    computer,    // Used as a computer
    num_t_flags  // MUST be last
};

struct ter_t {
    std::string name;
    char sym;
    nc_color color;
    unsigned char movecost;
    unsigned flags : num_t_flags;
};

enum ter_id {
    t_null = 0,
    // Ground
    t_dirt,
    t_dirtmound,
    t_pit,
    t_rock_floor,
    t_rubble,
    t_grass,
    t_pavement,
    t_pavement_y,
    t_sidewalk,
    t_floor,
    t_grate,
    t_slime,
    // Walls & doors
    t_wall_v,
    t_wall_h,
    t_wall_glass_v,
    t_wall_glass_h,
    t_reinforced_glass_v,
    t_reinforced_glass_h,
    t_bars,
    t_door_c,
    t_door_b,
    t_door_o,
    t_door_locked,
    t_door_frame,
    t_door_boarded,
    t_door_metal_c,
    t_door_metal_o,
    t_door_metal_locked,
    t_bulletin,
    t_portcullis,
    t_window,
    t_window_frame,
    t_window_boarded,
    t_rock,
    // Tree
    t_tree,
    t_tree_young,
    t_underbrush,
    t_wax,
    t_floor_wax,
    t_fence_v,
    t_fence_h,
    t_railing_v,
    t_railing_h,
    // Water, lava, etc.
    t_water_sh,
    t_water_dp,
    t_sewage,
    t_lava,
    // Embellishments
    t_bed,
    t_toilet,
    t_gas_pump,
    t_gas_pump_smashed,
    t_counter,
    t_radio_tower,
    t_radio_controls,
    t_computer_broken,
    t_computer_nether,
    t_computer_lab,
    // Containers
    t_fridge,
    t_dresser,
    t_rack,
    t_bookcase,
    t_dumpster,
    t_vat,
    // Staircases etc.
    t_stairs_down,
    t_stairs_up,
    t_manhole,
    t_ladder,
    t_slope_down,
    t_slope_up,
    // Special
    t_card_reader,
    t_card_reader_broken,
    t_manhole_cover,
    t_slot_machine,
    num_terrain_types
};

const ter_t terlist[num_terrain_types] = {
  // MUST match enum ter_id above!
    {"nothing",              ' ',  c_white,   2,  flag_to_bit_position(transparent)                                  },
    {"dirt",                 '.',  c_brown,   2,  flag_to_bit_position(transparent) | flag_to_bit_position(diggable) },
    {"mound of dirt",        '#',  c_brown,   3,
     flag_to_bit_position(transparent) | flag_to_bit_position(diggable)                                              },
    {"shallow pit",          '0',  c_brown,   14,
     flag_to_bit_position(transparent) | flag_to_bit_position(diggable)                                              },
    {"rock floor",           '.',  c_ltgray,  2,  flag_to_bit_position(transparent)                                  },
    {"pile of rubble",       '#',  c_ltgray,  4,
     flag_to_bit_position(transparent) | flag_to_bit_position(rough)
         | flag_to_bit_position(diggable)                                                                            },
    {"grass",                '.',  c_green,   2,  flag_to_bit_position(transparent) | flag_to_bit_position(diggable) },
    {"pavement",             '.',  c_dkgray,  2,  flag_to_bit_position(transparent)                                  },
    {"yellow pavement",      '.',  c_yellow,  2,  flag_to_bit_position(transparent)                                  },
    {"sidewalk",             '.',  c_ltgray,  2,  flag_to_bit_position(transparent)                                  },
    {"floor",                '.',  c_cyan,    2,  flag_to_bit_position(transparent)                                  },
    {"metal grate",          '#',  c_dkgray,  2,  flag_to_bit_position(transparent)                                  },
    {"slime",                '~',  c_green,   6,
     flag_to_bit_position(transparent) | flag_to_bit_position(container)
         | flag_to_bit_position(flammable)                                                                           },
    {"wall",                 '|',  c_ltgray,  0,  flag_to_bit_position(flammable)                                    },
    {"wall",                 '-',  c_ltgray,  0,  flag_to_bit_position(flammable)                                    },
    {"glass wall",           '|',  c_ltcyan,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)                                              },
    {"glass wall",           '-',  c_ltcyan,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)                                              },
    {"reinforced glass",     '|',  c_ltcyan,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)                                              },
    {"reinforced glass",     '-',  c_ltcyan,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)                                              },
    {"metal bars",           '"',  c_ltgray,  0,  flag_to_bit_position(transparent)                                  },
    {"closed wood door",     '+',  c_brown,   0,
     flag_to_bit_position(bashable) | flag_to_bit_position(flammable)                                                },
    {"damaged wood door",    '&',  c_brown,   0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)
         | flag_to_bit_position(flammable)                                                                           },
    {"open wood door",       '\'', c_brown,   2,  flag_to_bit_position(transparent)                                  },
    {"closed wood door",     '+',  c_brown,   0,  // Actually locked
     flag_to_bit_position(bashable) | flag_to_bit_position(flammable)},
    {"empty door frame",     '.',  c_brown,   2,  flag_to_bit_position(transparent)                                  },
    {"boarded up door",      '#',  c_brown,   0,
     flag_to_bit_position(bashable) | flag_to_bit_position(flammable)                                                },
    {"closed metal door",    '+',  c_cyan,    0,  0                                                                  },
    {"open metal door",      '\'', c_cyan,    2,  flag_to_bit_position(transparent)                                  },
    {"closed metal door",    '+',  c_cyan,    0,  // Actually locked
     0                                                               },
    {"bulletin board",       '6',  c_blue,    0,  0                                                                  },
    {"makeshift portcullis", '&',  c_cyan,    0,  0                                                                  },
    {"window",               '"',  c_ltcyan,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)
         | flag_to_bit_position(flammable)                                                                           },
    {"window frame",         '0',  c_ltcyan,  12,
     flag_to_bit_position(container) | flag_to_bit_position(transparent)
         | flag_to_bit_position(sharp) | flag_to_bit_position(flammable)
         | flag_to_bit_position(noitem)                                                                              },
    {"boarded up window",    '#',  c_brown,   0,
     flag_to_bit_position(bashable) | flag_to_bit_position(flammable)                                                },
    {"solid rock",           '#',  c_white,   0,  0                                                                  },
    {"tree",                 '7',  c_green,   0,  flag_to_bit_position(flammable)                                    },
    {"young tree",           '1',  c_green,   0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)
         | flag_to_bit_position(flammable)                                                                           },
    {"underbrush",           '#',  c_green,   7,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)
         | flag_to_bit_position(diggable) | flag_to_bit_position(container)
         | flag_to_bit_position(rough) | flag_to_bit_position(flammable)                                             },
    {"wax wall",             '#',  c_yellow,  0,
     flag_to_bit_position(container) | flag_to_bit_position(flammable)                                               },
    {"wax floor",            '.',  c_yellow,  2,  flag_to_bit_position(transparent)                                  },
    {"picket fence",         '|',  c_brown,   3,
     flag_to_bit_position(transparent) | flag_to_bit_position(diggable)
         | flag_to_bit_position(flammable) | flag_to_bit_position(noitem)                                            },
    {"picket fence",         '-',  c_brown,   3,
     flag_to_bit_position(transparent) | flag_to_bit_position(diggable)
         | flag_to_bit_position(flammable) | flag_to_bit_position(noitem)                                            },
    {"railing",              '|',  c_yellow,  3,  flag_to_bit_position(transparent) | flag_to_bit_position(noitem)   },
    {"railing",              '-',  c_yellow,  3,  flag_to_bit_position(transparent) | flag_to_bit_position(noitem)   },
    {"shallow water",        '~',  c_ltblue,  5,
     flag_to_bit_position(transparent) | flag_to_bit_position(swimmable)                                             },
    {"deep water",           '~',  c_blue,    0,
     flag_to_bit_position(transparent) | flag_to_bit_position(swimmable)                                             },
    {"sewage",               '~',  c_ltgreen, 6,
     flag_to_bit_position(transparent) | flag_to_bit_position(swimmable)                                             },
    {"lava",                 '~',  c_red,     0,  flag_to_bit_position(transparent)                                  },
    {"bed",                  '#',  c_magenta, 5,
     flag_to_bit_position(transparent) | flag_to_bit_position(container)
         | flag_to_bit_position(flammable)                                                                           },
    {"toilet",               '&',  c_white,   0,  flag_to_bit_position(transparent) | flag_to_bit_position(bashable) },
    {"gasoline pump",        '&',  c_red,     0,
     flag_to_bit_position(transparent) | flag_to_bit_position(explodes)                                              },
    {"smashed gas pump",     '&',  c_ltred,   0,  flag_to_bit_position(transparent)                                  },
    {"counter",              '#',  c_blue,    4,  flag_to_bit_position(transparent)                                  },
    {"radio tower",          '&',  c_ltgray,  0,  0                                                                  },
    {"radio controls",       '6',  c_green,   0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)                                              },
    {"broken computer",      '6',  c_ltgray,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(computer)                                              },
    {"computer console",     '6',  c_blue,    0,
     flag_to_bit_position(transparent) | flag_to_bit_position(computer)                                              },
    {"computer console",     '6',  c_blue,    0,
     flag_to_bit_position(transparent) | flag_to_bit_position(computer)                                              },
    {"refrigerator",         '{',  c_ltcyan,  0,  flag_to_bit_position(container)                                    },
    {"dresser",              '{',  c_brown,   0,
     flag_to_bit_position(transparent) | flag_to_bit_position(container)
         | flag_to_bit_position(flammable)                                                                           },
    {"display rack",         '{',  c_ltgray,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(container)                                             },
    {"book case",            '{',  c_brown,   0,
     flag_to_bit_position(container) | flag_to_bit_position(flammable)                                               },
    {"dumpster",             '{',  c_green,   0,  flag_to_bit_position(container)                                    },
    {"cloning vat",          '0',  c_ltcyan,  0,
     flag_to_bit_position(transparent) | flag_to_bit_position(bashable)
         | flag_to_bit_position(container) | flag_to_bit_position(sealed)                                            },
    {"stairs down",          '>',  c_yellow,  2,
     flag_to_bit_position(transparent) | flag_to_bit_position(goes_down)                                             },
    {"stairs up",            '<',  c_yellow,  2,
     flag_to_bit_position(transparent) | flag_to_bit_position(goes_up)                                               },
    {"manhole",              '>',  c_dkgray,  2,
     flag_to_bit_position(transparent) | flag_to_bit_position(goes_down)                                             },
    {"ladder",               '<',  c_dkgray,  2,  flag_to_bit_position(transparent) | flag_to_bit_position(goes_up)  },
    {"downward slope",       '>',  c_brown,   2,
     flag_to_bit_position(transparent) | flag_to_bit_position(goes_down)                                             },
    {"upward slope",         '<',  c_brown,   2,
     flag_to_bit_position(transparent) | flag_to_bit_position(goes_up)                                               },
    {"card reader",          '6',  c_pink,    0,  0                                                                  },
    {"broken card reader",   '6',  c_ltgray,  0,  0                                                                  },
    {"manhole cover",        '0',  c_dkgray,  2,  flag_to_bit_position(transparent)                                  },
    {"slot machine",         '6',  c_green,   0,  flag_to_bit_position(bashable)                                     }
};

struct field_t {
    std::string name[3];
    char sym;
    nc_color color[3];
    bool transparent[3];
    bool dangerous[3];
    int halflife; // In turns
};

enum field_id {
    fd_null = 0,
    fd_blood,
    fd_bile,
    fd_slime,
    fd_acid,
    fd_fire,
    fd_smoke,
    fd_tear_gas,
    fd_nuke_gas,
    fd_electricity,
    num_fields
};

const std::array<field_t, field_id::num_fields> fieldlist {
    {{{
          "",
          "",
          "",
      },
      '%',
      {c_white, c_white, c_white},
      {true, true, true},
      {false, false, false},
      0},
     {{"blood splatter", "blood stain", "puddle of blood"},
      '%',
      {c_red, c_red, c_red},
      {true, true, true},
      {false, false, false},
      2500},
     {{"bile splatter", "bile stain", "puddle of bile"},
      '%',
      {c_pink, c_pink, c_pink},
      {true, true, true},
      {false, false, false},
      2500},
     {{"slime trail", "slime stain", "puddle of slime"},
      '%',
      {c_ltgreen, c_ltgreen, c_green},
      {true, true, true},
      {false, false, false},
      2500},
     {{"acid splatter", "acid streak", "pool of acid"},
      '5',
      {c_ltgreen, c_green, c_green},
      {true, true, true},
      {true, true, true},
      10},
     {{"small fire", "fire", "raging fire"},
      '4',
      {c_yellow, c_ltred, c_red},
      {true, true, true},
      {true, true, true},
      2000},
     {{"thin smoke", "smoke", "thick smoke"},
      '8',
      {c_white, c_ltgray, c_dkgray},
      {true, false, false},
      {false, true, true},
      400},
     {{"hazy cloud", "tear gas", "thick tear gas"},
      '8',
      {c_white, c_yellow, c_brown},
      {true, false, false},
      {true, true, true},
      600},
     {{"hazy cloud", "radioactive gas", "thick radioactive gas"},
      '8',
      {c_white, c_ltgreen, c_green},
      {true, true, false},
      {true, true, true},
      1000},
     {{"sparks", "electric crackle", "electric cloud"},
      '9',
      {c_white, c_cyan, c_blue},
      {true, true, true},
      {true, true, true},
      2}}
};

struct field {
    field_id type;
    char density {1};
    int age;

    field()
    {
        type = fd_null;
        age = 0;
    }

    field(field_id t, const char p_density, unsigned int a)
        : density {p_density}
    {
        type = t;
        age = a;
    }

    bool is_null()
    {
        if (type == fd_null || type == fd_blood || type == fd_bile || type == fd_slime)
            return true;
        return false;
    }
    bool is_dangerous() { return fieldlist[type].dangerous[density - 1]; }
    std::string name() { return fieldlist[type].name[density - 1]; }
};

struct spawn_point {
    int posx, posy;
    int count;
    mon_id type;
    spawn_point(mon_id T = mon_null, int C = 0, int X = -1, int Y = -1)
        : posx(X)
        , posy(Y)
        , count(C)
        , type(T)
    {
    }
};

struct submap {
    ter_id ter[SEEX][SEEY];            // Terrain on each square
    std::vector<Item> itm[SEEX][SEEY]; // Items on each square
    trap_id trp[SEEX][SEEY];           // Trap on each square
    field fld[SEEX][SEEY];             // Field on each square
    int rad[SEEX][SEEY];               // Irradiation of each square
    std::vector<spawn_point> spawns;
};
} // namespace oocdda

#endif // OOCDDA_MAPDATA_HPP
