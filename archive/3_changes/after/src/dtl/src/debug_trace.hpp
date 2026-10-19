#ifndef DTL_DEBUG_TRACE_HPP
#define DTL_DEBUG_TRACE_HPP

#include <cstdarg>
#include <cstdio>

namespace DTL {
// Flush every checkpoint: the next operation may block on a DTU access.
inline void DebugTrace(const char* format, ...) {
    std::fputs("[DTL-TRACE] ", stderr);
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
    std::fflush(stderr);
}
}

#endif
