#ifndef OOCDDA_LINE_HPP
#define OOCDDA_LINE_HPP

#include <string>
#include <vector>

#include "point.hpp"

namespace oocdda {
// The "t" value decides WHICH Bresenham line is used.
std::vector<Point> line_to(int x1, int y1, int x2, int y2, int t);
int trig_dist(int x1, int y1, int x2, int y2);
int rl_dist(int x1, int y1, int x2, int y2);
std::string direction_from(int x1, int y1, int x2, int y2);
} // namespace oocdda

#endif // OOCDDA_LINE_HPP
