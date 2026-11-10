#include "VoxelGrid.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
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

// Уровень, с которого начинается спуск, и запоминание текущей ячейки как предыдущей.
// Узел уровня l покрывает куб со стороной 2^(D-l); ячейки лежат в одном узле уровня l,
// если их координаты совпадают в битах с номерами >= D - l. Поэтому общий предок —
// уровень D - bit_width(diff), где diff — объединение отличающихся битов по трём осям.
[[nodiscard]] std::uint32_t StartLevel(std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint32_t depth,
                                       TraversalState& state) noexcept {
  std::uint32_t level = 0u;
  if (state.traversal == Traversal::kStack) {
    std::uint32_t const diff = (x ^ state.prev_x) | (y ^ state.prev_y) | (z ^ state.prev_z);
    std::uint32_t const shared = diff == 0u ? state.valid_level : depth - static_cast<std::uint32_t>(std::bit_width(diff));
    level = std::min(state.valid_level, shared);
  }
  state.prev_x = x;
  state.prev_y = y;
  state.prev_z = z;
  return level;
}

// Хеш-таблица уникальных узлов одного уровня SVDAG: содержимое узлов лежит подряд в flat,
// таблица хранит номер узла + 1 (0 — пустая ячейка). Открытая адресация, линейное пробирование.
class UniqueNodeTable {
 public:
  explicit UniqueNodeTable(std::size_t expected_nodes) {
    std::size_t capacity = 16u;
    while (capacity < 2u * expected_nodes) {
      capacity <<= 1u;
    }
    slots_.assign(capacity, 0u);
    offsets_.reserve(expected_nodes + 1u);
    offsets_.push_back(0u);
  }

  // Номер узла с таким содержимым; новый узел добавляется.
  [[nodiscard]] std::uint32_t Insert(std::span<std::uint32_t const> content) {
    std::size_t const mask = slots_.size() - 1u;
    for (std::size_t slot = Hash(content) & mask;; slot = (slot + 1u) & mask) {
      std::uint32_t const entry = slots_[slot];
      if (entry == 0u) {
        std::uint32_t const id = static_cast<std::uint32_t>(Count());
        flat_.insert(flat_.end(), content.begin(), content.end());
        offsets_.push_back(flat_.size());
        slots_[slot] = id + 1u;
        return id;
      }
      std::span<std::uint32_t const> const existing = Node(entry - 1u);
      if (existing.size() == content.size() &&
          std::memcmp(existing.data(), content.data(), content.size() * sizeof(std::uint32_t)) == 0) {
        return entry - 1u;
      }
    }
  }

  [[nodiscard]] std::size_t Count() const noexcept { return offsets_.size() - 1u; }
  [[nodiscard]] std::size_t Words() const noexcept { return flat_.size(); }
  [[nodiscard]] std::span<std::uint32_t const> Node(std::size_t id) const noexcept {
    return std::span<std::uint32_t const>{flat_}.subspan(offsets_[id], offsets_[id + 1u] - offsets_[id]);
  }

  void ReleaseTable() noexcept { std::vector<std::uint32_t>{}.swap(slots_); }

 private:
  [[nodiscard]] static std::size_t Hash(std::span<std::uint32_t const> content) noexcept {
    std::uint64_t h = 0x9e3779b97f4a7c15ULL ^ content.size();
    for (std::uint32_t const word : content) {
      h ^= word;
      h *= 0xbf58476d1ce4e5b9ULL;
      h ^= h >> 31u;
    }
    return static_cast<std::size_t>(h ^ (h >> 29u));
  }

  std::vector<std::uint32_t> flat_;
  std::vector<std::size_t> offsets_;
  std::vector<std::uint32_t> slots_;
};
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
    case Representation::kDenseBrick:
      return "dense-brick";
  }
  return "unknown";
}

std::string_view ToString(Traversal traversal) noexcept {
  return traversal == Traversal::kStack ? "stack" : "restart";
}

std::size_t DenseMemoryBytes(std::uint32_t resolution) noexcept {
  std::size_t const words = (static_cast<std::size_t>(resolution) * resolution * resolution + 31u) / 32u;
  return (words + 3u) / 4u * 4u * sizeof(std::uint32_t);
}

std::uint32_t IVoxelGrid::EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                          TraversalState& state) const {
  ++state.fetches;  // плотная сетка: одно слово на шаг
  return EmptyCell(x, y, z);
}

std::uint32_t Log2Ceil(std::uint32_t value) noexcept {
  std::uint32_t depth = 0u;
  while ((1ull << depth) < value) {
    ++depth;
  }
  return depth;
}

// ───────────── DenseGrid ─────────────
DenseGrid::DenseGrid(std::uint32_t resolution, std::span<std::uint64_t const> sorted_codes, Layout layout)
    : resolution_{resolution}, layout_{layout}, bits_(DenseMemoryBytes(resolution) / sizeof(std::uint32_t), 0u) {
  RequirePowerOfTwo(resolution);
  if (layout_ == Layout::kBrick && resolution < kBrickSide) {
    throw std::invalid_argument{"Brick layout requires N >= 8"};
  }
  for (std::uint64_t const code : sorted_codes) {
    VoxelCoord const c = DecodeMorton(code);
    Set(c.x, c.y, c.z, true);
  }
}

Representation DenseGrid::GetRepresentation() const noexcept {
  return layout_ == Layout::kBrick ? Representation::kDenseBrick : Representation::kDense;
}
std::uint32_t DenseGrid::Resolution() const noexcept { return resolution_; }

