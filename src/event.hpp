#ifndef OOCDDA_EVENT_HPP
#define OOCDDA_EVENT_HPP

#include <cstddef>

namespace oocdda {
class game;
struct faction;

enum event_type { EVENT_NULL, EVENT_HELP, NUM_EVENT_TYPES };

struct event {
    event_type type;
    int turn;
    faction* relevant_faction;

    event()
    {
        type = EVENT_NULL;
        turn = 0;
        relevant_faction = NULL;
    }

    event(event_type e_t, int t, faction* r_f)
    {
        type = e_t;
        turn = t;
        relevant_faction = r_f;
    }

    void actualize(game* g);
};
} // namespace oocdda

#endif // OOCDDA_EVENT_HPP
