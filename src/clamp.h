#include "WProgram.h"

template <typename A, typename B, typename C>
constexpr auto clamp(A _val, B _min, C _max) {
    return max(_min, min(_val, _max));
}

template <class T, class A, class B, class C, class D>
constexpr auto cmap(T val, A in_min, B in_max, C out_min, D out_max) {
    return clamp(map(val, in_min, in_max, out_min, out_max), out_min, out_max);
}