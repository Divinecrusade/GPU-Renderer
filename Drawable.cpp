#include "Drawable.hpp"

namespace gpu_renderer {
void Drawable::Draw(Graphics& gfx) {
  assert(((void)"Index buffer must be initialised (or added) first",
          indecies_buff_ != nullptr));
  for (auto const& gfx_comp : gfx_comps_) {
    gfx_comp->Bind(gfx);
  }
  gfx.DrawIndexed(indecies_buff_->Count());
}

void Drawable::AddBindable(std::shared_ptr<Bindable> gfx_comp) {
  assert(((void)"Do not add IndexBuffer as a common Bindable. "
                "Use AddIndexBuffer instead",
          typeid(*gfx_comp) != typeid(bindable::IndexBuffer)));
  gfx_comps_.push_back(std::move(gfx_comp));
}

void Drawable::AddIndexBuffer(
    std::shared_ptr<bindable::IndexBuffer> indecies_buff) {
  assert(((void)"Multiple IndexBuffers are not supported",
          !indecies_buff_));
  indecies_buff_ = indecies_buff.get();
  gfx_comps_.push_back(std::move(indecies_buff));
}

}  // namespace gpu_renderer
