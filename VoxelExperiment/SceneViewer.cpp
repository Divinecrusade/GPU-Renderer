#include "SceneViewer.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

#include "../Error.hpp"
#include "RayTraversal.hpp"

namespace gpu_renderer::voxel {
namespace {
constexpr std::uint8_t kKeySpace = 0x20u;  // VK_SPACE
constexpr std::uint8_t kKeyLeft = 0x25u;   // VK_LEFT
constexpr std::uint8_t kKeyUp = 0x26u;     // VK_UP
constexpr std::uint8_t kKeyRight = 0x27u;  // VK_RIGHT
constexpr std::uint8_t kKeyDown = 0x28u;   // VK_DOWN
constexpr std::uint8_t kKeyH = 0x48u;      // 'H'
constexpr std::uint8_t kKeyT = 0x54u;      // 'T'

constexpr float kAngularSpeed = 0.3f;  // рад/с
constexpr wchar_t kHelp[] = L"   [←→ сцена, ↑↓ представление, H шаги, T стек/от корня, Пробел пауза]";

[[nodiscard]] std::wstring Widen(std::string_view text) {
  std::wstring result;
  result.reserve(text.size());
  for (char const ch : text) {
    result.push_back(static_cast<unsigned char>(ch) < 128u ? static_cast<wchar_t>(ch) : L'?');
  }
  return result;
}

[[nodiscard]] std::size_t Step(std::size_t index, std::size_t count, bool forward) noexcept {
  return forward ? (index + 1u) % count : (index + count - 1u) % count;
}
}  // namespace

SceneViewer::SceneViewer(std::vector<SceneCase> cases, std::vector<std::optional<LoadedScene>> cache,
                         std::vector<Representation> representations, std::uint32_t width, std::uint32_t height,
                         std::size_t start_case, std::size_t start_representation, Traversal traversal)
    : cases_{std::move(cases)},
      cache_{std::move(cache)},
      representations_{std::move(representations)},
      width_{width},
      height_{height},
      case_index_{start_case},
      representation_index_{start_representation},
      traversal_{traversal} {
  if (cases_.empty() || representations_.empty()) {
    throw std::invalid_argument{"Scene viewer needs at least one scene and one representation"};
  }
  cache_.resize(cases_.size());
  case_resolutions_.reserve(cases_.size());
  for (std::size_t i = 0u; i < cases_.size(); ++i) {
    case_resolutions_.push_back(cache_[i] ? cache_[i]->resolution : cases_[i].resolution);
  }
  case_index_ %= cases_.size();
  representation_index_ %= representations_.size();
  SetStatus(L"Подготовка: " + Describe() + L"…");
}

std::wstring const& SceneViewer::Status() const noexcept { return status_; }

void SceneViewer::SetStatus(std::wstring message) { status_ = std::move(message) + kHelp; }

std::wstring SceneViewer::Describe() const {
  SceneCase const& scene = cases_[case_index_];
  std::wstring text = Widen(ToString(scene.kind));
  if (scene.kind == SceneKind::kModel) {
    text += L" " + scene.model_path.filename().wstring();
  }
  std::uint32_t const n = case_resolutions_[case_index_];  // для модели известно после загрузки
  if (n != 0u) {
    text += L" N=" + std::to_wstring(n);
  }
  Representation const representation = representations_[representation_index_];
  text += L" (" + std::to_wstring(case_index_ + 1u) + L"/" + std::to_wstring(cases_.size()) + L") | " +
          Widen(ToString(representation));
  if (representation == Representation::kSvo || representation == Representation::kSvdag) {
    text += L" (" + Widen(ToString(traversal_)) + L")";
  }
  return text;
}

void SceneViewer::OnKeyPressed(std::uint8_t key) noexcept {
  try {
    switch (key) {
      case kKeyLeft:
      case kKeyRight:
        case_index_ = Step(case_index_, cases_.size(), key == kKeyRight);
        break;
      case kKeyUp:
      case kKeyDown:
        representation_index_ = Step(representation_index_, representations_.size(), key == kKeyUp);
        break;
      case kKeyH:
        show_steps_ = !show_steps_;
        return;
      case kKeyT:
        traversal_ = traversal_ == Traversal::kStack ? Traversal::kRestart : Traversal::kStack;
        traversal_changed_ = true;
        return;
      case kKeySpace:
        paused_ = !paused_;
        return;
      default:
        return;
    }
    SetStatus(L"Подготовка: " + Describe() + L"…");
    rebuild_state_ = RebuildState::kAnnounced;
  } catch (...) {
    // нехватка памяти под строку статуса не должна ронять цикл сообщений
  }
}

void SceneViewer::Rebuild(Graphics& gfx) {
  volume_.reset();
  grid_.reset();
  details_.clear();

  std::optional<LoadedScene>& cached = cache_[case_index_];
  LoadedScene loaded_now;
  LoadedScene const* scene = cached ? &*cached : nullptr;
  if (scene == nullptr) {
    try {
      loaded_now = LoadSceneCase(cases_[case_index_]);
    } catch (std::exception const& e) {
      SetStatus(Describe() + L" | ошибка загрузки: " + Widen(e.what()));
      return;
    }
    if (loaded_now.shell_codes.size() <= kMaxCachedVoxels) {
      cached = std::move(loaded_now);
      scene = &*cached;
    } else {
      scene = &loaded_now;
    }
  }
  resolution_ = scene->resolution;
  case_resolutions_[case_index_] = resolution_;

  Representation const representation = representations_[representation_index_];
  if (IsDense(representation) && resolution_ > drawable::VoxelVolume::kMaxDenseResolution) {
    SetStatus(Describe() + L" | недоступно при N > " + std::to_wstring(drawable::VoxelVolume::kMaxDenseResolution));
    return;
  }
  try {
    grid_ = BuildGrid(representation, resolution_, scene->shell_codes);
    volume_ = std::make_unique<drawable::VoxelVolume>(gfx, *grid_, width_, height_);
    volume_->SetTraversal(gfx, traversal_);
  } catch (exception::Error const& e) {
    volume_.reset();
    SetStatus(Describe() + L" | ошибка GPU: " + std::wstring{e.GetTypeOfException()});
    return;
  } catch (std::exception const& e) {
    volume_.reset();
    SetStatus(Describe() + L" | ошибка: " + Widen(e.what()));
    return;
  }

  wchar_t memory[32];
  std::swprintf(memory, std::size(memory), L"%.2f МиБ", static_cast<double>(grid_->MemoryBytes()) / 1048576.0);
  details_ = L" | " + std::wstring{memory} + L" | " + std::to_wstring(scene->shell_codes.size()) + L" вокселей";
  SetStatus(Describe() + details_);
}

void SceneViewer::RenderFrame(Graphics& gfx, FrameTimer::DeltaTime dt) {
  if (rebuild_state_ == RebuildState::kPending) {
    Rebuild(gfx);
    rebuild_state_ = RebuildState::kNone;
  } else if (rebuild_state_ == RebuildState::kAnnounced) {
    rebuild_state_ = RebuildState::kPending;  // этот кадр — только показать «Подготовка…»
  }

  if (!paused_) {
    constexpr float kMaxStep = 0.1f;  // после долгой перестройки камера не «прыгает»
    azimuth_ += kAngularSpeed * std::min(dt, kMaxStep);
  }
  if (traversal_changed_) {
    traversal_changed_ = false;
    if (volume_) {
      volume_->SetTraversal(gfx, traversal_);
    }
    SetStatus(Describe() + details_);
  }
  gfx.ClearBuffer({0.f, 0.f, 0.f});
  if (volume_) {
    volume_->SetView(gfx,
                     MakeCameraBasis(MakeOrbitCameraAt(resolution_, azimuth_),
                                     static_cast<float>(width_) / static_cast<float>(height_)),
                     show_steps_);
    volume_->Draw(gfx);
  }
  gfx.EndFrame();
}
}  // namespace gpu_renderer::voxel
