#ifndef VOXEL_SCENE_VIEWER_HPP
#define VOXEL_SCENE_VIEWER_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../FrameTimer.hpp"
#include "../Graphics.hpp"
#include "SceneGenerators.hpp"
#include "VoxelGrid.hpp"
#include "VoxelVolume.hpp"

namespace gpu_renderer::voxel {
// Интерактивный просмотр сцен после эксперимента (или вместо него, --voxel-view-only).
// Управление:
//   Влево / Вправо — предыдущая / следующая сцена;
//   Вверх / Вниз   — следующее / предыдущее представление;
//   H              — тепловая карта числа шагов на луч;
//   T              — спуск по SVO/SVDAG: со стеком или от корня (без перестройки структуры);
//   Пробел         — пауза вращения камеры.
// Сцена, которой нет в кэше, генерируется (или читается) при переключении;
// структура строится заново. И то и другое блокирует окно на время работы.
class SceneViewer final {
 public:
  // Сцены дольше 16 млн граничных вокселей (128 МБ кодов) в кэше не хранятся.
  static constexpr std::size_t kMaxCachedVoxels = 16'000'000u;

  SceneViewer(std::vector<SceneCase> cases, std::vector<std::optional<LoadedScene>> cache,
              std::vector<Representation> representations, std::uint32_t width, std::uint32_t height,
              std::size_t start_case = 0u, std::size_t start_representation = 0u,
              Traversal traversal = Traversal::kStack);

  void OnKeyPressed(std::uint8_t key) noexcept;
  void RenderFrame(Graphics& gfx, FrameTimer::DeltaTime dt);

  // Текст для заголовка окна: текущая сцена, представление, память, подсказка по клавишам.
  [[nodiscard]] std::wstring const& Status() const noexcept;

 private:
  void Rebuild(Graphics& gfx);
  void SetStatus(std::wstring message);
  [[nodiscard]] std::wstring Describe() const;

  std::vector<SceneCase> cases_;
  std::vector<std::optional<LoadedScene>> cache_;
  std::vector<std::uint32_t> case_resolutions_;
  std::vector<Representation> representations_;
  std::uint32_t width_;
  std::uint32_t height_;

  std::size_t case_index_;
  std::size_t representation_index_;
  // Перестройка откладывается на кадр, чтобы заголовок «Подготовка…» успел отобразиться.
  enum class RebuildState { kNone, kAnnounced, kPending };
  RebuildState rebuild_state_ = RebuildState::kAnnounced;

  std::uint32_t resolution_ = 0u;
  std::unique_ptr<IVoxelGrid> grid_;
  std::unique_ptr<drawable::VoxelVolume> volume_;

  Traversal traversal_ = Traversal::kStack;
  bool traversal_changed_ = false;
  bool show_steps_ = false;
  bool paused_ = false;
  float azimuth_ = 0.6f;
  std::wstring status_;
  std::wstring details_;  // память и число вокселей текущей структуры (для обновления заголовка)
};
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_SCENE_VIEWER_HPP
