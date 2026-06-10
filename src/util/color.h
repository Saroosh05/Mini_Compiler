#ifndef COLOR_H
#define COLOR_H

#include <string>
#include <ostream>

// ANSI color/style helpers.
// Call Color::disable() to turn off colors (e.g. when output is redirected to a file).
namespace Color {

inline bool& _enabled() {
    static bool v = true;
    return v;
}

inline void enable()  { _enabled() = true;  }
inline void disable() { _enabled() = false; }
inline bool isEnabled() { return _enabled(); }

inline const char* reset()       { return _enabled() ? "\033[0m"     : ""; }
inline const char* bold()        { return _enabled() ? "\033[1m"     : ""; }
inline const char* dim()         { return _enabled() ? "\033[2m"     : ""; }

inline const char* red()         { return _enabled() ? "\033[31m"    : ""; }
inline const char* green()       { return _enabled() ? "\033[32m"    : ""; }
inline const char* yellow()      { return _enabled() ? "\033[33m"    : ""; }
inline const char* blue()        { return _enabled() ? "\033[34m"    : ""; }
inline const char* magenta()     { return _enabled() ? "\033[35m"    : ""; }
inline const char* cyan()        { return _enabled() ? "\033[36m"    : ""; }
inline const char* white()       { return _enabled() ? "\033[37m"    : ""; }

inline const char* boldRed()     { return _enabled() ? "\033[1;31m"  : ""; }
inline const char* boldGreen()   { return _enabled() ? "\033[1;32m"  : ""; }
inline const char* boldYellow()  { return _enabled() ? "\033[1;33m"  : ""; }
inline const char* boldBlue()    { return _enabled() ? "\033[1;34m"  : ""; }
inline const char* boldMagenta() { return _enabled() ? "\033[1;35m"  : ""; }
inline const char* boldCyan()    { return _enabled() ? "\033[1;36m"  : ""; }
inline const char* boldWhite()   { return _enabled() ? "\033[1;37m"  : ""; }

// Convenience: wrap a string in a color and reset
inline std::string paint(const char* colorCode, const std::string& s) {
    if (!_enabled()) return s;
    return std::string(colorCode) + s + reset();
}

} // namespace Color

#endif // COLOR_H
