#ifndef VOXEL_GRID_HPP
#define VOXEL_GRID_HPP

#include <array>
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
  kSvdag = 3u,
  kDenseBrick = 4u  // плотная сетка с раскладкой кирпичами 8^3 (проверка гипотезы п. 4.3)
};

[[nodiscard]] std::string_view ToString(Representation representation) noexcept;

// Плотные представления обходятся 3D-DDA, остальные — иерархически (EmptyCell).
[[nodiscard]] constexpr bool IsDense(Representation representation) noexcept {
  return representation == Representation::kDense || representation == Representation::kDenseBrick;
}

// Плотная сетка строится на CPU только до этого N: при N = 4096 маска занимает 8 ГиБ.
// Для больших N объём плотной сетки записывается в отчёт по формуле N^3 / 8.
inline constexpr std::uint32_t kMaxDenseResolution = 2048u;
[[nodiscard]] std::size_t DenseMemoryBytes(std::uint32_t resolution) noexcept;

// Способ спуска по иерархии на каждом шаге обхода. Значения совпадают с gTraversal в шейдере.
//   kRestart — от корня на каждом шаге: O((k + log N) log N) на луч;
//   kStack   — путь хранится в стеке, спуск начинается с общего предка предыдущей и текущей
//              ячеек (уровень определяется старшим отличающимся битом координат) [Laine, Karras 2010].
// На число шагов способ не влияет, меняется только число чтений узлов.
enum class Traversal : std::uint32_t { kRestart = 0u, kStack = 1u };

[[nodiscard]] std::string_view ToString(Traversal traversal) noexcept;

// Состояние обхода одного луча для EmptyCellTraced.
struct TraversalState {
  static constexpr std::uint32_t kMaxDepth = 21u;  // как у кода Мортона

  Traversal traversal = Traversal::kStack;
  std::array<std::uint32_t, kMaxDepth> stack{};  // узел уровня l, содержащий предыдущую ячейку
  std::uint32_t valid_level = 0u;                 // самый глубокий уровень, записанный в stack
  std::uint32_t prev_x = 0u;
  std::uint32_t prev_y = 0u;
  std::uint32_t prev_z = 0u;
  std::uint64_t fetches = 0u;  // чтений 32-битных слов буфера — столько же, сколько в шейдере
};

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

  // То же с учётом состояния обхода луча: спуск со стеком и подсчёт чтений буфера.
  // Первый вызов для луча — с состоянием по умолчанию (stack[0] — корень, valid_level = 0).
  [[nodiscard]] virtual std::uint32_t EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                                      TraversalState& state) const;

  // Точный объём данных, передаваемых на GPU, в байтах.
  [[nodiscard]] virtual std::size_t MemoryBytes() const noexcept = 0;

  // Содержимое структурированного буфера для пиксельного шейдера.
  [[nodiscard]] virtual std::span<std::uint32_t const> GpuData() const noexcept = 0;

  // Размер элемента буфера в 32-битных словах: 1 — StructuredBuffer<uint> (t0),
  // 4 — StructuredBuffer<uint4> (t1, плотные сетки: при N = 2048 слов 2^28, а элементов
  // в представлении буфера D3D11 допускается не больше 2^27).
  [[nodiscard]] virtual std::uint32_t GpuElementWords() const noexcept { return 1u; }
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

// Плотная сетка: 1 бит на воксель, M = N^3 / 8.
//   kLinear — индекс i = x + N(y + Nz): 32-байтный сектор содержит 256 вокселей вдоль x;
//   kBrick  — кирпичи 8^3 (64 байта) в порядке x, y, z, внутри кирпича x + 8(y + 8z):
//             сектор содержит блок 8 x 8 x 4, т. е. соседние лучи любого ракурса делят сектора.
class DenseGrid final : public IVoxelGrid, public IEditableVoxelGrid {
 public:
  enum class Layout { kLinear, kBrick };
  static constexpr std::uint32_t kBrickSide = 8u;

  DenseGrid(std::uint32_t resolution, std::span<std::uint64_t const> sorted_codes, Layout layout = Layout::kLinear);

  [[nodiscard]] Representation GetRepresentation() const noexcept override;
  [[nodiscard]] std::uint32_t Resolution() const noexcept override;
  [[nodiscard]] bool Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const override;
  [[nodiscard]] std::uint32_t EmptyCell(std::uint32_t x, std::uint32_t y,
                                        std::uint32_t z) const override;
  [[nodiscard]] std::size_t MemoryBytes() const noexcept override;
  [[nodiscard]] std::span<std::uint32_t const> GpuData() const noexcept override;
  [[nodiscard]] std::uint32_t GpuElementWords() const noexcept override;
  void Set(std::uint32_t x, std::uint32_t y, std::uint32_t z, bool value) override;

 private:
  [[nodiscard]] std::size_t Index(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept;

  std::uint32_t resolution_;
  Layout layout_;
  std::vector<std::uint32_t> bits_;  // дополнен нулями до кратного 4 словам (элемент uint4)
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
  [[nodiscard]] std::uint32_t EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                              TraversalState& state) const override;
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
  [[nodiscard]] std::uint32_t EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                              TraversalState& state) const override;
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
// Одинаковые узлы ищутся хеш-таблицей с открытой адресацией: O(n) в среднем, без выделения
// памяти на каждый узел (при N = 4096 на предпоследних уровнях — миллионы узлов).
// Узел = [маска][указатель на каждого непустого ребёнка], 4(1+k) байт; узел уровня D-1 = [маска].
class SparseVoxelDag final : public IVoxelGrid {
 public:
  explicit SparseVoxelDag(SparseVoxelOctree const& octree);

  [[nodiscard]] Representation GetRepresentation() const noexcept override;
  [[nodiscard]] std::uint32_t Resolution() const noexcept override;
  [[nodiscard]] bool Get(std::uint32_t x, std::uint32_t y, std::uint32_t z) const override;
  [[nodiscard]] std::uint32_t EmptyCell(std::uint32_t x, std::uint32_t y,
                                        std::uint32_t z) const override;
  [[nodiscard]] std::uint32_t EmptyCellTraced(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                              TraversalState& state) const override;
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
