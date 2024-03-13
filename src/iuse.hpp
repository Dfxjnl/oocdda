#ifndef OOCDDA_IUSE_HPP
#define OOCDDA_IUSE_HPP

namespace oocdda {
class Game;
class Item;

class iuse {
public:
    void none(Game* g, Item* it, bool t);

    // Food and drugs (administration).
    void royal_jelly(Game* g, Item* it, bool t);
    void bandage(Game* g, Item* it, bool t);
    void firstaid(Game* g, Item* it, bool t);
    void caff(Game* g, Item* it, bool t);
    void alcohol(Game* g, Item* it, bool t);
    void pkill_1(Game* g, Item* it, bool t);
    void pkill_2(Game* g, Item* it, bool t);
    void pkill_3(Game* g, Item* it, bool t);
    void pkill_4(Game* g, Item* it, bool t);
    void pkill_l(Game* g, Item* it, bool t);
    void xanax(Game* g, Item* it, bool t);
    void cig(Game* g, Item* it, bool t);
    void weed(Game* g, Item* it, bool t);
    void coke(Game* g, Item* it, bool t);
    void meth(Game* g, Item* it, bool t);
    void poison(Game* g, Item* it, bool t);
    void hallu(Game* g, Item* it, bool t);
    void thorazine(Game* g, Item* it, bool t);
    void prozac(Game* g, Item* it, bool t);
    void sleep(Game* g, Item* it, bool t);
    void flumed(Game* g, Item* it, bool t);
    void flusleep(Game* g, Item* it, bool t);
    void inhaler(Game* g, Item* it, bool t);
    void blech(Game* g, Item* it, bool t);
    void mutagen(Game* g, Item* it, bool t);
    void purifier(Game* g, Item* it, bool t);
    // TOOLS
    void lighter(Game* g, Item* it, bool t);
    void sew(Game* g, Item* it, bool t);
    void scissors(Game* g, Item* it, bool t);
    void extinguisher(Game* g, Item* it, bool t);
    void hammer(Game* g, Item* it, bool t);
    void light_off(Game* g, Item* it, bool t);
    void light_on(Game* g, Item* it, bool t);
    void water_purifier(Game* g, Item* it, bool t);
    void two_way_radio(Game* g, Item* it, bool t);
    void radio_off(Game* g, Item* it, bool t);
    void radio_on(Game* g, Item* it, bool t);
    void crowbar(Game* g, Item* it, bool t);
    void makemound(Game* g, Item* it, bool t);
    void dig(Game* g, Item* it, bool t);
    void chainsaw_off(Game* g, Item* it, bool t);
    void chainsaw_on(Game* g, Item* it, bool t);
    void jackhammer(Game* g, Item* it, bool t);
    void set_trap(Game* g, Item* it, bool t);
    void geiger(Game* g, Item* it, bool t);
    void teleport(Game* g, Item* it, bool t);
    void can_goo(Game* g, Item* it, bool t);
    void grenade(Game* g, Item* it, bool t);
    void grenade_act(Game* g, Item* it, bool t);
    void gasbomb(Game* g, Item* it, bool t);
    void gasbomb_act(Game* g, Item* it, bool t);
    void smokebomb(Game* g, Item* it, bool t);
    void smokebomb_act(Game* g, Item* it, bool t);
    void molotov(Game* g, Item* it, bool t);
    void molotov_lit(Game* g, Item* it, bool t);
    void dynamite(Game* g, Item* it, bool t);
    void dynamite_act(Game* g, Item* it, bool t);
    void mininuke(Game* g, Item* it, bool t);
    void mininuke_act(Game* g, Item* it, bool t);
    void pheromone(Game* g, Item* it, bool t);
    void portal(Game* g, Item* it, bool t);
};
} // namespace oocdda

#endif // OOCDDA_IUSE_HPP
