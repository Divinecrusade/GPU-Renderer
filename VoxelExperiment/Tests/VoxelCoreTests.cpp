// Тесты ядра воксельного эксперимента (без DirectX). Не входит в сборку GPU-Renderer.vcxproj.
// Сборка (из корня репозитория):
//   g++ -O2 -std=c++20 -pthread VoxelExperiment/Tests/VoxelCoreTests.cpp VoxelExperiment/VoxelGrid.cpp
//       VoxelExperiment/SceneGenerators.cpp VoxelExperiment/RayTraversal.cpp
//       VoxelExperiment/ExperimentReport.cpp VoxelExperiment/ExperimentConfig.cpp -o voxel_core_tests
//   MSVC: cl /std:c++20 /O2 /EHsc <те же файлы>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../ExperimentConfig.hpp"
#include "../ExperimentReport.hpp"
#include "../Morton.hpp"
#include "../RayTraversal.hpp"
#include "../SceneGenerators.hpp"
#include "../VoxelGrid.hpp"

namespace {
int g_failures = 0;

void Check(bool condition, char const* what) {
  std::printf("[%s] %s\n", condition ? " OK " : "FAIL", what);
  if (!condition) {
    ++g_failures;
  }
}

using namespace gpu_renderer::voxel;

constexpr Representation kAll[] = {Representation::kDense, Representation::kBlock, Representation::kSvo,
                                   Representation::kSvdag};

std::vector<std::unique_ptr<IVoxelGrid>> BuildAll(std::uint32_t n, std::vector<std::uint64_t> const& codes) {
  std::vector<std::unique_ptr<IVoxelGrid>> grids;
  for (Representation const r : kAll) {
    grids.push_back(BuildGrid(r, n, codes));
  }
  return grids;
}

bool QueriesAgree(std::vector<std::unique_ptr<IVoxelGrid>> const& grids, std::vector<std::uint64_t> const& codes,
                  std::uint32_t n) {
  std::mt19937_64 rng{42u};
  for (int i = 0; i < 200000; ++i) {
    VoxelCoord c{};
    if (i % 2 == 0) {  // окрестность оболочки
      c = DecodeMorton(codes[rng() % codes.size()]);
      c.x = std::min(n - 1u, c.x + static_cast<std::uint32_t>(rng() % 2u));
    } else {
      c = {static_cast<std::uint32_t>(rng() % n), static_cast<std::uint32_t>(rng() % n),
           static_cast<std::uint32_t>(rng() % n)};
    }
    bool const reference = grids.front()->Get(c.x, c.y, c.z);
    for (auto const& grid : grids) {
      if (grid->Get(c.x, c.y, c.z) != reference) {
        return false;
      }
    }
  }
  return true;
}

// Доля лучей, у которых точка попадания отличается от плотной сетки (DDA).
double RayMismatchRate(std::vector<std::unique_ptr<IVoxelGrid>> const& grids, std::uint32_t n) {
  constexpr std::uint32_t kWidth = 160u;
  constexpr std::uint32_t kHeight = 120u;
  std::size_t mismatches = 0u;
  std::size_t total = 0u;
  for (std::size_t view = 0u; view < 3u; ++view) {
    CameraBasis const basis = MakeCameraBasis(MakeOrbitCamera(n, view, 3u), 4.f / 3.f);
    for (std::uint32_t py = 0u; py < kHeight; ++py) {
      for (std::uint32_t px = 0u; px < kWidth; ++px) {
        Float3 const dir = PrimaryRayDirection(basis, px, py, kWidth, kHeight);
        RayHit const reference = TraceRay(*grids.front(), basis.position, dir);
        for (std::size_t g = 1u; g < grids.size(); ++g) {
          RayHit const hit = TraceRay(*grids[g], basis.position, dir);
          ++total;
          if (hit.hit != reference.hit ||
              (hit.hit && (hit.x != reference.x || hit.y != reference.y || hit.z != reference.z))) {
            ++mismatches;
          }
        }
      }
    }
  }
  return static_cast<double>(mismatches) / static_cast<double>(total);
}
}  // namespace

