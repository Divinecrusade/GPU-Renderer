#ifndef SHADER_HPP
#define SHADER_HPP

#include "OptimisedStlHeader.hpp"

#include "Graphics.hpp"
#include "SupportedShaderType.hpp"

namespace gpu_renderer::bindable {
template <SupportedShaderType T>
class Shader : public Graphics::Bindable {
 public:
  Shader() = delete;
  Shader(std::filesystem::path const& shader_file, Graphics& gfx)
    : Bindable{gfx} {
    assert(shader_file.extension() == L".cso");
    GetDebugger().StartTraceInDebugMode();
    if (HRESULT const operation_status =
            D3DReadFileToBlob(shader_file.c_str(), &shader_blob_);
        FAILED(operation_status)) {
      throw GetDebugger().CreateDirectXError(operation_status, 
                                             "Failed to read compiled pixel shader into blob",
                                             "Error during shader loading", 
                                             __FILEW__, __LINE__);
    }

#pragma warning(push)
#pragma warning(disable : 26462)
    constexpr ID3D11ClassLinkage* kNoClassLinkage = nullptr;
#pragma warning(pop)
    GetDebugger().StartTraceInDebugMode();
    if constexpr (T == SupportedShaderType::kPixel) {
      if (HRESULT const operation_status = GetDevice().CreatePixelShader(
              shader_blob_->GetBufferPointer(), shader_blob_->GetBufferSize(),
              kNoClassLinkage, &shader_);
          FAILED(operation_status)) {
        throw GetDebugger().CreateDirectXError(operation_status, 
                                               "Failed to create pixel shader from blob",
                                               "Error during shader loading", 
                                               __FILEW__, __LINE__);
      }
    } else if constexpr (T == SupportedShaderType::kVertex) {
      if (HRESULT const operation_status = GetDevice().CreateVertexShader(
              shader_blob_->GetBufferPointer(), shader_blob_->GetBufferSize(),
              kNoClassLinkage, &shader_);
          FAILED(operation_status)) {
        throw GetDebugger().CreateDirectXError(operation_status, 
                                               "Failed to create vertex shader from blob",
                                               "Error during shader loading", 
                                               __FILEW__, __LINE__);
      }
    } else {
      std::unreachable();
    }
  }
  Shader(Shader const&) = default;
  Shader(Shader&&) = default;

  Shader& operator=(Shader const&) = default;
  Shader& operator=(Shader&&) = default;

  ~Shader() = default;

  void Activate() override {
#pragma warning(push)
#pragma warning(disable : 26462)
    constexpr ID3D11ClassInstance* const* kNoInterfaces = nullptr;
#pragma warning(pop)
    if constexpr(T == SupportedShaderType::kPixel) {
      GetDeviceContext().PSSetShader(shader_.Get(), 
                                     kNoInterfaces,
                                     0u);
    }
    else if constexpr(T == SupportedShaderType::kVertex) {
      GetDeviceContext().VSSetShader(shader_.Get(), 
                                     kNoInterfaces, 
                                     0u);
    }
    else {
      std::unreachable();
    }
  }
  
  [[nodiscard]] ID3DBlob& GetByteCode() const noexcept
  requires(T == SupportedShaderType::kVertex)
  {
    return *shader_blob_.Get();
  }

 private:
  Microsoft::WRL::ComPtr<ID3DBlob> shader_blob_;

  using ShaderInterface = decltype([] {
    if constexpr (T == SupportedShaderType::kPixel) {
      return std::type_identity<ID3D11PixelShader>{};
    }
    else if constexpr (T == SupportedShaderType::kVertex) {
      return std::type_identity<ID3D11VertexShader>{};
    }
    else {
      std::unreachable();
    }
  }())::type;
  Microsoft::WRL::ComPtr<ShaderInterface> shader_{};
};

using VertexShader = Shader<SupportedShaderType::kVertex>;
using PixelShader = Shader<SupportedShaderType::kPixel>;
}  // namespace gpu_renderer::bindable

#endif  // !SHADER_HPP
