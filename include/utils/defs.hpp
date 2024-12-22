#pragma once

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <emmintrin.h>
static inline void spin_loop_pause() noexcept
{
  _mm_pause();
}
#endif