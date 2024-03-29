#ifndef OOCDDA_KEYPRESS_HPP
#define OOCDDA_KEYPRESS_HPP

namespace oocdda {
// Simple text input--translates numpad to vikeys
long input();
// If ch is vikey, x & y are set to corresponding direction; ch=='y'->x=-1,y=-1
void get_direction(int& x, int& y, char ch);

#define CTRL(n) (n - 'A' + 1 < 1 ? n - 'a' + 1 : n - 'A' + 1)
#define KEY_ESCAPE 27
} // namespace oocdda

#endif // OOCDDA_KEYPRESS_HPP
