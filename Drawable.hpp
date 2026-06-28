#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP

#include "Bindable.hpp"
#include "FrameTimer.hpp"
#include "Graphics.hpp"
#include "IndexBuffer.hpp"

namespace gpu_renderer {

template <typename T>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template <typename T>
concept BindablePtr =
    is_shared_ptr<std::decay_t<T>>::value &&
    std::derived_from<typename std::decay_t<T>::element_type, Bindable>;

class Drawable {
 public:
  Drawable() = default;
  Drawable(Drawable const&) = default;
  Drawable(Drawable&&) = default;
  template <BindablePtr... Binds>
  Drawable(std::shared_ptr<bindable::IndexBuffer> indecies_buff,
           Binds&&... gfx_comps)
      : indecies_buff_{indecies_buff.get()},
        gfx_comps_{std::move(indices_buff), std::forward<Binds>(gfx_comps)...} {
  }

  Drawable& operator=(Drawable const&) = default;
  Drawable& operator=(Drawable&&) = default;

  virtual ~Drawable() = default;

  void Draw(Graphics& gfx);

  virtual void Update(FrameTimer::DeltaTime dt) = 0;
  virtual DirectX::XMMATRIX GetTransformation() const = 0;

 protected:
  void AddBindable (std::shared_ptr<Bindable> gfx_comp);
  void AddIndexBuffer(std::shared_ptr<bindable::IndexBuffer> indecies_buff);

 private:
  bindable::IndexBuffer const* indecies_buff_ = nullptr;
  std::vector<std::shared_ptr<Bindable>> gfx_comps_;
};
}  // namespace gpu_renderer

#endif  // !DRAWABLE_HPP