std::size_t DenseGrid::Index(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept {
  std::size_t const n = resolution_;
  if (layout_ == Layout::kLinear) {
    return x + n * (y + n * z);
  }
  constexpr std::uint32_t kLog = 3u;
  constexpr std::uint32_t kMask = kBrickSide - 1u;
  std::size_t const bricks = n >> kLog;
  std::size_t const brick = (x >> kLog) + bricks * ((y >> kLog) + bricks * (z >> kLog));
  std::size_t const local = (x & kMask) + kBrickSide * ((y & kMask) + kBrickSide * (z & kMask));
  return brick * (kBrickSide * kBrickSide * kBrickSide) + local;
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
std::uint32_t DenseGrid::GpuElementWords() const noexcept { return 4u; }

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

std::uint32_t BlockGrid::EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                         TraversalState& state) const {
  bool const empty_block = data_[DirectoryIndex(x, y, z)] == 0u;
  state.fetches += empty_block ? 1u : 2u;  // каталог, затем слово блока
  return empty_block ? kBlockSide : (Get(x, y, z) ? 0u : 1u);
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

std::uint32_t SparseVoxelOctree::EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                                 TraversalState& state) const {
  if (nodes_.empty()) {
    ++state.fetches;
    return resolution_;
  }
  bool const use_stack = state.traversal == Traversal::kStack;
  std::uint32_t level = StartLevel(x, y, z, depth_, state);
  std::uint32_t node = use_stack ? state.stack[level] : 0u;
  for (; level < depth_; ++level) {
    if (use_stack) {
      state.stack[level] = node;
      state.valid_level = level;
    }
    std::uint32_t const octant = Octant(x, y, z, depth_ - 1u - level);
    std::uint32_t const mask = nodes_[2u * node];
    ++state.fetches;
    if (((mask >> octant) & 1u) == 0u) {
      return resolution_ >> (level + 1u);
    }
    if (level + 1u == depth_) {
      return 0u;
    }
    node = nodes_[2u * node + 1u] + ChildRank(mask, octant);
    ++state.fetches;
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

  // Проход 1 (снизу вверх): уникальные узлы каждого уровня; дети — локальные номера уровня ниже.
  std::vector<UniqueNodeTable> unique;
  unique.reserve(depth_);
  for (std::uint32_t i = 0u; i < depth_; ++i) {
    unique.emplace_back(0u);
  }
  std::vector<std::uint32_t> child_ids;
  std::vector<std::uint32_t> current_ids;
  std::vector<std::uint32_t> content;
  content.reserve(9u);
  for (std::uint32_t level = depth_; level-- > 0u;) {
    UniqueNodeTable table{keys[level].size()};
    current_ids.assign(keys[level].size(), 0u);
    std::size_t child = 0u;
    for (std::size_t i = 0u; i < keys[level].size(); ++i) {
      content.assign(1u, masks[level][i]);
      if (level + 1u < depth_) {
        for (int k = std::popcount(masks[level][i]); k > 0; --k) {
          content.push_back(child_ids[child++]);
        }
      }
      current_ids[i] = table.Insert(content);
    }
    table.ReleaseTable();
    child_ids.swap(current_ids);
    unique_per_level_[level] = table.Count();
    unique[level] = std::move(table);
  }

  // Проход 2: раскладка уровней 0..D-1, локальные номера детей заменяются смещениями.
  std::vector<std::vector<std::uint32_t>> position(depth_);
  std::size_t total = 0u;
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    position[level].resize(unique[level].Count());
    for (std::size_t i = 0u; i < unique[level].Count(); ++i) {
      position[level][i] = static_cast<std::uint32_t>(total);
      total += unique[level].Node(i).size();
    }
  }
  if (total > 0xffffffffu) {
    throw std::length_error{"SVDAG does not fit 32-bit pointers"};
  }
  data_.reserve(total);
  for (std::uint32_t level = 0u; level < depth_; ++level) {
    for (std::size_t i = 0u; i < unique[level].Count(); ++i) {
      std::span<std::uint32_t const> const node = unique[level].Node(i);
      data_.push_back(node[0]);
      for (std::size_t c = 1u; c < node.size(); ++c) {
        data_.push_back(position[level + 1u][node[c]]);
      }
    }
    position[level].clear();
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

std::uint32_t SparseVoxelDag::EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                              TraversalState& state) const {
  if (data_.empty()) {
    ++state.fetches;
    return resolution_;
  }
  bool const use_stack = state.traversal == Traversal::kStack;
  std::uint32_t level = StartLevel(x, y, z, depth_, state);
  std::uint32_t pointer = use_stack ? state.stack[level] : 0u;  // корень — первый узел уровня 0
  for (; level < depth_; ++level) {
    if (use_stack) {
      state.stack[level] = pointer;
      state.valid_level = level;
    }
    std::uint32_t const octant = Octant(x, y, z, depth_ - 1u - level);
    std::uint32_t const mask = data_[pointer];
    ++state.fetches;
    if (((mask >> octant) & 1u) == 0u) {
      return resolution_ >> (level + 1u);
    }
    if (level + 1u == depth_) {
      return 0u;
    }
    pointer = data_[pointer + 1u + ChildRank(mask, octant)];
    ++state.fetches;
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
    case Representation::kDenseBrick:
      return std::make_unique<DenseGrid>(resolution, sorted_codes, DenseGrid::Layout::kBrick);
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
