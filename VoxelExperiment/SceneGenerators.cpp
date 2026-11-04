#include "SceneGenerators.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Morton.hpp"

namespace gpu_renderer::voxel {
namespace {
// ───────────── Value-шум с фрактальным суммированием (fBm), значения ~[-1, 1] ─────────────
[[nodiscard]] std::uint32_t Hash3(int x, int y, int z, std::uint32_t seed) noexcept {
  std::uint32_t h = seed ^ (static_cast<std::uint32_t>(x) * 0x8da6b343u) ^
                    (static_cast<std::uint32_t>(y) * 0xd8163841u) ^
                    (static_cast<std::uint32_t>(z) * 0xcb1ab31fu);
  h ^= h >> 13u;
  h *= 0x5bd1e995u;
  h ^= h >> 15u;
  return h;
}

[[nodiscard]] double ValueNoise(double x, double y, double z, std::uint32_t seed) noexcept {
  int const ix = static_cast<int>(std::floor(x));
  int const iy = static_cast<int>(std::floor(y));
  int const iz = static_cast<int>(std::floor(z));
  double const fx = x - ix;
  double const fy = y - iy;
  double const fz = z - iz;
  double const u = fx * fx * (3.0 - 2.0 * fx);
  double const v = fy * fy * (3.0 - 2.0 * fy);
  double const w = fz * fz * (3.0 - 2.0 * fz);
  auto const corner = [&](int a, int b, int c) {
    return Hash3(ix + a, iy + b, iz + c, seed) / 2147483647.5 - 1.0;
  };
  auto const lerp = [](double a, double b, double t) { return a + (b - a) * t; };
  return lerp(lerp(lerp(corner(0, 0, 0), corner(1, 0, 0), u), lerp(corner(0, 1, 0), corner(1, 1, 0), u), v),
              lerp(lerp(corner(0, 0, 1), corner(1, 0, 1), u), lerp(corner(0, 1, 1), corner(1, 1, 1), u), v), w);
}

[[nodiscard]] double FractalNoise(double x, double y, double z, int octaves, std::uint32_t seed) noexcept {
  double sum = 0.0;
  double amplitude = 0.5;
  double frequency = 1.0;
  for (int octave = 0; octave < octaves; ++octave) {
    sum += amplitude * ValueNoise(x * frequency, y * frequency, z * frequency,
                                  seed + static_cast<std::uint32_t>(octave));
    amplitude *= 0.5;
    frequency *= 2.0;
  }
  return sum;
}

template <typename Job>
void ParallelFor(int count, unsigned threads, Job const& job) {
  if (threads == 0u) {
    threads = std::max(1u, std::thread::hardware_concurrency());
  }
  std::vector<std::thread> pool;
  pool.reserve(threads);
  for (unsigned t = 0u; t < threads; ++t) {
    pool.emplace_back([&, t] {
      for (int i = static_cast<int>(t); i < count; i += static_cast<int>(threads)) {
        job(i);
      }
    });
  }
  for (auto& worker : pool) {
    worker.join();
  }
}
}  // namespace

std::string_view ToString(SceneKind scene) noexcept {
  switch (scene) {
    case SceneKind::kSphere:
      return "sphere";
    case SceneKind::kTerrain:
      return "terrain";
    case SceneKind::kModel:
      return "model";
  }
  return "unknown";
}

// Столбец (x, y) занят на отрезке [a, b]; воксель граничный, если z = a, z = b
// или z не попадает в отрезок хотя бы одного из четырёх соседних столбцов.
std::vector<std::uint64_t> GenerateSphereShell(std::uint32_t resolution, double radius) {
  int const n = static_cast<int>(resolution);
  double const center = resolution / 2.0;
  auto const interval = [&](int x, int y, int& a, int& b) {
    a = 1;
    b = 0;
    if (x < 0 || y < 0 || x >= n || y >= n) {
      return;
    }
    double const dx = x + 0.5 - center;
    double const dy = y + 0.5 - center;
    double const q = radius * radius - dx * dx - dy * dy;
    if (q < 0.0) {
      return;
    }
    double const h = std::sqrt(q);
    a = std::max(0, static_cast<int>(std::ceil(center - h - 0.5)));
    b = std::min(n - 1, static_cast<int>(std::floor(center + h - 0.5)));
  };

  std::vector<std::uint64_t> codes;
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      int a = 0;
      int b = 0;
      interval(x, y, a, b);
      if (a > b) {
        continue;
      }
      int lo = a;
      int hi = b;
      int const nx[4] = {x - 1, x + 1, x, x};
      int const ny[4] = {y, y, y - 1, y + 1};
      for (int k = 0; k < 4; ++k) {
        int a2 = 0;
        int b2 = 0;
        interval(nx[k], ny[k], a2, b2);
        if (a2 > b2) {
          lo = 1;
          hi = 0;
          break;
        }
        lo = std::max(lo, a2);
        hi = std::min(hi, b2);
      }
      for (int z = a; z <= b; ++z) {
        if (z == a || z == b || lo > hi || z < lo || z > hi) {
          codes.push_back(EncodeMorton(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y),
                                       static_cast<std::uint32_t>(z)));
        }
      }
    }
  }
  std::sort(codes.begin(), codes.end());
  return codes;
}

