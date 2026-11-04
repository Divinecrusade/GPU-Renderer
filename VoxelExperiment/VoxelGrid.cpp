#include "VoxelGrid.hpp"

#include <bit>
#include <cassert>
#include <map>
#include <stdexcept>

#include "Morton.hpp"

namespace gpu_renderer::voxel {
namespace {
[[nodiscard]] std::uint32_t Octant(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                   std::uint32_t shift) noexcept {
  return ((x >> shift) & 1u) | (((y >> shift) & 1u) << 1u) | (((z >> shift) & 1u) << 2u);
}

[[nodiscard]] std::uint32_t ChildRank(std::uint32_t mask, std::uint32_t octant) noexcept {
  return static_cast<std::uint32_t>(std::popcount(mask & ((1u << octant) - 1u)));
}

void RequirePowerOfTwo(std::uint32_t resolution) {
  if (resolution < 2u || !std::has_single_bit(resolution)) {
    throw std::invalid_argument{"Voxel grid resolution must be a power of two (>= 2)"};
  }
}
}  // namespace

std::string_view ToString(Representation representation) noexcept {
  switch (representation) {
    case Representation::kDense:
      return "dense";
    case Representation::kBlock:
      return "block";
    case Representation::kSvo:
      return "svo";
    case Representation::kSvdag:
      return "svdag";
  }
  return "unknown";
}

std::uint32_t Log2Ceil(std::uint32_t value) noexcept {
  std::uint32_t depth = 0u;
  while ((1ull << depth) < value) {
    ++depth;
  }
  return depth;
}

// ───────────── DenseGrid ─────────────
DenseGrid::DenseGrid(std::uint32_t resolution, std::span<std::uint64_t const> sorted_codes)
    : resolution_{resolution},
      bits_((static_cast<std::size_t>(resolution) * resolution * resolution + 31u) / 32u, 0u) {
  RequirePowerOfTwo(resolution);
  for (std::uint64_t const code : sorted_codes) {
    VoxelCoord const c = DecodeMorton(code);
    Set(c.x, c.y, c.z, true);
  }
}

Representation DenseGrid::GetRepresentation() const noexcept { return Representation::kDense; }
std::uint32_t DenseGrid::Resolution() const noexcept { return resolution_; }

std::size_t DenseGrid::Index(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept {
  return x + static_cast<std::size_t>(resolution_) * (y + static_cast<std::size_t>(resolution_) * z);
}

bool DenseGrid::Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  std::size_t const i = Index(x, y, z);
  return ((bits_[i >> 5u] >> (i & 31u)) & 1u) != 0u;
}

void DenseGrid::Set(std::uint32_t x, std::uint32_t y, std::uint32_t z, bool value) {
  std::size_t const i = Index(x, y, z);
  std::uint32_t const bit = 1u << (i & 31u);
  if (value) {
    bits_[i >> 5u] |= bit;
  } else {
    bits_[i >> 5u] &= ~bit;
  }
}

std::uint32_t DenseGrid::EmptyCell(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  return Get(x, y, z) ? 0u : 1u;
}

std::size_t DenseGrid::MemoryBytes() const noexcept { return bits_.size() * sizeof(std::uint32_t); }
std::span<std::uint32_t const> DenseGrid::GpuData() const noexcept { return bits_; }

// ───────────── BlockGrid ─────────────
BlockGrid::BlockGrid(std::uint32_t resolution, std::span<std::uint64_t const> sorted_codes)
    : resolution_{resolution},
      blocks_per_side_{resolution / kBlockSide},
      directory_size_{static_cast<std::size_t>(blocks_per_side_) * blocks_per_side_ * blocks_per_side_} {
  RequirePowerOfTwo(resolution);
  if (resolution < kBlockSide) {
    throw std::invalid_argument{"Block grid resolution must be at least the block side"};
  }
  data_.assign(directory_size_, 0u);
  for (std::uint64_t const code : sorted_codes) {
    VoxelCoord const c = DecodeMorton(code);
    Set(c.x, c.y, c.z, true);
  }
}

Representation BlockGrid::GetRepresentation() const noexcept { return Representation::kBlock; }
std::uint32_t BlockGrid::Resolution() const noexcept { return resolution_; }

std::size_t BlockGrid::DirectoryIndex(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept {
  return (x >> kLogBlockSide) +
         static_cast<std::size_t>(blocks_per_side_) *
             ((y >> kLogBlockSide) + static_cast<std::size_t>(blocks_per_side_) * (z >> kLogBlockSide));
}

std::uint32_t BlockGrid::LocalIndex(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
  constexpr std::uint32_t kMask = kBlockSide - 1u;
  return (x & kMask) + kBlockSide * ((y & kMask) + kBlockSide * (z & kMask));
}

bool BlockGrid::Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  std::uint32_t const block = data_[DirectoryIndex(x, y, z)];
  if (block == 0u) {
    return false;
  }
  std::uint32_t const local = LocalIndex(x, y, z);
  std::uint32_t const word =
      data_[directory_size_ + static_cast<std::size_t>(block - 1u) * kWordsPerBlock + (local >> 5u)];
  return ((word >> (local & 31u)) & 1u) != 0u;
}

void BlockGrid::Set(std::uint32_t x, std::uint32_t y, std::uint32_t z, bool value) {
  std::size_t const dir = DirectoryIndex(x, y, z);
  if (data_[dir] == 0u) {
    if (!value) {
      return;
    }
    data_[dir] = static_cast<std::uint32_t>(BlockCount() + 1u);
    data_.resize(data_.size() + kWordsPerBlock, 0u);
  }
  // Опустевший блок не освобождается: для эксперимента это допустимо.
  std::uint32_t const local = LocalIndex(x, y, z);
  std::uint32_t& word =
      data_[directory_size_ + static_cast<std::size_t>(data_[dir] - 1u) * kWordsPerBlock + (local >> 5u)];
  std::uint32_t const bit = 1u << (local & 31u);
  if (value) {
    word |= bit;
  } else {
    word &= ~bit;
  }
}

std::uint32_t BlockGrid::EmptyCell(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  if (data_[DirectoryIndex(x, y, z)] == 0u) {
    return kBlockSide;
  }
  return Get(x, y, z) ? 0u : 1u;
}

std::size_t BlockGrid::MemoryBytes() const noexcept { return data_.size() * sizeof(std::uint32_t); }
std::span<std::uint32_t const> BlockGrid::GpuData() const noexcept { return data_; }
std::size_t BlockGrid::BlockCount() const noexcept { return (data_.size() - directory_size_) / kWordsPerBlock; }
std::size_t BlockGrid::DirectorySize() const noexcept { return directory_size_; }

// ───────────── SparseVoxelOctree ─────────────
SparseVoxelOctree::SparseVoxelOctree(std::uint32_t resolution,
                                     std::span<std::uint64_t const> sorted_codes)
    : resolution_{resolution}, depth_{Log2Ceil(resolution)} {
  RequirePowerOfTwo(resolution);
  level_keys_.assign(depth_, {});
  level_masks_.assign(depth_, {});

  // Уровень D-1: группировка вокселей по code >> 3, маска — по младшим трём битам.
  auto& bottom_keys = level_keys_[depth_ - 1u];
  auto& bottom_masks = level_masks_[depth_ - 1u];
  for (std::uint64_t const code : sorted_codes) {
    std::uint64_t const key = code >> 3u;
    if (bottom_keys.empty() || bottom_keys.back() != key) {
      assert(((void)"Codes must be sorted", bottom_keys.empty() || bottom_keys.back() < key));
      bottom_keys.push_back(key);
      bottom_masks.push_back(0u);
    }
    bottom_masks.back() |= 1u << static_cast<std::uint32_t>(code & 7u);
  }

  // Уровни D-2..0: группировка ключей детей по key >> 3.
  for (std::uint32_t level = depth_ - 1u; level-- > 0u;) {
    for (std::uint64_t const child_key : level_keys_[level + 1u]) {
      std::uint64_t const key = child_key >> 3u;
      if (level_keys_[level].empty() || level_keys_[level].back() != key) {
        level_keys_[level].push_back(key);
        level_masks_[level].push_back(0u);
      }
      level_masks_[level].back() |= 1u << static_cast<std::uint32_t>(child_key & 7u);
    }
  }

  // Раскладка в ширину: дети узла лежат подряд, так как ключи уровня отсортированы.
  std::vector<std::size_t> offset(depth_ + 1u, 0u);
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    offset[level + 1u] = offset[level] + level_keys_[level].size();
  }
  nodes_.assign(offset[depth_] * 2u, 0u);
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    std::size_t child = 0u;
    for (std::size_t i = 0u; i < level_keys_[level].size(); ++i) {
      std::size_t const node = offset[level] + i;
      nodes_[2u * node] = level_masks_[level][i];
      if (level + 1u < depth_) {
        auto const& children = level_keys_[level + 1u];
        while (child < children.size() && (children[child] >> 3u) < level_keys_[level][i]) {
          ++child;
        }
        nodes_[2u * node + 1u] = static_cast<std::uint32_t>(offset[level + 1u] + child);
      }
    }
  }
}

