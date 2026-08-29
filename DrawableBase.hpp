#ifndef DRAWABLE_BASE_HPP
#define DRAWABLE_BASE_HPP

#include "Drawable.hpp"

namespace gpu_renderer {
template<typename T>
class DrawableBase : public Drawable {
 protected:
  bool IsCommonInitialized() const noexcept
  {
    return !common_bindables_.empty();
  }

  void AddCommonBindable(std::shared_ptr<Bindable> gfx_comp) noexcept
  {
    common_bindables_.push_back(std::move(gfx_comp));
  }

 private:
  gsl::span<std::shared_ptr<Bindable> const> GetCommonBindables() const override
  {
    return common_bindables_;
  }

 private:
  static std::vector<std::shared_ptr<Bindable>> common_bindables_;
};

template<typename T>
std::vector<std::shared_ptr<Bindable>> DrawableBase<T>::common_bindables_{};
}

#endif
