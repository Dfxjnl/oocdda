#ifndef OOCDDA_MONSTER_ATTACK_HPP
#define OOCDDA_MONSTER_ATTACK_HPP

namespace oocdda {
class Game;
class Monster;

class mattack {
public:
    void none(Game* g, Monster* z) {};
    void antqueen(Game* g, Monster* z);
    void shriek(Game* g, Monster* z);
    void acid(Game* g, Monster* z);
    void shockstorm(Game* g, Monster* z);
    void boomer(Game* g, Monster* z);
    void resurrect(Game* g, Monster* z);
    void growplants(Game* g, Monster* z);
    void fungus(Game* g, Monster* z);
    void plant(Game* g, Monster* z);
    void disappear(Game* g, Monster* z);
    void formblob(Game* g, Monster* z);
    void gene_sting(Game* g, Monster* z);
    void stare(Game* g, Monster* z);
    void photograph(Game* g, Monster* z) {}; // TODO: needs faction
    void tazer(Game* g, Monster* z);
    void smg(Game* g, Monster* z);
    void flamethrower(Game* g, Monster* z);
    void multi_robot(Game* g, Monster* z); // Pick from tazer, smg, flame
};
} // namespace oocdda

#endif // OOCDDA_MONSTER_ATTACK_HPP
