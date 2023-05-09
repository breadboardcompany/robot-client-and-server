#pragma once

template <typename T> static constexpr void encode(unsigned char *dst, T src) {
  static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
  if constexpr (sizeof(T) == 2) {
    dst[0] = src;
    dst[1] = src >> 8;
  } else if constexpr (sizeof(T) == 4) {
    dst[0] = src;
    dst[1] = src >> 8;
    dst[2] = src >> 16;
    dst[3] = src >> 24;
  } else if constexpr (sizeof(T) == 8) {
    dst[0] = src;
    dst[1] = src >> 8;
    dst[2] = src >> 16;
    dst[3] = src >> 24;
    dst[4] = src >> 32;
    dst[5] = src >> 40;
    dst[6] = src >> 48;
    dst[7] = src >> 56;
  }
}

template <typename T> constexpr T decode(unsigned char const *src) {
  static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
  if constexpr (sizeof(T) == 2) {
    return static_cast<T>(src[0]) | (static_cast<T>(src[1]) << 8);
  } else if constexpr (sizeof(T) == 4) {
    return static_cast<T>(src[0]) | (static_cast<T>(src[1]) << 8) |
           (static_cast<T>(src[2]) << 16) | (static_cast<T>(src[3]) << 24);
  } else if constexpr (sizeof(T) == 8) {
    return static_cast<T>(src[0]) | (static_cast<T>(src[1]) << 8) |
           (static_cast<T>(src[2]) << 16) | (static_cast<T>(src[3]) << 24)
    | (static_cast<T>(src[4]) << 32) | (static_cast<T>(src[5]) << 40) |
           (static_cast<T>(src[6]) << 48) | (static_cast<T>(src[7]) << 56);
  }
}
