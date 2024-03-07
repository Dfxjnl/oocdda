#ifndef OOCDDA_MONATTACK_HPP
#define OOCDDA_MONATTACK_HPP

namespace oocdda {
class Game;
class monster;

class mattack {
public:
    void none(Game* g, monster* z) {};
    void antqueen(Game* g, monster* z);
    void shriek(Game* g, monster* z);
    void acid(Game* g, monster* z);
    void shockstorm(Game* g, monster* z);
    void boomer(Game* g, monster* z);
    void resurrect(Game* g, monster* z);
    void growplants(Game* g, monster* z);
    void fungus(Game* g, monster* z);
    void plant(Game* g, monster* z);
    void disappear(Game* g, monster* z);
    void formblob(Game* g, monster* z);
    void gene_sting(Game* g, monster* z);
    void stare(Game* g, monster* z);
    void photograph(Game* g, monster* z) {}; // TODO: needs faction
    void tazer(Game* g, monster* z);
    void smg(Game* g, monster* z);
    void flamethrower(Game* g, monster* z);
    void multi_robot(Game* g, monster* z); // Pick from tazer, smg, flame
};
} // namespace oocdda

#endif // OOCDDA_MONATTACK_HPP
