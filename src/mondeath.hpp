#ifndef OOCDDA_MONDEATH_HPP
#define OOCDDA_MONDEATH_HPP

namespace oocdda {
class Game;
class monster;

class mdeath {
public:
    void normal(Game* g, monster* z);      // Empty function
    void acid(Game* g, monster* z);        // Drops acid around the body
    void boomer(Game* g, monster* z);      // Explodes in vomit :3
    void fungus(Game* g, monster* z);      // Explodes in spores D:
    void fungusawake(Game* g, monster* z); // Turn into live fungaloid
    void shriek(Game* g, monster* z);      // Screams loudly
    void worm(Game* g, monster* z);        // Spawns 2 half-worms
    void disappear(Game* g, monster* z);   // Hallucination disappears
    void guilt(Game* g, monster* z);       // Morale penalty
    void blobsplit(Game* g, monster* z);   // Creates more blobs
    void melt(Game* g, monster* z);        // Normal death, but melts
    void explode(Game* g, monster* z);     // Damaging explosion
};
} // namespace oocdda

#endif // OOCDDA_MONDEATH_HPP
