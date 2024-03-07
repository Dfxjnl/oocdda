#ifndef OOCDDA_ENUMS_HPP
#define OOCDDA_ENUMS_HPP

#include <stdexcept>

namespace oocdda {
#ifndef sgn
#    define sgn(x) (((x) < 0) ? -1 : 1)
#endif

/**
 * \brief Calculates the bit position in a bitfield for a given flag.
 *
 * This function converts a flag (represented as an unsigned integer) to its corresponding bit
 * position in a bitfield. It's useful for setting, clearing, or checking specific flags in a
 * bitfield.
 *
 * \param value The flag value to convert to a bit position. This must be a non-negative integer
 * within the range of the bitfield size.
 *
 * \return The bit position corresponding to the given flag value. The position is represented as an
 * unsigned integer where the bit at the position is set to 1.
 *
 * \throws std::out_of_range if the provided value exceeds the maximum bit position that can be
 * represented by an unsigned int.
 */
[[nodiscard]] constexpr auto flag_to_bit_position(const unsigned int value) -> unsigned int
{
    constexpr unsigned int max_value { sizeof(unsigned int) * 8 - 1 };

    if (value > max_value) {
        throw std::out_of_range("value out of range");
    }

    return 1U << value;
}

enum material {
    MNULL = 0,
    // Food Materials
    LIQUID,
    VEGGY,
    FLESH,
    POWDER,
    // Clothing
    COTTON,
    WOOL,
    LEATHER,
    KEVLAR,
    // Other
    STONE,
    PAPER,
    WOOD,
    PLASTIC,
    GLASS,
    IRON,
    STEEL,
    SILVER
};

struct point {
    int x;
    int y;
    point(int X = 0, int Y = 0)
        : x(X)
        , y(Y)
    {
    }
    ~point() { }
};
} // namespace oocdda

#endif // OOCDDA_ENUMS_HPP
