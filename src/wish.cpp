#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

#include <ncurses/curses.h>

#include "color.hpp"
#include "game.hpp"
#include "item.hpp"
#include "itype.hpp"
#include "output.hpp"
#include "player.hpp"

namespace oocdda {
#define LESS(a, b) ((a) < (b) ? (a) : (b))

void Game::wish()
{
    int a = 0, shift = 0;
    int line;
    char ch {0};
    bool grep = false, found = false;
    int search;
    std::string pattern;
    std::string info;
    Item tmp;
    tmp.corpse = mtypes[0];
    do {
        erase();
        mvprintw(0, 0, "Wish for: ");
        if (grep) {
            found = false;
            if (ch == '\n') {
                grep = false;
                found = true;
                pattern = "";
            } else {
                pattern = pattern + ch;
                if (ch == ' ')
                    ch = '.'; // Don't escape the wish function yet!
            }

            for (search = 0; search < std::ssize(itypes) && !found; ++search) {
                if (itypes[search]->name.find(pattern) != std::string::npos) {
                    shift = search;
                    a = 0;

                    if (shift + 23 > static_cast<int>(itypes.size())) {
                        a = shift + 23 - static_cast<int>(itypes.size());
                        shift = static_cast<int>(itypes.size()) - 23;
                    }

                    found = true;
                }
            }

            if (found)
                mvprintw(1, 0, "%s               ", pattern.c_str());
            else if (grep)
                mvprintz(1, 0, c_red, "%s not found!             ", pattern.c_str());
            else
                mvprintw(1, 0, "                      ");
        } else { // Not grepping; scroll by keys
            if (ch == 'j')
                a++;
            if (ch == 'k')
                a--;
            if (ch == '/') {
                grep = true;
                pattern = "";
            }
        }
        if (a < 0) {
            a = 0;
            shift--;
            if (shift < 0)
                shift = 0;
        }
        if (a > 22) {
            a = 22;
            shift++;

            if (shift + 23 > static_cast<int>(itypes.size())) {
                shift = static_cast<int>(itypes.size()) - 23;
            }
        }

        for (std::size_t i {1}; i < LESS(24, itypes.size()); ++i) {
            mvprintz(static_cast<int>(i), 40, nc_color::c_white,
                     itypes[i - 1 + shift]->name.c_str());
            printz(itypes[i - 1 + shift]->color, "%c%", itypes[i - 1 + shift]->sym);
        }

        tmp.make(itypes[a + shift]);
        if (tmp.is_tool())
            tmp.charges = dynamic_cast<it_tool*>(tmp.type)->def_charges;
        else if (tmp.is_ammo())
            tmp.charges = 100;
        info = tmp.info();
        line = 2;
        mvprintw(line, 1, "%s", info.c_str());
        ch = getch();
    } while (ch != ' ');
    clear();
    mvprintw(0, 0, "\nWish granted.");
    tmp.invlet = nextinv;
    u.i_add(tmp);
    advance_nextinv();
    getch();
}
} // namespace oocdda