std::vector<std::uint64_t> GenerateTerrainShell(std::uint32_t resolution, TerrainParams const& params,
                                                unsigned threads) {
  int const n = static_cast<int>(resolution);
  std::size_t const side = resolution;
  std::vector<double> height(side * side);
  ParallelFor(n, threads, [&](int z) {
    for (int x = 0; x < n; ++x) {
      height[static_cast<std::size_t>(x) + side * static_cast<std::size_t>(z)] =
          resolution * (params.base_height +
                        params.amplitude * FractalNoise(x * params.frequency / resolution, 0.0,
                                                        z * params.frequency / resolution, 5, params.seed));
    }
  });

  double const cave_depth = params.cave_depth * resolution;
  double const cave_scale = params.cave_frequency / resolution;
  auto const solid = [&](int x, int y, int z) -> bool {
    if (y < 0) {
      return true;  // под сеткой — сплошное дно
    }
    if (x < 0 || z < 0 || x >= n || z >= n || y >= n) {
      return false;
    }
    double const h = height[static_cast<std::size_t>(x) + side * static_cast<std::size_t>(z)];
    if (y >= h) {
      return false;
    }
    if (y < h - cave_depth) {
      return true;
    }
    double const n1 = FractalNoise(x * cave_scale, y * cave_scale, z * cave_scale, 3, params.seed + 100u);
    double const n2 = FractalNoise(x * cave_scale, y * cave_scale, z * cave_scale, 3, params.seed + 200u);
    return !(std::fabs(n1) < params.cave_width && std::fabs(n2) < params.cave_width);
  };

  std::vector<std::vector<std::uint64_t>> per_slice(side);
  ParallelFor(n, threads, [&](int z) {
    auto& out = per_slice[static_cast<std::size_t>(z)];
    for (int x = 0; x < n; ++x) {
      double const h = height[static_cast<std::size_t>(x) + side * static_cast<std::size_t>(z)];
      double lowest = h;
      int const nx[4] = {x - 1, x + 1, x, x};
      int const nz[4] = {z, z, z - 1, z + 1};
      for (int k = 0; k < 4; ++k) {
        bool const outside = nx[k] < 0 || nz[k] < 0 || nx[k] >= n || nz[k] >= n;
        lowest = std::min(lowest, outside ? 0.0
                                          : height[static_cast<std::size_t>(nx[k]) +
                                                   side * static_cast<std::size_t>(nz[k])]);
      }
      int const y_begin = std::max(0, static_cast<int>(std::floor(lowest - cave_depth)) - 1);
      int const y_end = std::min(n - 1, static_cast<int>(std::ceil(h)));
      for (int y = y_begin; y <= y_end; ++y) {
        if (!solid(x, y, z)) {
          continue;
        }
        if (!solid(x - 1, y, z) || !solid(x + 1, y, z) || !solid(x, y - 1, z) ||
            !solid(x, y + 1, z) || !solid(x, y, z - 1) || !solid(x, y, z + 1)) {
          out.push_back(EncodeMorton(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y),
                                     static_cast<std::uint32_t>(z)));
        }
      }
    }
  });

  std::size_t total = 0u;
  for (auto const& slice : per_slice) {
    total += slice.size();
  }
  std::vector<std::uint64_t> codes;
  codes.reserve(total);
  for (auto const& slice : per_slice) {
    codes.insert(codes.end(), slice.begin(), slice.end());
  }
  std::sort(codes.begin(), codes.end());
  return codes;
}

