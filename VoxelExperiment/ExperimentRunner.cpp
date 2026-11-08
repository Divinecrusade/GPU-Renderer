#include "ExperimentRunner.hpp"

#include <chrono>
#include <iostream>
#include <string>

#include "../Error.hpp"
#include "SceneGenerators.hpp"

namespace gpu_renderer::voxel {
namespace {
using Clock = std::chrono::steady_clock;

[[nodiscard]] double MillisecondsSince(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// CSV-безопасная узкая строка: не-ASCII заменяется на '?', разделитель ';' — на ','.
[[nodiscard]] std::string ToCsvText(std::wstring_view text) {
  std::string result;
  for (wchar_t const ch : text) {
    result.push_back(ch == L';' ? ',' : (ch < 128 ? static_cast<char>(ch) : '?'));
  }
  return result;
}

[[nodiscard]] std::string ToCsvText(std::string_view text) {
  std::string result{text};
  for (char& ch : result) {
    if (ch == ';' || ch == '\n') {
      ch = ',';
    }
  }
  return result;
}

[[nodiscard]] std::wstring Widen(std::string_view text) {
  std::wstring result;
  for (char const ch : text) {
    result.push_back(static_cast<unsigned char>(ch) < 128u ? static_cast<wchar_t>(ch) : L'?');
  }
  return result;
}

void Log(std::string const& message) { std::wclog << Widen(message) << L'\n'; }
}  // namespace

ExperimentRunner::ExperimentRunner(Graphics& gfx, ExperimentConfig config, std::uint32_t width,
                                   std::uint32_t height)
    : config_{std::move(config)},
      width_{width},
      height_{height},
      timer_{gfx},
      cases_{BuildSceneCases(config_)},
      cache_(cases_.size()) {
  if (config_.view_only) {
    StartViewer();
    return;
  }
  report_.emplace(config_.report_path);
  if (cases_.empty()) {
    phase_ = Phase::kFinished;
  }
  Log("Voxel experiment: " + std::to_string(cases_.size()) + " scene cases x " +
      std::to_string(config_.representations.size()) + " representations x " + std::to_string(config_.views) +
      " views, report: " + config_.report_path.string());
}

bool ExperimentRunner::IsFinished() const noexcept { return phase_ == Phase::kFinished; }

void ExperimentRunner::OnKeyPressed(std::uint8_t key) noexcept {
  if (viewer_) {
    viewer_->OnKeyPressed(key);
  }
}

std::optional<std::wstring> ExperimentRunner::TakeStatusChange() {
  std::wstring status = viewer_ ? viewer_->Status() : ProgressText();
  if (status == shown_status_) {
    return std::nullopt;
  }
  shown_status_ = status;
  return status;
}

std::wstring ExperimentRunner::ProgressText() const {
  if (cases_.empty()) {
    return L"Эксперимент: нет сцен";
  }
  std::wstring text = L"Эксперимент: сцена " + std::to_wstring(case_index_ + 1u) + L"/" +
                      std::to_wstring(cases_.size()) + L" (" + Widen(SceneCaseName(cases_[case_index_]));
  std::uint32_t const n = scene_loaded_ ? scene_.resolution : cases_[case_index_].resolution;
  if (n != 0u) {
    text += L" N=" + std::to_wstring(n);
  }
  text += L"), " + Widen(ToString(config_.representations[representation_index_]));
  switch (phase_) {
    case Phase::kPrepare:
      text += L", подготовка…";
      break;
    case Phase::kWarmup:
    case Phase::kMeasure:
      text += L", ракурс " + std::to_wstring(view_index_ + 1u) + L"/" + std::to_wstring(config_.views) +
              (phase_ == Phase::kWarmup ? L", прогрев" : L", серия " + std::to_wstring(series_done_ + 1u) +
                                                             L"/" + std::to_wstring(config_.series));
      break;
    case Phase::kFinished:
      break;
  }
  return text;
}

void ExperimentRunner::RenderFrame(Graphics& gfx, FrameTimer::DeltaTime dt) {
  if (viewer_) {
    viewer_->RenderFrame(gfx, dt);
    return;
  }
  if (phase_ == Phase::kFinished) {  // просматривать нечего: нет сцен
    gfx.ClearBuffer({0.f, 0.f, 0.f});
    gfx.EndFrame();
    return;
  }
  if (phase_ == Phase::kPrepare) {
    PrepareRepresentation(gfx);  // тяжёлая CPU-работа; окно на это время не обновляется
    gfx.ClearBuffer({0.f, 0.f, 0.f});
    gfx.EndFrame();
    return;
  }

  gfx.ClearBuffer({0.f, 0.f, 0.f});
  timer_.Begin(gfx);
  volume_->Draw(gfx);
  timer_.End(gfx);
  gfx.EndFrame();
  std::optional<double> const frame_ms = timer_.WaitMilliseconds(gfx);
  ++frame_in_phase_;

  if (phase_ == Phase::kWarmup) {
    if (frame_in_phase_ >= config_.warmup_frames) {
      phase_ = Phase::kMeasure;
      frame_in_phase_ = 0u;
    }
    return;
  }

  if (frame_ms) {
    frame_ms_.push_back(*frame_ms);
  }
  if (frame_in_phase_ >= config_.frames_per_series) {
    if (!frame_ms_.empty()) {
      series_ms_.push_back(Mean(frame_ms_));  // среднее по серии — одно наблюдение
    }
    frame_ms_.clear();
    frame_in_phase_ = 0u;
    if (++series_done_ >= config_.series) {
      FinishView(gfx);
    }
  }
}

bool ExperimentRunner::LoadScene() {
  SceneCase const& scene = cases_[case_index_];
  try {
    scene_ = LoadSceneCase(scene);
  } catch (std::exception const& e) {
    Log(std::string{"Scene loading failed: "} + e.what());
    scene_ = LoadedScene{};
    scene_.resolution = scene.resolution;
    return false;
  }
  scene_loaded_ = true;
  Log("Scene " + SceneCaseName(scene) + " N=" + std::to_string(scene_.resolution) + ": " +
      std::to_string(scene_.shell_codes.size()) + " boundary voxels, " + std::to_string(scene_.generation_ms) +
      " ms");
  return true;
}

void ExperimentRunner::PrepareRepresentation(Graphics& gfx) {
  if (!scene_loaded_ && !LoadScene()) {
    grid_.reset();
    WriteCpuOnlyRows("failed: scene loading");
    representation_index_ = config_.representations.size() - 1u;  // пропустить сцену целиком
    AdvanceRepresentation();
    return;
  }

  Representation const representation = config_.representations[representation_index_];
  auto const start = Clock::now();
  try {
    grid_ = BuildGrid(representation, scene_.resolution, scene_.shell_codes);
  } catch (std::exception const& e) {
    grid_.reset();
    build_ms_ = MillisecondsSince(start);
    WriteCpuOnlyRows("failed: build: " + ToCsvText(e.what()));
    AdvanceRepresentation();
    return;
  }
  build_ms_ = MillisecondsSince(start);
  Log("  " + std::string{ToString(representation)} + ": " + std::to_string(grid_->MemoryBytes()) + " bytes, built in " +
      std::to_string(build_ms_) + " ms");

  if (representation == Representation::kDense && scene_.resolution > drawable::VoxelVolume::kMaxDenseResolution) {
    WriteCpuOnlyRows("skipped: dense index is 32-bit (N > 1024)");
    AdvanceRepresentation();
    return;
  }
  try {
    volume_ = std::make_unique<drawable::VoxelVolume>(gfx, *grid_, width_, height_);
  } catch (exception::Error const& e) {
    volume_.reset();
    WriteCpuOnlyRows("failed: GPU upload: " + ToCsvText(e.GetTypeOfException()));
    AdvanceRepresentation();
    return;
  } catch (std::exception const& e) {
    volume_.reset();
    WriteCpuOnlyRows("failed: GPU upload: " + ToCsvText(e.what()));
    AdvanceRepresentation();
    return;
  }
  view_index_ = 0u;
  PrepareView(gfx);
}

CameraBasis ExperimentRunner::ViewBasis(std::size_t view) const noexcept {
  return MakeCameraBasis(MakeOrbitCamera(scene_.resolution, view, config_.views),
                         static_cast<float>(width_) / static_cast<float>(height_));
}

void ExperimentRunner::PrepareView(Graphics& gfx) {
  CameraBasis const basis = ViewBasis(view_index_);
  volume_->SetView(gfx, basis, false);
  cpu_stats_ = TraceImage(*grid_, basis, width_, height_, config_.cpu_stride);
  phase_ = Phase::kWarmup;
  frame_in_phase_ = 0u;
  series_done_ = 0u;
  frame_ms_.clear();
  series_ms_.clear();
}

MeasurementRow ExperimentRunner::MakeRow(std::size_t view) const {
  MeasurementRow row;
  row.scene = SceneCaseName(cases_[case_index_]);
  row.resolution = scene_.resolution;
  row.representation = std::string{ToString(config_.representations[representation_index_])};
  row.view = view;
  row.boundary_voxels = scene_.shell_codes.size();
  row.generation_ms = scene_.generation_ms;
  row.build_ms = build_ms_;
  row.memory_bytes = grid_ ? grid_->MemoryBytes() : 0u;
  row.cpu_rays = cpu_stats_.rays;
  row.cpu_mean_steps = cpu_stats_.mean_steps;
  row.hit_rate = cpu_stats_.hit_rate;
  return row;
}

void ExperimentRunner::FinishView(Graphics& gfx) {
  MeasurementRow row = MakeRow(view_index_);
  row.gpu_status = series_ms_.empty() ? "failed: all GPU timestamps disjoint" : "ok";
  row.gpu_ms_mean = Mean(series_ms_);
  row.gpu_ms_std = StdDev(series_ms_);
  if (row.gpu_ms_mean > 0.0) {
    row.mrays_per_second = static_cast<double>(width_) * height_ / (row.gpu_ms_mean * 1e-3) / 1e6;
  }
  report_->Append(row);
  Log("    view " + std::to_string(view_index_) + ": " + std::to_string(row.gpu_ms_mean) + " +- " +
      std::to_string(row.gpu_ms_std) + " ms, " + std::to_string(row.mrays_per_second) + " Mrays/s, " +
      std::to_string(row.cpu_mean_steps) + " steps/ray");

  if (++view_index_ < config_.views) {
    PrepareView(gfx);
  } else {
    AdvanceRepresentation();
  }
}

void ExperimentRunner::WriteCpuOnlyRows(std::string const& gpu_status) {
  Log("  " + gpu_status);
  for (std::size_t view = 0u; view < config_.views; ++view) {
    cpu_stats_ = grid_ ? TraceImage(*grid_, ViewBasis(view), width_, height_, config_.cpu_stride) : ImageTraceStats{};
    MeasurementRow row = MakeRow(view);
    row.gpu_status = gpu_status;
    report_->Append(row);
  }
}

void ExperimentRunner::AdvanceRepresentation() {
  volume_.reset();
  grid_.reset();
  bool const last_representation = representation_index_ + 1u >= config_.representations.size();
  if (!last_representation) {
    ++representation_index_;
    phase_ = Phase::kPrepare;
    return;
  }

  // Сцена измерена во всех представлениях: в кэш для просмотра, если не слишком велика.
  if (scene_loaded_ && scene_.shell_codes.size() <= SceneViewer::kMaxCachedVoxels) {
    cache_[case_index_] = std::move(scene_);
  }
  scene_ = LoadedScene{};
  scene_loaded_ = false;
  representation_index_ = 0u;

  if (case_index_ + 1u >= cases_.size()) {
    phase_ = Phase::kFinished;
    Log("Voxel experiment finished, results: " + config_.report_path.string());
    StartViewer();
    return;
  }
  ++case_index_;
  phase_ = Phase::kPrepare;
}

void ExperimentRunner::StartViewer() {
  phase_ = Phase::kFinished;
  if (cases_.empty() || config_.representations.empty()) {
    return;
  }
  // Начинаем с последней измеренной сцены и SVO (если оно есть в списке).
  std::size_t start_representation = 0u;
  for (std::size_t i = 0u; i < config_.representations.size(); ++i) {
    if (config_.representations[i] == Representation::kSvo) {
      start_representation = i;
    }
  }
  std::size_t const start_case = config_.view_only ? 0u : cases_.size() - 1u;
  viewer_ = std::make_unique<SceneViewer>(cases_, std::move(cache_), config_.representations, width_, height_,
                                          start_case, start_representation);
  cache_.clear();
}
}  // namespace gpu_renderer::voxel
