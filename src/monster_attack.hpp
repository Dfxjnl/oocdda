#ifndef OOCDDA_MONSTER_ATTACK_HPP
#define OOCDDA_MONSTER_ATTACK_HPP

namespace oocdda {
class Game;
class Monster;

namespace monster_attack {
void antqueen(Game& game, Monster& monster);
void shriek(Game& game, Monster& monster);
void acid(Game& game, Monster& monster);
void shockstorm(Game& game, Monster& monster);
void boomer(Game& game, Monster& monster);
void resurrect(Game& game, Monster& monster);
void growplants(Game& game, Monster& monster);
void fungus(Game& game, Monster& monster);
void plant(Game& game, Monster& monster);
void disappear(Game& game, Monster& monster);
void formblob(Game& game, Monster& monster);
void gene_sting(Game& game, Monster& monster);
void stare(Game& game, Monster& monster);
void photograph(Game& game, Monster& monster);
void tazer(Game& game, Monster& monster);
void smg(Game& game, Monster& monster);
void flamethrower(Game& game, Monster& monster);
void multi_robot(Game& game, Monster& monster); // Pick from tazer, SMG, flamethrower.
} // namespace monster_attack
} // namespace oocdda

#endif // OOCDDA_MONSTER_ATTACK_HPP
