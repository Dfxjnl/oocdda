#ifndef OOCDDA_POINT_HPP
#define OOCDDA_POINT_HPP

#include <cstddef>
#include <vector>

namespace oocdda {
struct Point {
    constexpr auto operator<=>(const Point&) const noexcept = default;

    int x {0};
    int y {0};
};

[[nodiscard]] constexpr auto find_adjacent_points(const Point center, const int range = 1) noexcept
    -> std::vector<Point>
{
    if (range < 0) {
        return {};
    }

    if (range == 0) {
        return {center};
    }

    std::vector<Point> adjacent_points;
    adjacent_points.reserve((2 * static_cast<std::size_t>(range) + 1)
                            * (2 * static_cast<std::size_t>(range) + 1));

    for (int x {center.x - range}; x <= center.x + range; ++x) {
        for (int y {center.y - range}; y <= center.y + range; ++y) {
            adjacent_points.emplace_back(Point {x, y});
        }
    }

    return adjacent_points;
}
} // namespace oocdda

#endif // OOCDDA_POINT_HPP
