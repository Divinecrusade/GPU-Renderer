#include "Drawable.hpp"

namespace gpu_renderer {
void Drawable::Draw(Graphics& gfx) {
  assert(((void)"Index buffer must be initialised (or added) first",
          indecies_buff_ != nullptr));
  for (auto const& gfx_comp : gfx_comps_) {
    gfx_comp->Bind(gfx);
  }
  for (auto const& common_gfx_comp : GetCommonBindables()) {
    common_gfx_comp->Bind(gfx);
  }
  gfx.DrawIndexed(indecies_buff_->Count());
}

void Drawable::AddBindable(std::shared_ptr<Bindable> gfx_comp) {
  assert(((void)"Do not add IndexBuffer as a common Bindable. "
                "Use AddIndexBuffer instead",
          typeid(*gfx_comp) != typeid(bindable::IndexBuffer)));
  gfx_comps_.push_back(std::move(gfx_comp));
}

void Drawable::AddBindable(
    std::shared_ptr<bindable::IndexBuffer> indecies_buff) {
  assert(((void)"Multiple IndexBuffers are not supported",
          !indecies_buff_));
  indecies_buff_ = indecies_buff.get();
  gfx_comps_.push_back(std::move(indecies_buff));
}

void Drawable::UseCommonIndexBuffer() {
  for (auto const& common_gfx_comp : GetCommonBindables()) {
    if (indecies_buff_ =
            dynamic_cast<bindable::IndexBuffer*>(common_gfx_comp.get());
        indecies_buff_) {
      break;
    }
  }
  assert(((void)"There's in common must be at least 1 index buffer",
          indecies_buff_ != nullptr));
  }
}  // namespace gpu_renderer