int main() {
  Check(DecodeMorton(EncodeMorton(1234u, 77u, 2047u)).z == 2047u, "Morton: кодирование обратимо");

  {  // Формула (5): |dOmega| ~ c * 4 pi R^2, c = E[max|n_i|] ~ 0.831
    std::uint32_t const n = 512u;
    double const radius = n / 2.0 - 1.0;
    auto const codes = GenerateSphereShell(n, radius);
    double const predicted = 0.8312 * 4.0 * std::numbers::pi * radius * radius;
    double const error = std::fabs(static_cast<double>(codes.size()) / predicted - 1.0);
    std::printf("      сфера N=%u: %zu граничных вокселей, оценка %.0f, погрешность %.3f%%\n", n, codes.size(),
                predicted, 100.0 * error);
    Check(error < 0.01, "Сфера: число граничных вокселей совпадает с формулой (5) с точностью 1%");
  }

  for (bool const terrain : {false, true}) {
    std::uint32_t const n = terrain ? 128u : 256u;
    auto const codes = terrain ? GenerateTerrainShell(n) : GenerateSphereShell(n, n / 2.0 - 1.0);
    auto const grids = BuildAll(n, codes);
    std::string const scene = terrain ? "Ландшафт" : "Сфера";
    Check(!codes.empty(), (scene + ": сцена не пуста").c_str());
    Check(QueriesAgree(grids, codes, n), (scene + ": точечные запросы всех представлений совпадают").c_str());
    auto const& dag = static_cast<SparseVoxelDag const&>(*grids[3]);
    std::printf("      %s: уникальных узлов SVDAG на уровне D-1: %zu\n", scene.c_str(),
                dag.UniqueNodesPerLevel().back());
    Check(dag.UniqueNodesPerLevel().back() <= 255u, (scene + ": SVDAG, уровень D-1 <= 255 узлов").c_str());
    Check(dag.MemoryBytes() < grids[2]->MemoryBytes(), (scene + ": SVDAG меньше SVO").c_str());
    bool bounded = true;
    std::uint32_t const depth = Log2Ceil(n);
    for (auto const& grid : grids) {
      for (std::size_t view = 0u; view < 3u; ++view) {
        ImageTraceStats const stats =
            TraceImage(*grid, MakeCameraBasis(MakeOrbitCamera(n, view, 3u), 16.f / 9.f), 320u, 180u);
        std::uint32_t const limit =
            grid->GetRepresentation() == Representation::kDense ? 3u * n + 1u : 6u * n + depth + 1u;
        bounded = bounded && stats.max_steps <= limit;
      }
    }
    Check(bounded, (scene + ": шагов на луч не больше оценки худшего случая (нет зацикливания)").c_str());
    double const mismatch = RayMismatchRate(grids, n);
    std::printf("      %s: доля расхождений точек попадания с DDA: %.4f%%\n", scene.c_str(), 100.0 * mismatch);
    Check(mismatch < 0.005, (scene + ": обход лучей согласован с DDA (< 0.5% касательных лучей)").c_str());
  }

  {  // Выборка пикселей для CPU-обхода: среднее почти не меняется, лучей в stride^2 меньше
    std::uint32_t const n = 256u;
    auto const codes = GenerateTerrainShell(n);
    SparseVoxelOctree const svo{n, codes};
    CameraBasis const basis = MakeCameraBasis(MakeOrbitCamera(n, 0u, 3u), 16.f / 9.f);
    ImageTraceStats const full = TraceImage(svo, basis, 480u, 270u, 1u);
    ImageTraceStats const sampled = TraceImage(svo, basis, 480u, 270u, 4u);
    double const deviation = std::fabs(sampled.mean_steps / full.mean_steps - 1.0);
    std::printf("      шагов на луч: все пиксели %.3f (%zu лучей), шаг 4: %.3f (%zu лучей), отклонение %.2f%%\n",
                full.mean_steps, full.rays, sampled.mean_steps, sampled.rays, 100.0 * deviation);
    Check(full.rays == 480u * 270u && sampled.rays == 120u * 67u, "TraceImage: число лучей при выборке");
    Check(deviation < 0.03, "TraceImage: выборка с шагом 4 меняет среднее меньше чем на 3%");
  }

  {  // Сцены эксперимента
    ExperimentConfig config;
    config.scenes = {SceneKind::kSphere};
    config.resolutions = {64u, 128u};
    config.models = {std::filesystem::path{"a.binvox"}};
    auto const cases = BuildSceneCases(config);
    Check(cases.size() == 3u && cases[1].resolution == 128u && cases[2].kind == SceneKind::kModel &&
              SceneCaseName(cases[2]) == "model:a.binvox" && SceneCaseName(cases[0]) == "sphere",
          "Сцены: порядок и имена");
    LoadedScene const sphere = LoadSceneCase(cases[0]);
    Check(sphere.resolution == 64u && sphere.shell_codes == GenerateSphereShell(64u, 31.0), "Сцены: загрузка сферы");
    bool thrown = false;
    try {
      static_cast<void>(LoadSceneCase(cases[2]));
    } catch (std::exception const&) {
      thrown = true;
    }
    Check(thrown, "Сцены: отсутствующая модель — исключение");
  }

  {  // Ландшафт: многопоточная генерация детерминирована
    auto const single = GenerateTerrainShell(128u, {}, 1u);
    auto const multi = GenerateTerrainShell(128u, {}, 8u);
    Check(single == multi, "Ландшафт: результат не зависит от числа потоков");
  }

  {  // binvox: запись и чтение оболочки без потерь
    std::uint32_t const n = 64u;
    auto const codes = GenerateSphereShell(n, 30.0);
    auto const path = std::filesystem::temp_directory_path() / "voxel_core_test.binvox";
    SaveBinvox(path, n, codes);
    VoxelModel const model = LoadBinvox(path);
    std::filesystem::remove(path);
    Check(model.resolution == n && model.shell_codes == codes, "binvox: запись и чтение без потерь");
  }

  {  // Статистика
    double const xs[] = {128.0, 256.0, 512.0, 1024.0};
    double const ys[] = {3.0 * 128.0 * 128.0, 3.0 * 256.0 * 256.0, 3.0 * 512.0 * 512.0, 3.0 * 1024.0 * 1024.0};
    Check(std::fabs(FitPowerLawExponent(xs, ys) - 2.0) < 1e-9, "МНК: показатель степени y = 3x^2 равен 2");
    double const sample[] = {1.0, 2.0, 3.0, 4.0};
    Check(std::fabs(Mean(sample) - 2.5) < 1e-12 && std::fabs(StdDev(sample) - std::sqrt(5.0 / 3.0)) < 1e-12,
          "Среднее и выборочное СКО");
  }

  {  // CSV
    auto const path = std::filesystem::temp_directory_path() / "voxel_core_test.csv";
    {
      CsvReport report{path};
      report.Append({.scene = "sphere", .resolution = 256u, .representation = "svo", .gpu_status = "ok"});
    }
    std::ifstream file{path};
    std::string header;
    std::string row;
    std::getline(file, header);
    std::getline(file, row);
    file.close();
    std::filesystem::remove(path);
    Check(header.rfind("scene;N;", 0) == 0 && row.rfind("sphere;256;svo;", 0) == 0, "CSV: заголовок и строка");
  }

  {  // Командная строка
    std::wstring_view const line =
        L"--voxel-experiment --voxel-scenes=terrain --voxel-resolutions=128,512 "
        L"--voxel-model=\"C:\\models\\my model.binvox\" --voxel-representations=svo,svdag --voxel-views=2 "
        L"--voxel-cpu-stride=8 --voxel-view-only";
    ExperimentConfig const config = ParseExperimentConfig(line);
    Check(IsExperimentRequested(line) && !IsExperimentRequested(L"--other"), "Конфигурация: флаг эксперимента");
    Check(config.scenes.size() == 1u && config.scenes[0] == SceneKind::kTerrain && config.resolutions.size() == 2u &&
              config.resolutions[1] == 512u && config.models.size() == 1u &&
              config.models[0] == std::filesystem::path{L"C:\\models\\my model.binvox"} &&
              config.representations.size() == 2u && config.representations[1] == Representation::kSvdag &&
              config.views == 2u && config.cpu_stride == 8u && config.view_only,
          "Конфигурация: опции и путь в кавычках");
    bool rejected = false;
    try {
      static_cast<void>(ParseExperimentConfig(L"--voxel-experiment --voxel-resolutions=300"));
    } catch (std::invalid_argument const&) {
      rejected = true;
    }
    Check(rejected, "Конфигурация: N не степень двойки отклоняется");
  }

  std::printf("\n%s: %d ошибок\n", g_failures == 0 ? "ВСЕ ТЕСТЫ ПРОЙДЕНЫ" : "ЕСТЬ ОШИБКИ", g_failures);
  return g_failures == 0 ? 0 : 1;
}
