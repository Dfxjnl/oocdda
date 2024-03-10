#ifndef OOCDDA_SETVECTOR_HPP
#define OOCDDA_SETVECTOR_HPP

#include <vector>

#include "itype.hpp"
#include "monster_type.hpp"

namespace oocdda {
struct component;
struct items_location_and_chance;

void setvector(std::vector<itype_id>& vec, ...);
void setvector(std::vector<component>& vec, ...);
void setvector(std::vector<mon_id>& vec, ...);
void setvector(std::vector<items_location_and_chance>& vec, ...);
} // namespace oocdda

#endif // OOCDDA_SETVECTOR_HPP
