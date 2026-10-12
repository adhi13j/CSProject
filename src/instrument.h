/**
 * @file instrument.h
 * @brief Optional counters for the benchmark's `aux_peak` metric (SPEC 8.2, 12.6).
 *
 * Private to `src/` and `bench/`: it is not part of the public contract in
 * `include/formula.h`. Without `-DBENCH_INSTRUMENT` every macro expands to
 * nothing, so the normal build, `make test` and the CLI carry no overhead.
 *
 * - AUX_NOTE(size): record a stack / queue length (iterative code).
 * - AUX_ENTER():    record one more live call frame until the end of the enclosing scope (recursive code).
 *
 * `bench::auxPeak` is the largest value noted since the last reset.
 */
#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include <cstddef>

#ifdef BENCH_INSTRUMENT

namespace bench {
inline std::size_t auxPeak = 0;    ///< peak auxiliary structure size / recursion depth since reset
inline std::size_t auxDepth = 0;   ///< live recursion depth right now

/** @brief Forget the previous peak; call before each measured run. */
inline void auxReset() {
    auxPeak = 0;
    auxDepth = 0;
}

/** @brief RAII marker for one live recursive call. */
struct AuxFrame {
    AuxFrame() {
        if (++auxDepth > auxPeak) {
            auxPeak = auxDepth;
        }
    }
    ~AuxFrame() { --auxDepth; }
};
} // namespace bench

#define AUX_NOTE(size)                                \
    do {                                              \
        if ((size) > bench::auxPeak) {                \
            bench::auxPeak = (size);                  \
        }                                             \
    } while (0)
#define AUX_ENTER() bench::AuxFrame auxFrameGuard_

#else

#define AUX_NOTE(size) ((void)0)
#define AUX_ENTER() ((void)0)

#endif // BENCH_INSTRUMENT

#endif // INSTRUMENT_H
