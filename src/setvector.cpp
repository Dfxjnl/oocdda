#include <cstdarg>

#include "setvector.hpp"

#include "crafting.hpp"
#include "mapitems.hpp"

namespace oocdda {
void setvector(std::vector<itype_id>& vec, ...)
{
    va_list ap;
    va_start(ap, vec);
    itype_id tmp;

    while ((tmp = static_cast<itype_id>(va_arg(ap, int))) != 0U) {
        vec.push_back(tmp);
    }

    va_end(ap);
}

void setvector(std::vector<component>& vec, ...)
{
    va_list ap;
    va_start(ap, vec);
    itype_id it_tmp;
    int n_tmp;

    while ((it_tmp = static_cast<itype_id>(va_arg(ap, int))) != 0U) {
        n_tmp = va_arg(ap, int);
        vec.emplace_back(it_tmp, n_tmp);
    }

    va_end(ap);
}

void setvector(std::vector<mon_id>& vec, ...)
{
    va_list ap;
    va_start(ap, vec);
    mon_id tmp;

    while ((tmp = static_cast<mon_id>(va_arg(ap, int))) != 0U) {
        vec.push_back(tmp);
    }

    va_end(ap);
}

void setvector(std::vector<items_location_and_chance>& vec, ...)
{
    va_list ap;
    va_start(ap, vec);
    items_location tmploc;
    int tmpchance;

    while ((tmploc = static_cast<items_location>(va_arg(ap, int))) != 0U) {
        tmpchance = va_arg(ap, int);
        vec.emplace_back(tmploc, tmpchance);
    }

    va_end(ap);
}
} // namespace oocdda
