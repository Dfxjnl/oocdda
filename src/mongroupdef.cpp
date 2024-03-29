#include <cstddef>

#include "game.hpp"
#include "mongroup.hpp"
#include "monster_type.hpp"
#include "setvector.hpp"

namespace oocdda {
void Game::init_moncats()
{
    setvector(moncats[mcat_forest], mon_squirrel, mon_rabbit, mon_deer, mon_wolf, mon_bear, NULL);
    setvector(moncats[mcat_ant], mon_ant_larva, mon_ant, mon_ant_soldier, NULL);
    setvector(moncats[mcat_bee], mon_bee, NULL);
    setvector(moncats[mcat_worm], mon_graboid, mon_worm, mon_halfworm, NULL);
    setvector(moncats[mcat_zombie], mon_zombie, mon_zombie_shrieker, mon_zombie_spitter,
              mon_zombie_fast, mon_zombie_electric, mon_zombie_brute, mon_zombie_hulk,
              mon_zombie_necro, mon_boomer, mon_skeleton, NULL);
    setvector(moncats[mcat_plants], mon_triffid, mon_triffid_young, mon_triffid_queen, NULL);
    setvector(moncats[mcat_fungi], mon_fungaloid, mon_fungaloid_dormant, mon_ant_fungus,
              mon_zombie_fungus, mon_boomer_fungus, mon_spore, NULL);
    setvector(moncats[mcat_goo], mon_blob, NULL);
    setvector(moncats[mcat_chud], mon_chud, mon_one_eye, mon_crawler, NULL);
    setvector(moncats[mcat_sewer], mon_sewer_fish, mon_sewer_snake, mon_sewer_rat, NULL);
    setvector(moncats[mcat_nether], mon_flying_polyp, mon_hunting_horror, mon_mi_go, mon_yugg,
              mon_gelatin, mon_flaming_eye, mon_kreck, mon_blank, NULL);
}
} // namespace oocdda
