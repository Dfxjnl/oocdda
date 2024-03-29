#include "bodypart.hpp"

#include "rng.hpp"

namespace oocdda {
std::string body_part_name(body_part bp, int side)
{
    switch (bp) {
    case bp_head:
        return "head";
    case bp_eyes:
        return "eyes";
    case bp_mouth:
        return "jaw";
    case bp_torso:
        return "torso";
    case bp_arms:
        if (side == 0)
            return "left arm";
        if (side == 1)
            return "right arm";
        return "arms";
    case bp_hands:
        if (side == 0)
            return "left hand";
        if (side == 1)
            return "right hand";
        return "hands";
    case bp_legs:
        if (side == 0)
            return "left leg";
        if (side == 1)
            return "right leg";
        return "legs";
    case bp_feet:
        if (side == 0)
            return "left foot";
        if (side == 1)
            return "right foot";
        return "feet";
    default:
        return "appendix";
    }
}

body_part random_body_part()
{
    int rn = rng(0, 30);
    if (rn == 0)
        return bp_eyes;
    if (rn <= 2)
        return bp_mouth;
    if (rn <= 6)
        return bp_head;
    if (rn <= 12)
        return bp_legs;
    if (rn <= 20)
        return bp_arms;
    return bp_torso;
}
} // namespace oocdda
