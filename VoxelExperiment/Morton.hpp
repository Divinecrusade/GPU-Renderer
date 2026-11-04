#ifndef VOXEL_MORTON_HPP
#define VOXEL_MORTON_HPP

#include <cstdint>

// Кодирование Мортона: чередование битов x, y, z (до 21 бита на ось, N <= 2^21).
// Младшие 3 бита кода — номер октанта: бит 0 — x, бит 1 — y, бит 2 — z.
// После сортировки по коду дети одного узла октодерева идут подряд.
namespace gpu_renderer::voxel {
struct VoxelCoord {
  std::uint32_t x;
  std::uint32_t y;
  std::uint32_t z;
};

[[nodiscard]] constexpr std::uint64_t SplitBitsBy3(std::uint32_t value) noexcept {
  std::uint64_t x = value & 0x1fffffu;
  x = (x | x << 32u) & 0x1f00000000ffffULL;
  x = (x | x << 16u) & 0x1f0000ff0000ffULL;
  x = (x | x << 8u) & 0x100f00f00f00f00fULL;
  x = (x | x << 4u) & 0x10c30c30c30c30c3ULL;
  x = (x | x << 2u) & 0x1249249249249249ULL;
  return x;
}

[[nodiscard]] constexpr std::uint32_t CompactBitsBy3(std::uint64_t code) noexcept {
  std::uint64_t x = code & 0x1249249249249249ULL;
  x = (x ^ (x >> 2u)) & 0x10c30c30c30c30c3ULL;
  x = (x ^ (x >> 4u)) & 0x100f00f00f00f00fULL;
  x = (x ^ (x >> 8u)) & 0x1f0000ff0000ffULL;
  x = (x ^ (x >> 16u)) & 0x1f00000000ffffULL;
  x = (x ^ (x >> 32u)) & 0x1fffffULL;
  return static_cast<std::uint32_t>(x);
}

[[nodiscard]] constexpr std::uint64_t EncodeMorton(std::uint32_t x, std::uint32_t y,
                                                   std::uint32_t z) noexcept {
  return SplitBitsBy3(x) | (SplitBitsBy3(y) << 1u) | (SplitBitsBy3(z) << 2u);
}

[[nodiscard]] constexpr VoxelCoord DecodeMorton(std::uint64_t code) noexcept {
  return {CompactBitsBy3(code), CompactBitsBy3(code >> 1u), CompactBitsBy3(code >> 2u)};
}

static_assert(DecodeMorton(EncodeMorton(5u, 1023u, 77u)).y == 1023u);
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_MORTON_HPP
