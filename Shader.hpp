#ifndef SHADER_HPP
#define SHADER_HPP

#include "OptimisedStlHeader.hpp"

#include "Bindable.hpp"
#include "SupportedShaderType.hpp"

namespace gpu_renderer::bindable {
template <SupportedShaderType T>
class Shader : public abstract::Bindable {
 public:
  Shader() = delete;
  Shader(std::filesystem::path const& shader_file, Graphics& gfx)
    : Bindable{gfx} {
    // StartTraceInDebugMode();
    // if (HRESULT const operation_status =
    //         D3DReadFileToBlob(L"PixelShader.cso", &shader_blob);
    //     FAILED(operation_status)) {
    //   throw CreateDirectXError(
    //       operation_status, "Failed to read compiled pixel shader into blob",
    //       "Error during shader loading", __FILEW__, __LINE__);
    // }
    assert(shader_file.extension() == L".cso");
    assert(SUCCEEDED(D3DReadFileToBlob(shader_file.c_str(), &shader_blob_)));

#pragma warning(push)
#pragma warning(disable : 26462)
    constexpr ID3D11ClassLinkage* kNoClassLinkage = nullptr;
#pragma warning(pop)
    if constexpr (T == SupportedShaderType::kPixel) {
      // StartTraceInDebugMode();
      // if (HRESULT const operation_status = device_->CreatePixelShader(
      //         shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(),
      //         NULL, &pixel_shader);
      //     FAILED(operation_status)) {
      //   throw CreateDirectXError(
      //       operation_status, "Failed to create pixel shader from blob",
      //       "Error during shader loading", __FILEW__, __LINE__);
      // }
      assert(SUCCEEDED(GetDevice().CreatePixelShader(
          shader_blob_->GetBufferPointer(), shader_blob_->GetBufferSize(),
          kNoClassLinkage, &shader_)));
    } else if constexpr (T == SupportedShaderType::kVertex) {
      // if (HRESULT const operation_status = device_->CreateVertexShader(
      //         shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(),
      //         NULL, &vertex_shader);
      //     FAILED(operation_status)) {
      //   throw CreateDirectXError(
      //       operation_status, "Failed to create vertex shader from blob",
      //       "Error during shader loading", __FILEW__, __LINE__);
      // }
      assert(SUCCEEDED(GetDevice().CreateVertexShader(
          shader_blob_->GetBufferPointer(), shader_blob_->GetBufferSize(),
          kNoClassLinkage, &shader_)));
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
}  // namespace gpu_renderer::bindable

#endif  // !SHADER_HPP
