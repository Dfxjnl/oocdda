#include <string>

#include <gtest/gtest.h>

#include "color.hpp"
#include "enums.hpp"
#include "mondeath.hpp"
#include "monster_type.hpp"

using oocdda::m_flags;
using oocdda::m_size;
using oocdda::material;
using oocdda::mdeath;
using oocdda::mon_id;
using oocdda::MonsterType;
using oocdda::nc_color;

TEST(MonsterTypeTest, DefaultConstructor)
{
    const MonsterType default_type;

    EXPECT_EQ(default_type.id, mon_id::mon_null);
    EXPECT_EQ(default_type.name, "human");
    EXPECT_TRUE(default_type.description.empty());
    EXPECT_EQ(default_type.sym, ' ');
    EXPECT_EQ(default_type.color, nc_color::c_white);
    EXPECT_EQ(default_type.size, m_size::MS_MEDIUM);
    EXPECT_EQ(default_type.mat, material::FLESH);
    EXPECT_EQ(default_type.flags, 0);
    EXPECT_EQ(default_type.frequency, 0);
    EXPECT_EQ(default_type.difficulty, 0);
    EXPECT_EQ(default_type.agro, 0);
    EXPECT_EQ(default_type.speed, 0);
    EXPECT_EQ(default_type.melee_skill, 0);
    EXPECT_EQ(default_type.melee_dice, 0);
    EXPECT_EQ(default_type.melee_sides, 0);
    EXPECT_EQ(default_type.melee_cut, 0);
    EXPECT_EQ(default_type.sk_dodge, 0);
    EXPECT_EQ(default_type.armor, 0);
    EXPECT_EQ(default_type.item_chance, 0);
    EXPECT_EQ(default_type.hp, 0);
    EXPECT_EQ(default_type.sp_freq, 0);
    EXPECT_EQ(default_type.dies, nullptr);
    EXPECT_EQ(default_type.special_attack, nullptr);
}

TEST(MonsterTypeTest, CustomConstructor)
{
    constexpr auto id {mon_id::mon_zombie};
    const std::string name {"zombie"};
    constexpr char symbol {'Z'};
    constexpr auto color {nc_color::c_ltgreen};
    constexpr auto size {m_size::MS_MEDIUM};
    constexpr auto material {material::FLESH};
    constexpr auto flags {
        flag_to_bit_position(m_flags::MF_SEES) | flag_to_bit_position(m_flags::MF_HEARS)
        | flag_to_bit_position(m_flags::MF_SMELLS) | flag_to_bit_position(m_flags::MF_STUMBLES)
        | flag_to_bit_position(m_flags::MF_WARM) | flag_to_bit_position(m_flags::MF_BASHES)
        | flag_to_bit_position(m_flags::MF_POISON)};
    constexpr int frequency {70};
    constexpr int difficulty {3};
    constexpr int aggressiveness {5};
    constexpr int speed {70};
    constexpr int melee_skill {8};
    constexpr int melee_dice {1};
    constexpr int melee_sides {5};
    constexpr int melee_cut_bonus {0};
    constexpr int dodge {1};
    constexpr int armor {0};
    constexpr int item_chance {40};
    constexpr int hit_points {50};
    constexpr int special_frequency {0};
    const auto death_function {&mdeath::normal};
    const auto special_attack_function {nullptr};
    const std::string description {"A human body, stumbling slowly forward on\n"
                                   "uncertain legs, possessed with an\n"
                                   "unstoppable rage."};

    const MonsterType zombie {id,
                              name,
                              symbol,
                              color,
                              size,
                              material,
                              flags,
                              frequency,
                              difficulty,
                              aggressiveness,
                              speed,
                              melee_skill,
                              melee_dice,
                              melee_sides,
                              melee_cut_bonus,
                              dodge,
                              armor,
                              item_chance,
                              hit_points,
                              special_frequency,
                              death_function,
                              special_attack_function,
                              description};

    EXPECT_EQ(zombie.id, id);
    EXPECT_EQ(zombie.name, name);
    EXPECT_EQ(zombie.description, description);
    EXPECT_EQ(zombie.sym, symbol);
    EXPECT_EQ(zombie.color, color);
    EXPECT_EQ(zombie.size, size);
    EXPECT_EQ(zombie.mat, material);
    EXPECT_EQ(zombie.flags, flags);
    EXPECT_EQ(zombie.frequency, frequency);
    EXPECT_EQ(zombie.difficulty, difficulty);
    EXPECT_EQ(zombie.agro, aggressiveness);
    EXPECT_EQ(zombie.speed, speed);
    EXPECT_EQ(zombie.melee_skill, melee_skill);
    EXPECT_EQ(zombie.melee_dice, melee_dice);
    EXPECT_EQ(zombie.melee_sides, melee_sides);
    EXPECT_EQ(zombie.melee_cut, melee_cut_bonus);
    EXPECT_EQ(zombie.sk_dodge, dodge);
    EXPECT_EQ(zombie.armor, armor);
    EXPECT_EQ(zombie.item_chance, item_chance);
    EXPECT_EQ(zombie.hp, hit_points);
    EXPECT_EQ(zombie.sp_freq, special_frequency);
    EXPECT_EQ(zombie.dies, death_function);
    EXPECT_EQ(zombie.special_attack, special_attack_function);
}
