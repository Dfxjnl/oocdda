#ifndef _SETVECTOR_H_
#define _SETVECTOR_H_
#include <vector>

#include "crafting.h"
#include "itype.h"
#include "mapitems.h"
#include "mongroup.h"
void setvector(std::vector<itype_id>& vec, ...);
void setvector(std::vector<component>& vec, ...);
void setvector(std::vector<mon_id>& vec, ...);
void setvector(std::vector<items_location_and_chance>& vec, ...);
#endif
