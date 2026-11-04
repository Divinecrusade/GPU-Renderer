#ifndef VOXEL_GRID_HPP
#define VOXEL_GRID_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

// Воксельные представления для вычислительного эксперимента.
// Все строятся из одного входа — отсортированного списка Мортон-кодов граничных вокселей
// (оболочка по 6-соседству). Модуль не зависит от DirectX и собирается на любой платформе.
namespace gpu_renderer::voxel {
// Значения совпадают с режимами в VoxelTracePS.hlsl.
enum class Representation : std::uint32_t {
  kDense = 0u,
  kBlock = 1u,
  kSvo = 2u,
  kSvdag = 3u
};

[[nodiscard]] std::string_view ToString(Representation representation) noexcept;

class IVoxelGrid {
 public:
  IVoxelGrid() = default;
  IVoxelGrid(IVoxelGrid const&) = delete;
  IVoxelGrid(IVoxelGrid&&) = delete;
  IVoxelGrid& operator=(IVoxelGrid const&) = delete;
  IVoxelGrid& operator=(IVoxelGrid&&) = delete;
  virtual ~IVoxelGrid() = default;

  [[nodiscard]] virtual Representation GetRepresentation() const noexcept = 0;
  [[nodiscard]] virtual std::uint32_t Resolution() const noexcept = 0;

  // Точечный запрос f(p): занят ли воксель.
  [[nodiscard]] virtual bool Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const = 0;

  // Сторона наибольшего пустого выровненного куба, содержащего воксель (степень 2);
  // 0 — воксель занят. Единственная операция, нужная иерархическому обходу луча.
  [[nodiscard]] virtual std::uint32_t EmptyCell(std::uint32_t x, std::uint32_t y,
                                                std::uint32_t z) const = 0;

  // Точный объём данных, передаваемых на GPU, в байтах.
  [[nodiscard]] virtual std::size_t MemoryBytes() const noexcept = 0;

  // Содержимое StructuredBuffer<uint> для пиксельного шейдера.
  [[nodiscard]] virtual std::span<std::uint32_t const> GpuData() const noexcept = 0;
};

class IEditableVoxelGrid {
 public:
  IEditableVoxelGrid() = default;
  IEditableVoxelGrid(IEditableVoxelGrid const&) = delete;
  IEditableVoxelGrid(IEditableVoxelGrid&&) = delete;
  IEditableVoxelGrid& operator=(IEditableVoxelGrid const&) = delete;
  IEditableVoxelGrid& operator=(IEditableVoxelGrid&&) = delete;
  virtual ~IEditableVoxelGrid() = default;

  virtual void Set(std::uint32_t x, std::uint32_t y, std::uint32_t z, bool value) = 0;
};

// Плотная сетка: 1 бит на воксель, M = N^3 / 8. Индекс i = x + N(y + Nz).
class DenseGrid final : public IVoxelGrid, public IEditableVoxelGrid {
 public:
  DenseGrid(std::uint32_t resolution, std::span<std::uint64_t const> sorted_codes);

  [[nodiscard]] Representation GetRepresentation() const noexcept override;
  [[nodiscard]] std::uint32_t Resolution() const noexcept override;
  [[nodiscard]] bool Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const override;
  [[nodiscard]] std::uint32_t EmptyCell(std::uint32_t x, std::uint32_t y,
                                        std::uint32_t z) const override;
  [[nodiscard]] std::size_t MemoryBytes() const noexcept override;
  [[nodiscard]] std::span<std::uint32_t const> GpuData() const noexcept override;
  void Set(std::uint32_t x, std::uint32_t y, std::uint32_t z, bool value) override;

 private:
  [[nodiscard]] std::size_t Index(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept;

  std::uint32_t resolution_;
  std::vector<std::uint32_t> bits_;
};

// Блочная сетка: каталог (N/B)^3 слов (0 — пустой блок, иначе номер блока + 1)
// и пул блоков по B^3 бит. GPU-буфер: [каталог][пул].
class BlockGrid final : public IVoxelGrid, public IEditableVoxelGrid {
 public:
  static constexpr std::uint32_t kBlockSide = 16u;
  static constexpr std::uint32_t kLogBlockSide = 4u;
  static constexpr std::uint32_t kWordsPerBlock = kBlockSide * kBlockSide * kBlockSide / 32u;

  BlockGrid(std::uint32_t resolution, std::span<std::uint64_t const> sorted_codes);

