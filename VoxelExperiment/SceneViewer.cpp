#include "SceneViewer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <stdexcept>

#include "../Error.hpp"
#include "FrameCapture.hpp"
#include "RayTraversal.hpp"

namespace gpu_renderer::voxel {
namespace {
constexpr std::uint8_t kKeySpace = 0x20u;  // VK_SPACE
constexpr std::uint8_t kKeyLeft = 0x25u;   // VK_LEFT
constexpr std::uint8_t kKeyUp = 0x26u;     // VK_UP
constexpr std::uint8_t kKeyRight = 0x27u;  // VK_RIGHT
constexpr std::uint8_t kKeyDown = 0x28u;   // VK_DOWN
constexpr std::uint8_t kKeyH = 0x48u;      // 'H'
constexpr std::uint8_t kKeyP = 0x50u;      // 'P'
constexpr std::uint8_t kKey1 = 0x31u;      // '1'
constexpr std::uint8_t kKey3 = 0x33u;      // '3'
constexpr std::size_t kExperimentViews = 3u;  // как --voxel-views по умолчанию

constexpr float kAngularSpeed = 0.3f;  // рад/с
constexpr wchar_t kHelp[] = L"   [←→ сцена, ↑↓ представление, H шаги, Пробел пауза, 1–3 ракурс, P снимок]";

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
                         std::size_t start_case, std::size_t start_representation)
    : cases_{std::move(cases)},
      cache_{std::move(cache)},
      representations_{std::move(representations)},
      width_{width},
      height_{height},
      case_index_{start_case},
      representation_index_{start_representation} {
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
  text += L" (" + std::to_wstring(case_index_ + 1u) + L"/" + std::to_wstring(cases_.size()) + L") | " +
          Widen(ToString(representations_[representation_index_]));
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
      case kKeySpace:
        paused_ = !paused_;
        view_index_ = -1;
        return;
      case kKeyP:
        capture_requested_ = true;
        return;
      default:
        if (key >= kKey1 && key <= kKey3) {
          // тот же азимут, что у ракурса эксперимента (MakeOrbitCamera в RayTraversal.cpp)
          view_index_ = key - kKey1;
          azimuth_ = 0.6f + 2.f * std::numbers::pi_v<float> * static_cast<float>(view_index_) /
                                static_cast<float>(kExperimentViews);
          paused_ = true;
        }
        return;
    }
    SetStatus(L"Подготовка: " + Describe() + L"…");
    rebuild_state_ = RebuildState::kAnnounced;
  } catch (...) {
    // нехватка памяти под строку статуса не должна ронять цикл сообщений
  }
}

std::filesystem::path SceneViewer::CaptureFileName() const {
  SceneCase const& scene = cases_[case_index_];
  std::string name = "capture_" + std::string{ToString(scene.kind)};
  if (scene.kind == SceneKind::kModel) {
    name += "-" + scene.model_path.stem().string();
  }
  name += "_N" + std::to_string(resolution_) + "_" + std::string{ToString(representations_[representation_index_])} +
          (show_steps_ ? "_steps" : "_shade");
  if (view_index_ >= 0) {
    name += "_view" + std::to_string(view_index_);
  } else {
    double const degrees = std::fmod(static_cast<double>(azimuth_) * 180.0 / std::numbers::pi, 360.0);
    name += "_az" + std::to_string(static_cast<int>(std::lround(degrees)));
  }
  return std::filesystem::current_path() / (name + ".bmp");
}

void SceneViewer::Rebuild(Graphics& gfx) {
  volume_.reset();
  grid_.reset();

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
  if (representation == Representation::kDense && resolution_ > drawable::VoxelVolume::kMaxDenseResolution) {
    SetStatus(Describe() + L" | недоступно на GPU при N > 1024");
    return;
  }
  try {
    grid_ = BuildGrid(representation, resolution_, scene->shell_codes);
    volume_ = std::make_unique<drawable::VoxelVolume>(gfx, *grid_, width_, height_);
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
  SetStatus(Describe() + L" | " + memory + L" | " + std::to_wstring(scene->shell_codes.size()) + L" вокселей");
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
  gfx.ClearBuffer({0.f, 0.f, 0.f});
  if (volume_) {
    volume_->SetView(gfx,
                     MakeCameraBasis(MakeOrbitCameraAt(resolution_, azimuth_),
                                     static_cast<float>(width_) / static_cast<float>(height_)),
                     show_steps_);
    volume_->Draw(gfx);
  }
  if (capture_requested_) {
    capture_requested_ = false;
    if (volume_) {
      try {
        std::filesystem::path const path = CaptureFileName();
        FrameCapture::SaveBackBuffer(gfx, path);
        SetStatus(Describe() + L" | сохранён " + path.filename().wstring());
      } catch (std::exception const& e) {
        SetStatus(Describe() + L" | снимок не сохранён: " + Widen(e.what()));
      }
    }
  }
  gfx.EndFrame();
}
}  // namespace gpu_renderer::voxel
