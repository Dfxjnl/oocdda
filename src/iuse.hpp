#ifndef OOCDDA_IUSE_HPP
#define OOCDDA_IUSE_HPP

namespace oocdda {
class Game;
class item;

class iuse {
public:
    void none(Game* g, item* it, bool t) {};
    // FOOD AND DRUGS (ADMINISTRATION)
    void royal_jelly(Game* g, item* it, bool t);
    void bandage(Game* g, item* it, bool t);
    void firstaid(Game* g, item* it, bool t);
    void caff(Game* g, item* it, bool t);
    void alcohol(Game* g, item* it, bool t);
    void pkill_1(Game* g, item* it, bool t);
    void pkill_2(Game* g, item* it, bool t);
    void pkill_3(Game* g, item* it, bool t);
    void pkill_4(Game* g, item* it, bool t);
    void pkill_l(Game* g, item* it, bool t);
    void xanax(Game* g, item* it, bool t);
    void cig(Game* g, item* it, bool t);
    void weed(Game* g, item* it, bool t);
    void coke(Game* g, item* it, bool t);
    void meth(Game* g, item* it, bool t);
    void poison(Game* g, item* it, bool t);
    void hallu(Game* g, item* it, bool t);
    void thorazine(Game* g, item* it, bool t);
    void prozac(Game* g, item* it, bool t);
    void sleep(Game* g, item* it, bool t);
    void flumed(Game* g, item* it, bool t);
    void flusleep(Game* g, item* it, bool t);
    void inhaler(Game* g, item* it, bool t);
    void blech(Game* g, item* it, bool t);
    void mutagen(Game* g, item* it, bool t);
    void purifier(Game* g, item* it, bool t);
    // TOOLS
    void lighter(Game* g, item* it, bool t);
    void sew(Game* g, item* it, bool t);
    void scissors(Game* g, item* it, bool t);
    void extinguisher(Game* g, item* it, bool t);
    void hammer(Game* g, item* it, bool t);
    void light_off(Game* g, item* it, bool t);
    void light_on(Game* g, item* it, bool t);
    void water_purifier(Game* g, item* it, bool t);
    void two_way_radio(Game* g, item* it, bool t);
    void radio_off(Game* g, item* it, bool t);
    void radio_on(Game* g, item* it, bool t);
    void crowbar(Game* g, item* it, bool t);
    void makemound(Game* g, item* it, bool t);
    void dig(Game* g, item* it, bool t);
    void chainsaw_off(Game* g, item* it, bool t);
    void chainsaw_on(Game* g, item* it, bool t);
    void jackhammer(Game* g, item* it, bool t);
    void set_trap(Game* g, item* it, bool t);
    void geiger(Game* g, item* it, bool t);
    void teleport(Game* g, item* it, bool t);
    void can_goo(Game* g, item* it, bool t);
    void grenade(Game* g, item* it, bool t);
    void grenade_act(Game* g, item* it, bool t);
    void gasbomb(Game* g, item* it, bool t);
    void gasbomb_act(Game* g, item* it, bool t);
    void smokebomb(Game* g, item* it, bool t);
    void smokebomb_act(Game* g, item* it, bool t);
    void molotov(Game* g, item* it, bool t);
    void molotov_lit(Game* g, item* it, bool t);
    void dynamite(Game* g, item* it, bool t);
    void dynamite_act(Game* g, item* it, bool t);
    void mininuke(Game* g, item* it, bool t);
    void mininuke_act(Game* g, item* it, bool t);
    void pheromone(Game* g, item* it, bool t);
    void portal(Game* g, item* it, bool t);
};
} // namespace oocdda

#endif // OOCDDA_IUSE_HPP
