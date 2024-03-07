#ifndef _SETVECTOR_H_
#define _SETVECTOR_H_

#include <vector>

#include "itype.h"
#include "mtype.h"

struct component;
struct items_location_and_chance;

void setvector(std::vector<itype_id>& vec, ...);
void setvector(std::vector<component>& vec, ...);
void setvector(std::vector<mon_id>& vec, ...);
void setvector(std::vector<items_location_and_chance>& vec, ...);

#endif