Representation SparseVoxelOctree::GetRepresentation() const noexcept { return Representation::kSvo; }
std::uint32_t SparseVoxelOctree::Resolution() const noexcept { return resolution_; }

std::uint32_t SparseVoxelOctree::EmptyCell(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  if (nodes_.empty()) {
    return resolution_;
  }
  std::uint32_t node = 0u;
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    std::uint32_t const octant = Octant(x, y, z, depth_ - 1u - level);
    std::uint32_t const mask = nodes_[2u * node];
    if (((mask >> octant) & 1u) == 0u) {
      return resolution_ >> (level + 1u);
    }
    if (level + 1u == depth_) {
      return 0u;
    }
    node = nodes_[2u * node + 1u] + ChildRank(mask, octant);
  }
  return 0u;
}

bool SparseVoxelOctree::Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  return EmptyCell(x, y, z) == 0u;
}

std::size_t SparseVoxelOctree::MemoryBytes() const noexcept { return nodes_.size() * sizeof(std::uint32_t); }
std::span<std::uint32_t const> SparseVoxelOctree::GpuData() const noexcept { return nodes_; }
std::uint32_t SparseVoxelOctree::Depth() const noexcept { return depth_; }
std::size_t SparseVoxelOctree::NodeCount() const noexcept { return nodes_.size() / 2u; }

