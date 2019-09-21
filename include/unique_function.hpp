#pragma once

#ifndef BEYOND_UNIQUE_FUNCTION_HPP
#define BEYOND_UNIQUE_FUNCTION_HPP

#include <functional>
#include <memory>
#include <type_traits>

#include <cassert>

namespace beyond {

template <typename R, typename... Args> class unique_function;

namespace details {

enum class unique_function_behaviors { move_to, trampoline, data, destory };

union unique_function_storage {
  std::aligned_storage_t<32, 8> small_{};
  void* large_;

  template <class T>
  static constexpr bool fit_small = sizeof(T) <= sizeof(small_) &&
                                    alignof(T) <= alignof(decltype(small_)) &&
                                    std::is_nothrow_move_constructible_v<T>;

  constexpr unique_function_storage() noexcept = default;

  template <typename Func, typename... Data> void emplace(Data&&... args)
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
    static void dispatch(unique_function_behaviors behavior,
                         unique_function<R(Args...)>& who, void* ret)
    {
      constexpr static bool fit_sm = fit_small<Func>;
      void* data = fit_sm ? &who.storage_.small_ : who.storage_.large_;

      switch (behavior) {
      case details::unique_function_behaviors::destory:
        if constexpr (fit_sm) {
          static_cast<Func*>(data)->~Func();
        } else {
          delete static_cast<Func*>(who.storage_.large_);
        }
        break;
      case details::unique_function_behaviors::move_to: {
        auto* func = static_cast<unique_function<R(Args...)>*>(ret);
        func->reset();
        func->storage_.template emplace<Func>(
            std::move(*static_cast<Func*>(data)));
        func->behaviors_ = behaviors<R, Args...>::template dispatch<Func>;
        who.reset();
      } break;
      case details::unique_function_behaviors::data:
        *static_cast<void**>(ret) = data;
        break;
      case details::unique_function_behaviors::trampoline:
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

} // namespace details

template <typename R, typename... Args> class unique_function<R(Args...)> {
public:
  /// @brief Default constructor
  unique_function() = default;

  /// @brief Construct a unique_function from an invokable
  template <typename Func, class DFunc = std::decay_t<Func>,
            class = std::enable_if_t<!std::is_same_v<DFunc, unique_function> &&
                                     std::is_move_constructible_v<DFunc>>>
  explicit unique_function(Func&& func)
  {
    static_assert(std::is_invocable_r_v<R, DFunc, Args...>);

    storage_.emplace<Func>(std::forward<Func>(func));
    behaviors_ = details::unique_function_storage::behaviors<
        R, Args...>::template dispatch<Func>;
  }

  unique_function(const unique_function&) = delete;
  unique_function& operator=(const unique_function&) = delete;

  /// @brief Move constructor
  unique_function(unique_function&& other) noexcept
  {
    if (other) {
      other.behaviors_(details::unique_function_behaviors::move_to, other,
                       this);
    }
  }

  /// @brief Move assignment
  unique_function& operator=(unique_function&& other) noexcept
  {
    if (other) {
      other.behaviors_(details::unique_function_behaviors::move_to, other,
                       this);
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

private:
  friend union details::unique_function_storage;

  void (*behaviors_)(details::unique_function_behaviors, unique_function&,
                     void*) = nullptr;
  details::unique_function_storage storage_;

  auto reset()
  {
    if (behaviors_) {
      behaviors_(details::unique_function_behaviors::destory, *this, nullptr);
    }
    behaviors_ = nullptr;
  }

  auto trampoline()
  {
    using PlainFunction = R(void*, Args&&...);
    PlainFunction* result = nullptr;

    assert(behaviors_);
    behaviors_(details::unique_function_behaviors::trampoline, *this, &result);
    return result;
  }

  auto data() noexcept -> void*
  {
    void* result = nullptr;

    assert(behaviors_);

    behaviors_(details::unique_function_behaviors::data, *this, &result);
    return result;
  }
};

} // namespace beyond

#endif // BEYOND_UNIQUE_FUNCTION_HPP
