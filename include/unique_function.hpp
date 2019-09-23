#pragma once

#ifndef BEYOND_UNIQUE_FUNCTION_HPP
#define BEYOND_UNIQUE_FUNCTION_HPP

#include <functional>
#include <memory>
#include <type_traits>

#include <cassert>

namespace beyond {

template <typename R, typename... Args> class unique_function;

namespace detail {

enum class unique_function_behaviors { move_to, trampoline, data, destory };

union unique_function_storage {
  std::aligned_storage_t<32, 8> small_{};
  void* large_;

  template <class T>
  static constexpr bool fit_small = sizeof(T) <= sizeof(small_) &&
                                    alignof(T) <= alignof(decltype(small_)) &&
                                    std::is_nothrow_move_constructible_v<T>;

  constexpr unique_function_storage() noexcept = default;

  template <typename Func, typename... Data>
  auto emplace(Data&&... args) -> void
  {
    if constexpr (fit_small<Func>) {
      ::new (reinterpret_cast<void*>(&small_))
          Func(std::forward<Data>(args)...);
    } else {
      large_ = new Func(std::forward<Data>(args)...);
    }
  }

  template <typename R, typename... Args> struct behaviors {
    template <typename Func>
    static auto dispatch(unique_function_behaviors behavior,
                         unique_function<R(Args...)>& who, void* ret) -> void
    {
      constexpr static bool fit_sm = fit_small<Func>;
      void* data = fit_sm ? &who.storage_.small_ : who.storage_.large_;

      switch (behavior) {
      case detail::unique_function_behaviors::destory:
        if constexpr (fit_sm) {
          static_cast<Func*>(data)->~Func();
        } else {
          delete static_cast<Func*>(who.storage_.large_);
        }
        break;
      case detail::unique_function_behaviors::move_to: {
        auto* func = static_cast<unique_function<R(Args...)>*>(ret);
        func->reset();
        func->storage_.template emplace<Func>(
            std::move(*static_cast<Func*>(data)));
        func->behaviors_ = behaviors<R, Args...>::template dispatch<Func>;
        who.reset();
      } break;
      case detail::unique_function_behaviors::data:
        *static_cast<void**>(ret) = data;
        break;
      case detail::unique_function_behaviors::trampoline:
        using PlainFunction = R(void*, Args&&...);
        auto trampoline = [](void* func, Args&&... args) -> R {
          return (*static_cast<Func*>(func))(std::forward<Args>(args)...);
        };
        *static_cast<PlainFunction**>(ret) = trampoline;
        break;
      }
    }
  };
};

} // namespace detail

template <typename R, typename... Args> class unique_function<R(Args...)> {
public:
  using result_type = R;

  /// @brief Default constructor
  unique_function() = default;

  /// @brief Construct a unique_function from an invokable
  template <typename Func, class DFunc = std::decay_t<Func>,
            class = std::enable_if_t<!std::is_same_v<DFunc, unique_function> &&
                                     std::is_move_constructible_v<DFunc>>>
  explicit unique_function(Func&& func)
  {
    static_assert(std::is_invocable_r_v<R, DFunc, Args...>);

    storage_.emplace<DFunc>(std::forward<DFunc>(func));
    behaviors_ = detail::unique_function_storage::behaviors<
        R, Args...>::template dispatch<DFunc>;
  }

  unique_function(const unique_function&) = delete;
  auto operator=(const unique_function&) -> unique_function& = delete;

  /// @brief Move constructor
  unique_function(unique_function&& other) noexcept
  {
    if (other) {
      other.behaviors_(detail::unique_function_behaviors::move_to, other, this);
    }
  }

  /// @brief Move assignment
  auto operator=(unique_function&& other) noexcept -> unique_function&
  {
    if (other) {
      other.behaviors_(detail::unique_function_behaviors::move_to, other, this);
    } else {
      reset();
    }
    return *this;
  }

  /// @brief Checks if a valid target is contained
  [[nodiscard]] operator bool() const noexcept
  {
    return behaviors_ != nullptr;
  }

  /// @brief invokes the target
  /// @throw std::bad_function_call if `*this` does not store a callable
  /// function target, i.e. `*this == false`
  auto operator()(Args... args) -> R
  {
    if (*this) {
      const auto trampoline = this->trampoline();
      void* func = this->data();
      return trampoline(func, std::forward<Args>(args)...);
    } else {
      throw std::bad_function_call{};
    }
  }

  /// @brief Swap another unique_function with `this`
  auto swap(unique_function& other) noexcept -> void
  {
    unique_function temp = std::move(other);
    other = std::move(*this);
    *this = std::move(temp);
  }

private:
  friend union detail::unique_function_storage;

  void (*behaviors_)(detail::unique_function_behaviors, unique_function&,
                     void*) = nullptr;
  detail::unique_function_storage storage_;

  auto reset()
  {
    if (behaviors_) {
      behaviors_(detail::unique_function_behaviors::destory, *this, nullptr);
    }
    behaviors_ = nullptr;
  }

  auto trampoline()
  {
    using PlainFunction = R(void*, Args&&...);
    PlainFunction* result = nullptr;

    assert(behaviors_);
    behaviors_(detail::unique_function_behaviors::trampoline, *this, &result);
    return result;
  }

  auto data() noexcept -> void*
  {
    void* result = nullptr;

    assert(behaviors_);

    behaviors_(detail::unique_function_behaviors::data, *this, &result);
    return result;
  }
};

/// @brief Exchanges the state of lhs with that of rhs. Effectively calls
/// `lhs.swap(rhs)`.
/// @related unique_function::swap
template <class R, class... Args>
auto swap(unique_function<R(Args...)>& lhs,
          unique_function<R(Args...)>& rhs) noexcept -> void
{
  lhs.swap(rhs);
}

template <class R, class... Args>
auto operator==(const unique_function<R(Args...)>& lhs, std::nullptr_t) noexcept
    -> bool
{
  return !lhs;
}

template <class R, class... Args>
auto operator==(std::nullptr_t, const unique_function<R(Args...)>& lhs) noexcept
    -> bool
{
  return !lhs;
}

template <class R, class... Args>
auto operator!=(const unique_function<R(Args...)>& lhs, std::nullptr_t) noexcept
    -> bool
{
  return lhs;
}

template <class R, class... Args>
auto operator!=(std::nullptr_t, const unique_function<R(Args...)>& lhs) noexcept
    -> bool
{
  return lhs;
}

// deduction guides
template <class R, typename... Args>
unique_function(R (*)(Args...))->unique_function<R(Args...)>;

namespace detail {

// TODO: Support member functions that take this by reference
template <typename T> struct member_function_pointer_trait {
};

#define BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(CV_OPT, NOEXCEPT_OPT)             \
  template <typename R, typename U, typename... Args>                          \
  struct member_function_pointer_trait<R (U::*)(Args...)                       \
                                           CV_OPT NOEXCEPT_OPT> {              \
    using return_type = R;                                                     \
    using guide_type = R(Args...);                                             \
  };

#define BEYOND_NOARG
BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(BEYOND_NOARG, BEYOND_NOARG)
BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(const, BEYOND_NOARG)
BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(const volatile, BEYOND_NOARG)
BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(volatile, BEYOND_NOARG)

BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(BEYOND_NOARG, noexcept)
BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(const, noexcept)
BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(const volatile, noexcept)
BEYOND_MEMBER_FUNCTION_POINTER_TRAIT(volatile, noexcept)

#undef BEYOND_NOARG
#undef BEYOND_MEMBER_FUNCTION_POINTER_TRAIT

// Main template: cannot find &Func::operator()
template <typename Func, typename = void>
struct function_deduce_signature_impl {
};

template <typename Func>
struct function_deduce_signature_impl<
    Func, std::void_t<decltype(&Func::operator())>> {
  using type = member_function_pointer_trait<decltype(&Func::operator())>;
};

template <typename Func>
struct function_deduce_signature
    : function_deduce_signature_impl<std::remove_cv_t<Func>> {
};

} // namespace detail

template <class Func, class = std::enable_if_t<!std::is_pointer_v<Func>>>
unique_function(Func)
    ->unique_function<
        typename detail::function_deduce_signature<Func>::type::guide_type>;

} // namespace beyond

#endif // BEYOND_UNIQUE_FUNCTION_HPP