// Формат binvox: текстовый заголовок ("#binvox 1", "dim d h w", "translate", "scale", "data"),
// затем пары байтов (значение, длина серии). Порядок вокселей: быстрее всего меняется y,
// затем z, затем x: index = x * (w * h) + z * w + y.
VoxelModel LoadBinvox(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  if (!file) {
    throw std::runtime_error{"Cannot open binvox file: " + path.string()};
  }
  std::string line;
  std::getline(file, line);
  if (line.rfind("#binvox", 0) != 0) {
    throw std::runtime_error{"Not a binvox file: " + path.string()};
  }
  std::uint32_t dim_x = 0u;
  std::uint32_t dim_y = 0u;
  std::uint32_t dim_z = 0u;
  while (std::getline(file, line)) {
    if (line.rfind("dim", 0) == 0) {
      std::istringstream{line.substr(3)} >> dim_x >> dim_y >> dim_z;
    } else if (line.rfind("data", 0) == 0) {
      break;
    }
  }
  if (dim_x == 0u || dim_y == 0u || dim_z == 0u) {
    throw std::runtime_error{"binvox header has no valid dim line"};
  }

  std::size_t const total = static_cast<std::size_t>(dim_x) * dim_y * dim_z;
  std::vector<bool> filled(total, false);
  std::size_t index = 0u;
  while (index < total) {
    int const value = file.get();
    int const count = file.get();
    if (value == EOF || count == EOF) {
      throw std::runtime_error{"binvox data is truncated"};
    }
    std::size_t const end = std::min(total, index + static_cast<std::size_t>(count));
    if (value != 0) {
      std::fill(filled.begin() + static_cast<std::ptrdiff_t>(index),
                filled.begin() + static_cast<std::ptrdiff_t>(end), true);
    }
    index = end;
  }

  auto const occupied = [&](long long x, long long y, long long z) {
    if (x < 0 || y < 0 || z < 0 || x >= dim_x || y >= dim_y || z >= dim_z) {
      return false;
    }
    return static_cast<bool>(filled[static_cast<std::size_t>(x) * dim_z * dim_y +
                                    static_cast<std::size_t>(z) * dim_y + static_cast<std::size_t>(y)]);
  };

  VoxelModel model;
  model.resolution = std::bit_ceil(std::max({dim_x, dim_y, dim_z, 2u}));
  for (long long x = 0; x < dim_x; ++x) {
    for (long long z = 0; z < dim_z; ++z) {
      for (long long y = 0; y < dim_y; ++y) {
        if (occupied(x, y, z) &&
            (!occupied(x - 1, y, z) || !occupied(x + 1, y, z) || !occupied(x, y - 1, z) ||
             !occupied(x, y + 1, z) || !occupied(x, y, z - 1) || !occupied(x, y, z + 1))) {
          model.shell_codes.push_back(EncodeMorton(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y),
                                                   static_cast<std::uint32_t>(z)));
        }
      }
    }
  }
  std::sort(model.shell_codes.begin(), model.shell_codes.end());
  return model;
}

void SaveBinvox(std::filesystem::path const& path, std::uint32_t resolution,
                std::span<std::uint64_t const> sorted_codes) {
  std::size_t const side = resolution;
  std::vector<bool> filled(side * side * side, false);
  for (std::uint64_t const code : sorted_codes) {
    VoxelCoord const c = DecodeMorton(code);
    filled[c.x * side * side + c.z * side + c.y] = true;
  }
  std::ofstream file{path, std::ios::binary};
  if (!file) {
    throw std::runtime_error{"Cannot create binvox file: " + path.string()};
  }
  file << "#binvox 1\ndim " << resolution << ' ' << resolution << ' ' << resolution
       << "\ntranslate 0 0 0\nscale 1\ndata\n";
  std::size_t i = 0u;
  while (i < filled.size()) {
    bool const value = filled[i];
    std::size_t run = 0u;
    while (i < filled.size() && filled[i] == value && run < 255u) {
      ++i;
      ++run;
    }
    file.put(static_cast<char>(value ? 1 : 0));
    file.put(static_cast<char>(static_cast<unsigned char>(run)));
  }
}
}  // namespace gpu_renderer::voxel