  [[nodiscard]] Representation GetRepresentation() const noexcept override;
  [[nodiscard]] std::uint32_t Resolution() const noexcept override;
  [[nodiscard]] bool Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const override;
  [[nodiscard]] std::uint32_t EmptyCell(std::uint32_t x, std::uint32_t y,
                                        std::uint32_t z) const override;
  [[nodiscard]] std::size_t MemoryBytes() const noexcept override;
  [[nodiscard]] std::span<std::uint32_t const> GpuData() const noexcept override;
  void Set(std::uint32_t x, std::uint32_t y, std::uint32_t z, bool value) override;

  [[nodiscard]] std::size_t BlockCount() const noexcept;
  [[nodiscard]] std::size_t DirectorySize() const noexcept;

 private:
  [[nodiscard]] std::size_t DirectoryIndex(std::uint32_t x, std::uint32_t y,
                                           std::uint32_t z) const noexcept;
  [[nodiscard]] static std::uint32_t LocalIndex(std::uint32_t x, std::uint32_t y,
                                                std::uint32_t z) noexcept;

  std::uint32_t resolution_;
  std::uint32_t blocks_per_side_;
  std::size_t directory_size_;
  std::vector<std::uint32_t> data_;
};

// SVO: узел = 2 слова {маска детей, индекс первого ребёнка} (8 байт).
// Дети лежат подряд в порядке битов маски, уровни — в порядке обхода в ширину.
// Узлы уровня D-1 хранят маску самих вокселей (листья неявные).
// Структура неизменяемая: правка = перестройка за O(n log n).
class SparseVoxelOctree final : public IVoxelGrid {
 public:
  SparseVoxelOctree(std::uint32_t resolution, std::span<std::uint64_t const> sorted_codes);

  [[nodiscard]] Representation GetRepresentation() const noexcept override;
  [[nodiscard]] std::uint32_t Resolution() const noexcept override;
  [[nodiscard]] bool Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const override;
  [[nodiscard]] std::uint32_t EmptyCell(std::uint32_t x, std::uint32_t y,
                                        std::uint32_t z) const override;
  [[nodiscard]] std::size_t MemoryBytes() const noexcept override;
  [[nodiscard]] std::span<std::uint32_t const> GpuData() const noexcept override;

  [[nodiscard]] std::uint32_t Depth() const noexcept;
  [[nodiscard]] std::size_t NodeCount() const noexcept;

  // Ключи и маски узлов по уровням: вход для построения SparseVoxelDag.
  [[nodiscard]] std::vector<std::vector<std::uint64_t>> const& LevelKeys() const noexcept;
  [[nodiscard]] std::vector<std::vector<std::uint32_t>> const& LevelMasks() const noexcept;

 private:
  std::uint32_t resolution_;
  std::uint32_t depth_;
  std::vector<std::vector<std::uint64_t>> level_keys_;
  std::vector<std::vector<std::uint32_t>> level_masks_;
  std::vector<std::uint32_t> nodes_;
};

// SVDAG: слияние одинаковых поддеревьев снизу вверх [Kämpe et al., 2013].
// Узел = [маска][указатель на каждого непустого ребёнка], 4(1+k) байт; узел уровня D-1 = [маска].
class SparseVoxelDag final : public IVoxelGrid {
 public:
  explicit SparseVoxelDag(SparseVoxelOctree const& octree);

  [[nodiscard]] Representation GetRepresentation() const noexcept override;
  [[nodiscard]] std::uint32_t Resolution() const noexcept override;
  [[nodiscard]] bool Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const override;
  [[nodiscard]] std::uint32_t EmptyCell(std::uint32_t x, std::uint32_t y,
                                        std::uint32_t z) const override;
  [[nodiscard]] std::size_t MemoryBytes() const noexcept override;
  [[nodiscard]] std::span<std::uint32_t const> GpuData() const noexcept override;

  // Число различных узлов на каждом уровне (на уровне D-1 не больше 255).
  [[nodiscard]] std::vector<std::size_t> const& UniqueNodesPerLevel() const noexcept;

 private:
  std::uint32_t resolution_;
  std::uint32_t depth_;
  std::vector<std::size_t> unique_per_level_;
  std::vector<std::uint32_t> data_;
};

[[nodiscard]] std::unique_ptr<IVoxelGrid> BuildGrid(Representation representation,
                                                    std::uint32_t resolution,
                                                    std::span<std::uint64_t const> sorted_codes);

[[nodiscard]] std::uint32_t Log2Ceil(std::uint32_t value) noexcept;
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_GRID_HPP
