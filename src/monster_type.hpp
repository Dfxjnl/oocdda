#ifndef OOCDDA_MONSTER_TYPE_HPP
#define OOCDDA_MONSTER_TYPE_HPP

// SEE ALSO: monitemsdef.cpp, which defines data on which items any given monster may carry.

#include <functional>
#include <string>
#include <string_view>

#include "color.hpp"
#include "enums.hpp"
#include "mondeath.hpp"
#include "monster_attack.hpp"

namespace oocdda {
class mdeath;

enum mon_id {
    mon_null = 0,
    mon_squirrel,
    mon_rabbit,
    mon_deer,
    mon_wolf,
    mon_bear,
    mon_dog,
    mon_ant_larva,
    mon_ant,
    mon_ant_soldier,
    mon_ant_queen,
    mon_ant_fungus,
    mon_bee,
    mon_graboid,
    mon_worm,
    mon_halfworm,
    mon_zombie,
    mon_zombie_shrieker,
    mon_zombie_spitter,
    mon_zombie_electric,
    mon_zombie_fast,
    mon_zombie_brute,
    mon_zombie_hulk,
    mon_zombie_fungus,
    mon_boomer,
    mon_boomer_fungus,
    mon_skeleton,
    mon_zombie_necro,
    mon_triffid,
    mon_triffid_young,
    mon_triffid_queen,
    mon_fungaloid,
    mon_fungaloid_dormant,
    mon_spore,
    mon_blob,
    mon_blob_small,
    mon_chud,
    mon_one_eye,
    mon_crawler,
    mon_sewer_fish,
    mon_sewer_snake,
    mon_sewer_rat,
    mon_flying_polyp,
    mon_hunting_horror,
    mon_mi_go,
    mon_yugg,
    mon_gelatin,
    mon_flaming_eye,
    mon_kreck,
    mon_blank,
    mon_eyebot,
    mon_manhack,
    mon_skitterbot,
    mon_secubot,
    mon_molebot,
    mon_tripod,
    mon_chickenbot,
    mon_tankbot,
    mon_turret,
    mon_hallu_zom,
    mon_hallu_bee,
    mon_hallu_ant,
    mon_hallu_mom,
    num_monsters
};

enum m_size {
    MS_TINY = 0, // Rodent
    MS_SMALL,    // Half human
    MS_MEDIUM,   // Human
    MS_LARGE,    // Cow
    MS_HUGE      // TAAAANK
};

// Feel free to add to m_flags.  Order shouldn't matter, just keep it tidy!
// There is a maximum number of 32 flags, including MF_MAX, so...
// And comment them well. ;)
enum m_flags {
    MF_SEES,        // It can see you (and will run/follow)
    MF_HEARS,       // It can hear you
    MF_GOODHEARING, // Pursues sounds more than most monsters
    MF_SMELLS,      // It can smell you
    MF_STUMBLES,    // Stumbles in its movement
    MF_WARM,        // Warm blooded
    MF_NOHEAD,      // Headshots not allowed!
    MF_HARDTOSHOOT, // Some shots are actually misses
    MF_BASHES,      // Bashes down doors
    MF_DESTROYS,    // Bashes down walls and more
    MF_POISON,      // Poisonous to eat
    MF_VENOM,       // Attack may poison the player
    MF_DIGS,        // Digs through the ground
    MF_FLIES,       // Can fly (over water, etc)
    MF_AQUATIC,     // Confined to water
    MF_SWIMS,       // Treats water as 50 movement point terrain
    MF_ATTACKMON,   // Attacks other monsters
    MF_ANIMAL,      // Is an "animal" for purposes of the Animal Empath trait
    MF_PLASTIC,     // Absorbs physical damage to a great degree
    MF_SUNDEATH,    // Dies in full sunlight
    MF_ACIDPROOF,   // Immune to acid
    MF_FIREY,       // Burns stuff and is immune to fire
    MF_SHOCK,       // Shocks the player if they attack w/out gloves
    MF_ELECTRONIC,  // E.g. a robot; affected by emp blasts, and other stuff
    MF_FUR,         // May produce fur when butchered.
    MF_LEATHER,     // May produce leather when butchered
    MF_IMMOBILE,    // Doesn't move (e.g. turrets)
    MF_MAX          // Sets the length of the flags - obviously MUST be last
};

struct MonsterType {
    MonsterType() = default;

    // Non-default (messy).
    MonsterType(const mon_id pid,
                const std::string_view pname,
                const char psym,
                const nc_color pcolor,
                const m_size psize,
                const material pmat,
                const unsigned int pflags,
                const int pfreq,
                const int pdiff,
                const int pagro,
                const int pspeed,
                const int pml_skill,
                const int pml_dice,
                const int pml_sides,
                const int pml_cut,
                const int pdodge,
                const int parmor,
                const int pitem_chance,
                const int php,
                const int psp_freq,
                void (mdeath::*pdies)(Game*, Monster*),
                const std::function<void(Game&, Monster&)>& p_special_attack,
                const std::string_view pdescription)
        : id {pid}
        , name {pname}
        , description {pdescription}
        , sym {psym}
        , color {pcolor}
        , size {psize}
        , mat {pmat}
        , flags {pflags}
        , frequency {pfreq}
        , difficulty {pdiff}
        , agro {pagro}
        , speed {pspeed}
        , melee_skill {pml_skill}
        , melee_dice {pml_dice}
        , melee_sides {pml_sides}
        , melee_cut {pml_cut}
        , sk_dodge {pdodge}
        , armor {parmor}
        , item_chance {pitem_chance}
        , hp {php}
        , sp_freq {psp_freq}
        , dies {pdies}
        , special_attack {p_special_attack}
    {
    }

    mon_id id {mon_id::mon_null};
    std::string name {"human"};
    std::string description;
    char sym {' '};                     // Symbol on the map.
    nc_color color {nc_color::c_white}; // Color of symbol.h).

    m_size size {m_size::MS_MEDIUM};
    material mat {material::FLESH}; // Generally, flesh; veggy?
    unsigned int flags {0};         // Bitfield of m_flags.

    int frequency {0};  // How often do these show up? 0 (never) to ??.
    int difficulty {0}; // Used all over; 30 min + (diff-3)*30 min = earlist appearance.
    int agro {0};       // How likely to attack; -5 to 5.

    int speed {0};       // Speed; human = 100.
    int melee_skill {0}; // Melee skill; should be 0 to 5.
    int melee_dice {0};  // Number of dice on melee hit.
    int melee_sides {0}; // Number of sides those dice have.
    int melee_cut {0};   // Bonus cutting damage.
    int sk_dodge {0};    // Dodge skill; should be 0 to 5.
    int armor {0};       // Natural armor.
    // Higher number means higher chance of loot. Negative number means maximum of one item
    // generated.
    int item_chance {0};
    int hp {0};

    int sp_freq {0}; // How long sp_attack takes to charge.

    void (mdeath::*dies)(Game*, Monster*) {nullptr}; // What happens when this monster dies.
    std::function<void(Game&, Monster&)> special_attack {nullptr}; // This monster's special attack.
};
} // namespace oocdda

#endif // OOCDDA_MONSTER_TYPE_HPP
