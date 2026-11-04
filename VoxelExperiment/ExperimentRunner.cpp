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

void Log(std::string const& message) { std::wclog << std::wstring(message.begin(), message.end()) << L'\n'; }
}  // namespace

ExperimentRunner::ExperimentRunner(Graphics& gfx, ExperimentConfig config, std::uint32_t width,
                                   std::uint32_t height)
    : config_{std::move(config)}, width_{width}, height_{height}, report_{config_.report_path}, timer_{gfx} {
  for (SceneKind const scene : config_.scenes) {
    for (std::uint32_t const n : config_.resolutions) {
      cases_.push_back({scene, n, {}});
    }
  }
  for (auto const& model : config_.models) {
    cases_.push_back({SceneKind::kModel, 0u, model});
  }
  if (cases_.empty()) {
    phase_ = Phase::kFinished;
  }
  Log("Voxel experiment: " + std::to_string(cases_.size()) + " scene cases x " +
      std::to_string(config_.representations.size()) + " representations x " + std::to_string(config_.views) +
      " views, report: " + config_.report_path.string());
}

bool ExperimentRunner::IsFinished() const noexcept { return phase_ == Phase::kFinished; }

void ExperimentRunner::RenderFrame(Graphics& gfx, FrameTimer::DeltaTime dt) {
  if (phase_ == Phase::kFinished) {
    RenderIdle(gfx, dt);
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
  auto const start = Clock::now();
  try {
    switch (scene.kind) {
      case SceneKind::kSphere:
        resolution_ = scene.resolution;
        codes_ = GenerateSphereShell(resolution_, resolution_ / 2.0 - 1.0);
        break;
      case SceneKind::kTerrain:
        resolution_ = scene.resolution;
        codes_ = GenerateTerrainShell(resolution_);
        break;
      case SceneKind::kModel: {
        VoxelModel model = LoadBinvox(scene.model_path);
        resolution_ = model.resolution;
        codes_ = std::move(model.shell_codes);
        break;
      }
    }
  } catch (std::exception const& e) {
    Log(std::string{"Scene loading failed: "} + e.what());
    resolution_ = scene.resolution;
    codes_.clear();
    generation_ms_ = MillisecondsSince(start);
    return false;
  }
  generation_ms_ = MillisecondsSince(start);
  scene_loaded_ = true;
  Log("Scene " + std::string{ToString(scene.kind)} + " N=" + std::to_string(resolution_) + ": " +
      std::to_string(codes_.size()) + " boundary voxels, " + std::to_string(generation_ms_) + " ms");
  return true;
}

void ExperimentRunner::PrepareRepresentation(Graphics& gfx) {
  if (!scene_loaded_ && !LoadScene()) {
    grid_.reset();
    WriteCpuOnlyRows("failed: scene loading");
    representation_index_ = config_.representations.size() - 1u;  // пропустить сцену целиком
    AdvanceRepresentation(gfx);
    return;
  }

  Representation const representation = config_.representations[representation_index_];
  auto const start = Clock::now();
  try {
    grid_ = BuildGrid(representation, resolution_, codes_);
  } catch (std::exception const& e) {
    grid_.reset();
    build_ms_ = MillisecondsSince(start);
    WriteCpuOnlyRows("failed: build: " + ToCsvText(e.what()));
    AdvanceRepresentation(gfx);
    return;
  }
  build_ms_ = MillisecondsSince(start);
  Log("  " + std::string{ToString(representation)} + ": " + std::to_string(grid_->MemoryBytes()) + " bytes, built in " +
      std::to_string(build_ms_) + " ms");

  if (representation == Representation::kDense && resolution_ > drawable::VoxelVolume::kMaxDenseResolution) {
    WriteCpuOnlyRows("skipped: dense index is 32-bit (N > 1024)");
    AdvanceRepresentation(gfx);
    return;
  }
  try {
    volume_ = std::make_unique<drawable::VoxelVolume>(gfx, *grid_, width_, height_);
  } catch (exception::Error const& e) {
    volume_.reset();
    WriteCpuOnlyRows("failed: GPU upload: " + ToCsvText(e.GetTypeOfException()));
    AdvanceRepresentation(gfx);
    return;
  } catch (std::exception const& e) {
    volume_.reset();
    WriteCpuOnlyRows("failed: GPU upload: " + ToCsvText(e.what()));
    AdvanceRepresentation(gfx);
    return;
  }
  view_index_ = 0u;
  PrepareView(gfx);
}

CameraBasis ExperimentRunner::ViewBasis(std::size_t view) const noexcept {
  return MakeCameraBasis(MakeOrbitCamera(resolution_, view, config_.views),
                         static_cast<float>(width_) / static_cast<float>(height_));
}

void ExperimentRunner::PrepareView(Graphics& gfx) {
  CameraBasis const basis = ViewBasis(view_index_);
  volume_->SetView(gfx, basis, false);
  cpu_stats_ = TraceImage(*grid_, basis, width_, height_);
  phase_ = Phase::kWarmup;
  frame_in_phase_ = 0u;
  series_done_ = 0u;
  frame_ms_.clear();
  series_ms_.clear();
}

MeasurementRow ExperimentRunner::MakeRow(std::size_t view) const {
  MeasurementRow row;
  row.scene = std::string{ToString(cases_[case_index_].kind)};
  if (cases_[case_index_].kind == SceneKind::kModel) {
    row.scene += ":" + ToCsvText(cases_[case_index_].model_path.filename().string());
  }
  row.resolution = resolution_;
  row.representation = std::string{ToString(config_.representations[representation_index_])};
  row.view = view;
  row.boundary_voxels = codes_.size();
  row.generation_ms = generation_ms_;
  row.build_ms = build_ms_;
  row.memory_bytes = grid_ ? grid_->MemoryBytes() : 0u;
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
  report_.Append(row);
  Log("    view " + std::to_string(view_index_) + ": " + std::to_string(row.gpu_ms_mean) + " +- " +
      std::to_string(row.gpu_ms_std) + " ms, " + std::to_string(row.mrays_per_second) + " Mrays/s, " +
      std::to_string(row.cpu_mean_steps) + " steps/ray");

  if (++view_index_ < config_.views) {
    PrepareView(gfx);
  } else {
    AdvanceRepresentation(gfx);
  }
}

void ExperimentRunner::WriteCpuOnlyRows(std::string const& gpu_status) {
  Log("  " + gpu_status);
  for (std::size_t view = 0u; view < config_.views; ++view) {
    cpu_stats_ = grid_ ? TraceImage(*grid_, ViewBasis(view), width_, height_) : ImageTraceStats{};
    MeasurementRow row = MakeRow(view);
    row.gpu_status = gpu_status;
    report_.Append(row);
  }
}

void ExperimentRunner::AdvanceRepresentation(Graphics& gfx) {
  bool const last_representation = representation_index_ + 1u >= config_.representations.size();
  bool const last_case = case_index_ + 1u >= cases_.size();
  if (last_representation && last_case) {
    // Последнюю сцену оставляем на экране; если её нельзя было загрузить на GPU — показываем SVO.
    if (!volume_ && !codes_.empty()) {
      try {
        grid_ = std::make_unique<SparseVoxelOctree>(resolution_, codes_);
        volume_ = std::make_unique<drawable::VoxelVolume>(gfx, *grid_, width_, height_);
      } catch (std::exception const&) {
        volume_.reset();
      }
    }
    phase_ = Phase::kFinished;
    Log("Voxel experiment finished, results: " + config_.report_path.string());
    return;
  }

  volume_.reset();
  grid_.reset();
  if (last_representation) {
    representation_index_ = 0u;
    ++case_index_;
    scene_loaded_ = false;
    codes_.clear();
    codes_.shrink_to_fit();
  } else {
    ++representation_index_;
  }
  phase_ = Phase::kPrepare;
}

void ExperimentRunner::RenderIdle(Graphics& gfx, FrameTimer::DeltaTime dt) {
  idle_time_ += dt;
  gfx.ClearBuffer({0.f, 0.f, 0.f});
  if (volume_) {
    constexpr float kAngularSpeed = 0.3f;     // рад/с
    constexpr float kHeatmapPeriod = 4.f;     // с
    bool const show_steps = static_cast<int>(idle_time_ / kHeatmapPeriod) % 2 == 1;
    volume_->SetView(gfx,
                     MakeCameraBasis(MakeOrbitCameraAt(resolution_, 0.6f + kAngularSpeed * idle_time_),
                                     static_cast<float>(width_) / static_cast<float>(height_)),
                     show_steps);
    volume_->Draw(gfx);
  }
  gfx.EndFrame();
}
}  // namespace gpu_renderer::voxel
