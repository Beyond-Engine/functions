#ifndef BEYOND_BASIC_FUNCTION_HPP
#define BEYOND_BASIC_FUNCTION_HPP

#ifndef BEYOND_FUNCTIONS_NAMESPACE
#define BEYOND_FUNCTIONS_NAMESPACE beyond
#endif

#include <functional>
#include <type_traits>

namespace BEYOND_FUNCTIONS_NAMESPACE {

namespace detail {

enum class function_behaviors { move_to, destory };

} // namespace detail

template <typename Storage, typename R, typename... Args>
struct basic_function {
public:
  using result_type = R;

  template <typename Func, class DFunc = std::decay_t<Func>,
            class = std::enable_if_t<!std::is_same_v<DFunc, basic_function> &&
                                     std::is_move_constructible_v<DFunc>>>
  explicit basic_function(Func&& func)
  {
    static_assert(std::is_invocable_r_v<R, DFunc, Args...>);

    storage_.template emplace<DFunc>(std::forward<DFunc>(func));
    behaviors_ =
        Storage::template behaviors<R, Args...>::template dispatch<DFunc>;
    function_ptr_ =
        Storage::template behaviors<R, Args...>::template invoke<DFunc>;
  }

  basic_function(const basic_function&) = delete;
  auto operator=(const basic_function&) & -> basic_function& = delete;

  basic_function(basic_function&& other) noexcept
  {
    if (other) {
      other.behaviors_(detail::function_behaviors::move_to, other, this);
    }
  }

  auto operator=(basic_function&& other) & noexcept -> basic_function&
  {
    if (other) {
      other.behaviors_(detail::function_behaviors::move_to, other, this);
    } else {
      this->reset();
    }
    return *this;
  }

  [[nodiscard]] operator bool() const noexcept // NOLINT
  {
    return behaviors_ != nullptr;
  }

  auto swap(basic_function& other) noexcept -> void
  {
    basic_function temp = std::move(other);
    other = std::move(*this);
    *this = std::move(temp);
  }

protected:
  basic_function() = default;
  ~basic_function()
  {
    this->reset();
  }

  auto invoke(Args... args) const -> R
  {
#ifndef BEYOND_FUNCTIONS_NO_EXCEPTION
    if (*this) {
      return this->function_ptr_(*this, std::forward<Args>(args)...);
    } else {
      throw std::bad_function_call{};
    }
#else
    return this->function_ptr_(*this, std::forward<Args>(args)...);
#endif
  }

private:
  friend Storage;

  void (*behaviors_)(detail::function_behaviors, basic_function&,
                     void*) = nullptr;
  R (*function_ptr_)(const basic_function&, Args&&...) = nullptr;
  Storage storage_;

  void reset()
  {
    if (behaviors_) {
      behaviors_(detail::function_behaviors::destory, *this, nullptr);
    }
    function_ptr_ = nullptr;
    behaviors_ = nullptr;
  }
};

template <typename Storage, typename R, typename... Args>
auto swap(basic_function<Storage, R, Args...>& lhs,
          basic_function<Storage, R, Args...>& rhs) noexcept -> void
{
  lhs.swap(rhs);
}

template <typename Storage, typename R, typename... Args>
auto operator==(const basic_function<Storage, R, Args...>& lhs,
                std::nullptr_t) noexcept -> bool
{
  return !lhs;
}

template <typename Storage, typename R, typename... Args>
auto operator==(std::nullptr_t,
                const basic_function<Storage, R, Args...>& lhs) noexcept -> bool
{
  return !lhs;
}

template <typename Storage, typename R, typename... Args>
auto operator!=(const basic_function<Storage, R, Args...>& lhs,
                std::nullptr_t) noexcept -> bool
{
  return lhs;
}

template <typename Storage, typename R, typename... Args>
auto operator!=(std::nullptr_t,
                const basic_function<Storage, R, Args...>& lhs) noexcept -> bool
{
  return lhs;
}

} // namespace BEYOND_FUNCTIONS_NAMESPACE

#endif // BEYOND_BASIC_FUNCTION_HPP
