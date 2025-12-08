#ifndef SHADER_HPP
#define SHADER_HPP

#include "Bindable.hpp"
#include "SupportedShaderType.hpp"

namespace gpu_renderer::bindable {
template <SupportedShaderType T>
class Shader : public abstract::Bindable {
 public:
  std::filesystem::path kValidFileExtension{L".cso"};

 public:
  Shader() = delete;
  Shader(std::filesystem::path const& shader_file) {
    // StartTraceInDebugMode();
    // if (HRESULT const operation_status =
    //         D3DReadFileToBlob(L"PixelShader.cso", &shader_blob);
    //     FAILED(operation_status)) {
    //   throw CreateDirectXError(
    //       operation_status, "Failed to read compiled pixel shader into blob",
    //       "Error during shader loading", __FILEW__, __LINE__);
    // }

    if (shader_file.extension() != kValidFileExtension) {
      throw std::invalid_argument{
          "Shader source file has wrong extension. Probably it is not valid"};
    }
    assert(SUCCEEDED(D3DReadFileToBlob(shader_file.c_str(), &shader_blob_)));
  }
  Shader(Shader const&) = default;
  Shader(Shader&&) = default;

  Shader& operator=(Shader const&) = default;
  Shader& operator=(Shader&&) = default;

  ~Shader() = default;

  void Bind(Graphics& gfx) override {
#pragma warning(push)
#pragma warning(disable : 26462)
    constexpr ID3D11ClassInstance* const* kNoInterfaces = nullptr;
    constexpr ID3D11ClassLinkage* kNoClassLinkage = nullptr;
#pragma warning(pop)

    if constexpr(T == SupportedShaderType::kPixel) {
      // StartTraceInDebugMode();
      // if (HRESULT const operation_status = device_->CreatePixelShader(
      //         shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(),
      //         NULL, &pixel_shader);
      //     FAILED(operation_status)) {
      //   throw CreateDirectXError(
      //       operation_status, "Failed to create pixel shader from blob",
      //       "Error during shader loading", __FILEW__, __LINE__);
      // }
      Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader{};

      assert(SUCCEEDED(GetDevice(gfx).CreatePixelShader(
          shader_blob_->GetBufferPointer(), shader_blob_->GetBufferSize(),
          kNoClassLinkage, &pixel_shader)));

      GetDeviceContext(gfx).PSSetShader(pixel_shader.Get(), kNoInterfaces,
                                        0u);
    }
    else if constexpr(T == SupportedShaderType::kVertex) {
      //if (HRESULT const operation_status = device_->CreateVertexShader(
      //        shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(),
      //        NULL, &vertex_shader);
      //    FAILED(operation_status)) {
      //  throw CreateDirectXError(
      //      operation_status, "Failed to create vertex shader from blob",
      //      "Error during shader loading", __FILEW__, __LINE__);
      //}
      //device_context_->VSSetShader(vertex_shader.Get(), kNoInterfacesForShader,
      //                             0);
      Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader{};

      assert(SUCCEEDED(GetDevice(gfx).CreateVertexShader(
          shader_blob_->GetBufferPointer(), shader_blob_->GetBufferSize(),
          kNoClassLinkage, &vertex_shader)));

      GetDeviceContext(gfx).VSSetShader(vertex_shader.Get(), kNoInterfaces, 
                                        0u);
    }
    else {
      std::unreachable();
    }
  }
  
  [[nodiscard]] ID3DBlob& GetByteCode() const noexcept {
    return *shader_blob_.Get();
  }

 private:
  Microsoft::WRL::ComPtr<ID3DBlob> shader_blob_;
};
}  // namespace gpu_renderer::bindable

#endif  // !SHADER_HPP