std::vector<std::vector<std::uint64_t>> const& SparseVoxelOctree::LevelKeys() const noexcept {
  return level_keys_;
}
std::vector<std::vector<std::uint32_t>> const& SparseVoxelOctree::LevelMasks() const noexcept {
  return level_masks_;
}

// ───────────── SparseVoxelDag ─────────────
SparseVoxelDag::SparseVoxelDag(SparseVoxelOctree const& octree)
    : resolution_{octree.Resolution()}, depth_{octree.Depth()}, unique_per_level_(depth_, 0u) {
  auto const& keys = octree.LevelKeys();
  auto const& masks = octree.LevelMasks();
  if (keys.empty() || keys.front().empty()) {
    return;
  }

  // Проход 1: уникальные узлы каждого уровня; дети — локальные индексы уровня ниже.
  using NodeContent = std::vector<std::uint32_t>;
  std::vector<std::vector<NodeContent>> unique(depth_);
  std::vector<std::uint32_t> child_ids;
  std::vector<std::uint32_t> current_ids;
  for (std::uint32_t level = depth_; level-- > 0u;) {
    std::map<NodeContent, std::uint32_t> table;
    current_ids.assign(keys[level].size(), 0u);
    std::size_t child = 0u;
    for (std::size_t i = 0u; i < keys[level].size(); ++i) {
      NodeContent content{masks[level][i]};
      if (level + 1u < depth_) {
        for (int k = std::popcount(masks[level][i]); k > 0; --k) {
          content.push_back(child_ids[child++]);
        }
      }
      auto [it, inserted] = table.try_emplace(content, static_cast<std::uint32_t>(unique[level].size()));
      if (inserted) {
        unique[level].push_back(std::move(content));
      }
      current_ids[i] = it->second;
    }
    child_ids.swap(current_ids);
    unique_per_level_[level] = unique[level].size();
  }

  // Проход 2: раскладка уровней 0..D-1, локальные индексы детей заменяются смещениями.
  std::vector<std::vector<std::uint32_t>> position(depth_);
  std::size_t total = 0u;
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    position[level].resize(unique[level].size());
    for (std::size_t i = 0u; i < unique[level].size(); ++i) {
      position[level][i] = static_cast<std::uint32_t>(total);
      total += unique[level][i].size();
    }
  }
  data_.reserve(total);
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    for (NodeContent const& node : unique[level]) {
      data_.push_back(node[0]);
      for (std::size_t c = 1u; c < node.size(); ++c) {
        data_.push_back(position[level + 1u][node[c]]);
      }
    }
  }
}

Representation SparseVoxelDag::GetRepresentation() const noexcept { return Representation::kSvdag; }
std::uint32_t SparseVoxelDag::Resolution() const noexcept { return resolution_; }

std::uint32_t SparseVoxelDag::EmptyCell(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  if (data_.empty()) {
    return resolution_;
  }
  std::uint32_t pointer = 0u;  // корень — первый узел уровня 0
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    std::uint32_t const octant = Octant(x, y, z, depth_ - 1u - level);
    std::uint32_t const mask = data_[pointer];
    if (((mask >> octant) & 1u) == 0u) {
      return resolution_ >> (level + 1u);
    }
    if (level + 1u == depth_) {
      return 0u;
    }
    pointer = data_[pointer + 1u + ChildRank(mask, octant)];
  }
  return 0u;
}

bool SparseVoxelDag::Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const {
  return EmptyCell(x, y, z) == 0u;
}

std::size_t SparseVoxelDag::MemoryBytes() const noexcept { return data_.size() * sizeof(std::uint32_t); }
std::span<std::uint32_t const> SparseVoxelDag::GpuData() const noexcept { return data_; }
std::vector<std::size_t> const& SparseVoxelDag::UniqueNodesPerLevel() const noexcept {
  return unique_per_level_;
}

std::unique_ptr<IVoxelGrid> BuildGrid(Representation representation, std::uint32_t resolution,
                                      std::span<std::uint64_t const> sorted_codes) {
  switch (representation) {
    case Representation::kDense:
      return std::make_unique<DenseGrid>(resolution, sorted_codes);
    case Representation::kBlock:
      return std::make_unique<BlockGrid>(resolution, sorted_codes);
    case Representation::kSvo:
      return std::make_unique<SparseVoxelOctree>(resolution, sorted_codes);
    case Representation::kSvdag: {
      SparseVoxelOctree const octree{resolution, sorted_codes};
      return std::make_unique<SparseVoxelDag>(octree);
    }
  }
  throw std::invalid_argument{"Unknown voxel representation"};
}
}  // namespace gpu_renderer::voxel
