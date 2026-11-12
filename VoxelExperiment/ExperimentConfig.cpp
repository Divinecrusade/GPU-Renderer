#include "ExperimentConfig.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>

namespace gpu_renderer::voxel {
namespace {
constexpr std::wstring_view kExperimentFlag = L"--voxel-experiment";

[[nodiscard]] std::vector<std::wstring> Tokenize(std::wstring_view command_line) {
  std::vector<std::wstring> tokens;
  std::wstring current;
  bool in_quotes = false;
  for (wchar_t const ch : command_line) {
    if (ch == L'"') {
      in_quotes = !in_quotes;
    } else if ((ch == L' ' || ch == L'\t') && !in_quotes) {
      if (!current.empty()) {
        tokens.push_back(std::move(current));
        current.clear();
      }
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    tokens.push_back(std::move(current));
  }
  return tokens;
}

[[nodiscard]] std::string Narrow(std::wstring_view text) {
  std::string result;
  result.reserve(text.size());
  for (wchar_t const ch : text) {
    result.push_back(ch < 128 ? static_cast<char>(ch) : '?');
  }
  return result;
}

[[nodiscard]] std::vector<std::wstring> SplitList(std::wstring_view list) {
  std::vector<std::wstring> items;
  std::size_t begin = 0u;
  while (begin <= list.size()) {
    std::size_t const end = std::min(list.find(L',', begin), list.size());
    if (end > begin) {
      items.emplace_back(list.substr(begin, end - begin));
    }
    begin = end + 1u;
  }
  return items;
}

[[nodiscard]] std::uint32_t ParseUnsigned(std::wstring_view value, std::wstring_view option) {
  try {
    std::size_t consumed = 0u;
    unsigned long const parsed = std::stoul(std::wstring{value}, &consumed);
    if (consumed != value.size() || parsed == 0u) {
      throw std::invalid_argument{""};
    }
    return static_cast<std::uint32_t>(parsed);
  } catch (std::exception const&) {
    throw std::invalid_argument{"Option " + Narrow(option) + " expects a positive integer, got '" +
                                Narrow(value) + "'"};
  }
}
}  // namespace

std::vector<SceneCase> BuildSceneCases(ExperimentConfig const& config) {
  std::vector<SceneCase> cases;
  for (SceneKind const scene : config.scenes) {
    for (std::uint32_t const n : config.resolutions) {
      cases.push_back({scene, n, {}});
    }
  }
  for (auto const& model : config.models) {
    cases.push_back({SceneKind::kModel, 0u, model});
  }
  return cases;
}

std::vector<MeasuredVariant> BuildVariants(ExperimentConfig const& config) {
  std::vector<MeasuredVariant> variants;
  for (Representation const r : config.representations) {
    bool const hierarchical = r == Representation::kSvo || r == Representation::kSvdag;
    if (!hierarchical) {
      variants.push_back({r, Traversal::kStack, false});
      continue;
    }
    for (Traversal const t : config.traversals) {
      variants.push_back({r, t, true});
    }
  }
  return variants;
}

bool IsExperimentRequested(std::wstring_view command_line) {
  for (std::wstring const& token : Tokenize(command_line)) {
    if (token == kExperimentFlag) {
      return true;
    }
  }
  return false;
}

ExperimentConfig ParseExperimentConfig(std::wstring_view command_line) {
  ExperimentConfig config;
  bool models_given = false;
  for (std::wstring const& token : Tokenize(command_line)) {
    if (token == kExperimentFlag) {
      continue;
    }
    std::size_t const eq = token.find(L'=');
    std::wstring_view const option = std::wstring_view{token}.substr(0u, eq);
    std::wstring_view const value = eq == std::wstring::npos ? std::wstring_view{} : std::wstring_view{token}.substr(eq + 1u);

    if (option == L"--voxel-scenes") {
      config.scenes.clear();
      for (std::wstring const& item : SplitList(value)) {
        if (item == L"sphere") {
          config.scenes.push_back(SceneKind::kSphere);
        } else if (item == L"terrain") {
          config.scenes.push_back(SceneKind::kTerrain);
        } else if (item != L"none") {
          throw std::invalid_argument{"Unknown scene '" + Narrow(item) + "'"};
        }
      }
    } else if (option == L"--voxel-resolutions") {
      config.resolutions.clear();
      for (std::wstring const& item : SplitList(value)) {
        std::uint32_t const n = ParseUnsigned(item, option);
        if (n < 16u || !std::has_single_bit(n)) {
          throw std::invalid_argument{"Resolution must be a power of two >= 16, got " + std::to_string(n)};
        }
        config.resolutions.push_back(n);
      }
    } else if (option == L"--voxel-model") {
      if (value.empty()) {
        throw std::invalid_argument{"Option --voxel-model expects a path"};
      }
      config.models.emplace_back(value);
      models_given = true;
    } else if (option == L"--voxel-representations") {
      config.representations.clear();
      for (std::wstring const& item : SplitList(value)) {
        bool found = false;
        for (Representation const r : {Representation::kDense, Representation::kBlock, Representation::kSvo,
                                       Representation::kSvdag, Representation::kDenseBrick}) {
          if (Narrow(item) == ToString(r)) {
            config.representations.push_back(r);
            found = true;
          }
        }
        if (!found) {
          throw std::invalid_argument{"Unknown representation '" + Narrow(item) + "'"};
        }
      }
    } else if (option == L"--voxel-traversal") {
      config.traversals.clear();
      for (std::wstring const& item : SplitList(value)) {
        if (item == L"stack") {
          config.traversals.push_back(Traversal::kStack);
        } else if (item == L"restart") {
          config.traversals.push_back(Traversal::kRestart);
        } else {
          throw std::invalid_argument{"Unknown traversal '" + Narrow(item) + "' (expected stack or restart)"};
        }
      }
      if (config.traversals.empty()) {
        throw std::invalid_argument{"Option --voxel-traversal expects stack and/or restart"};
      }
    } else if (option == L"--voxel-views") {
      config.views = ParseUnsigned(value, option);
    } else if (option == L"--voxel-warmup-ms") {
      config.warmup_ms = ParseUnsigned(value, option);
    } else if (option == L"--voxel-warmup") {
      config.warmup_frames = ParseUnsigned(value, option);
    } else if (option == L"--voxel-series") {
      config.series = ParseUnsigned(value, option);
    } else if (option == L"--voxel-frames") {
      config.frames_per_series = ParseUnsigned(value, option);
    } else if (option == L"--voxel-cpu-stride") {
      config.cpu_stride = ParseUnsigned(value, option);
    } else if (option == L"--voxel-view-only") {
      if (eq != std::wstring::npos) {
        throw std::invalid_argument{"Option --voxel-view-only takes no value"};
      }
      config.view_only = true;
    } else if (option == L"--voxel-report") {
      if (value.empty()) {
        throw std::invalid_argument{"Option --voxel-report expects a path"};
      }
      config.report_path = std::filesystem::path{value};
    } else {
      throw std::invalid_argument{"Unknown option '" + Narrow(token) + "'"};
    }
  }
  if (config.representations.empty()) {
    throw std::invalid_argument{"At least one representation is required"};
  }
  if (config.scenes.empty() && !models_given) {
    throw std::invalid_argument{"Nothing to measure: no scenes and no models"};
  }
  return config;
}
}  // namespace gpu_renderer::voxel
