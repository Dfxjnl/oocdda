#ifndef OOCDDA_FACTION_HPP
#define OOCDDA_FACTION_HPP

#include <string>
#include <vector>

namespace oocdda {
class Game;

// TODO: Redefine?
#define MAX_FAC_NAME_SIZE 40

std::string fac_ranking_text(int val);
std::string fac_respect_text(int val);

enum faction_goal {
    FACGOAL_NULL = 0,     // Not meant to be used; denotes a bug
    FACGOAL_NONE,         // No particular goal outside of prosperity
    FACGOAL_WEALTH,       // Financial wealth
    FACGOAL_DOMINANCE,    // Complete military dominance
    FACGOAL_CLEANSE,      // Destroy zombies &c
    FACGOAL_SHADOW,       // Contact, control, etc. of the shadow world
    FACGOAL_APOCALYPSE,   // Destroy the world!
    FACGOAL_ANARCHY,      // Nihilistic; destroy anything
    FACGOAL_KNOWLEDGE,    // Golden age of knowledge; a new renaissance
    FACGOAL_NATURE,       // Golden age of ecological harmony
    FACGOAL_CIVILIZATION, // Rebuilding society as it was before
    NUM_FACGOALS
};

enum faction_job {
    FACJOB_NULL = 0,
    FACJOB_EXTORTION,   // Protection rackets, etc
    FACJOB_INFORMATION, // Gathering & sale of information
    FACJOB_TRADE,       // Bazaars, etc.
    FACJOB_CARAVANS,    // Transportation of goods
    FACJOB_SCAVENGE,    // Scavenging & sale of general supplies
    FACJOB_MERCENARIES, // General fighters-for-hire
    FACJOB_ASSASSINS,   // Targeted discreet killing
    FACJOB_RAIDERS,     // Raiding settlements, trade routes, &c
    FACJOB_THIEVES,     // Less violent; theft of property without killing
    FACJOB_GAMBLING,    // Maitenance of gambling parlors
    FACJOB_DOCTORS,     // Doctors for hire
    FACJOB_FARMERS,     // Farming & sale of food
    FACJOB_DRUGS,       // Drug dealing
    FACJOB_MANUFACTURE, // Manufacturing & selling goods
    NUM_FACJOBS
};

enum faction_value {
    FACVAL_NULL = 0,
    FACVAL_CHARITABLE,   // Give their job for free (often)
    FACVAL_LONERS,       // Avoid associating with outsiders
    FACVAL_EXPLORATION,  // Like to explore new territory
    FACVAL_ARTIFACTS,    // Seek out unique items
    FACVAL_BIONICS,      // Collection & installation of bionic parts
    FACVAL_BOOKS,        // Collection of books
    FACVAL_TRAINING,     // Training among themselves and others
    FACVAL_ROBOTS,       // Creation / use of robots
    FACVAL_TREACHERY,    // Untrustworthy
    FACVAL_STRAIGHTEDGE, // No drugs, alcohol, etc.
    FACVAL_LAWFUL,       // Abide by the old laws
    FACVAL_CRUELTY,      // Torture, murder, etc.
    NUM_FACVALS
};

struct faction {
    faction();
    ~faction();

    void randomize();
    void make_army();
    bool has_job(faction_job j);
    bool has_value(faction_value v);
    bool matches_us(faction_value v);
    std::string describe();

    int response_time(Game* g); // Time it takes for them to get to u

    std::string name;
    unsigned values : NUM_FACVALS; // Bitfield of values
    faction_goal goal;
    faction_job job1, job2;
    std::vector<int> opinion_of;
    int likes_u;
    int respects_u;
    bool known_by_u;
    int id;
    int strength, sneak, crime, cult, good; // Defining values
    int omx, omy;                           // Which overmap are we based in?
    int mapx, mapy;                         // Where in that overmap are we?
    int size;                               // How big is our sphere of influence?
    int power;                              // General measure of our power
};
} // namespace oocdda

#endif // OOCDDA_FACTION_HPP
