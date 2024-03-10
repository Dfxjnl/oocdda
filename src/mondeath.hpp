#ifndef OOCDDA_MONDEATH_HPP
#define OOCDDA_MONDEATH_HPP

namespace oocdda {
class Game;
class Monster;

class mdeath {
public:
    void normal(Game* g, Monster* z);      // Empty function
    void acid(Game* g, Monster* z);        // Drops acid around the body
    void boomer(Game* g, Monster* z);      // Explodes in vomit :3
    void fungus(Game* g, Monster* z);      // Explodes in spores D:
    void fungusawake(Game* g, Monster* z); // Turn into live fungaloid
    void shriek(Game* g, Monster* z);      // Screams loudly
    void worm(Game* g, Monster* z);        // Spawns 2 half-worms
    void disappear(Game* g, Monster* z);   // Hallucination disappears
    void guilt(Game* g, Monster* z);       // Morale penalty
    void blobsplit(Game* g, Monster* z);   // Creates more blobs
    void melt(Game* g, Monster* z);        // Normal death, but melts
    void explode(Game* g, Monster* z);     // Damaging explosion
};
} // namespace oocdda

#endif // OOCDDA_MONDEATH_HPP
