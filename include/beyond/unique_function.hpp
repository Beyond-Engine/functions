#ifndef BEYOND_UNIQUE_FUNCTION_HPP
#define BEYOND_UNIQUE_FUNCTION_HPP

#include "basic_function.hpp"

namespace BEYOND_FUNCTIONS_NAMESPACE {

template <typename Signature> class unique_function;
template <typename R, typename... Args> class unique_function_base;

template <std::size_t small_size> union unique_function_storage {
  void* large = nullptr;
  std::byte small[small_size];

  template <class T>
  static constexpr bool
      fit_small = sizeof(T) <= sizeof(small) && small_size % alignof(T) == 0 &&
                  std::is_nothrow_move_constructible_v<T>;

  unique_function_storage() = default;

  template <typename Func, typename... A> auto emplace(A&&... args)
  {
    if constexpr (fit_small<Func>) {
      ::new (static_cast<void*>(&small)) Func(std::forward<A>(args)...);
    } else {
      large = new Func(std::forward<A>(args)...);
    }
  }

  template <typename R, typename... Args> struct behaviors {
    R(*invoke_ptr)
    (unique_function_storage& storage, Args&&... args) = nullptr;
    void (*destroy_ptr)(unique_function_storage& storage) = nullptr;
    void (*move_ptr)(unique_function_storage& from,
                     unique_function_storage& to) = nullptr;

    template <typename Func> static auto get_instance()
    {
      constexpr static behaviors b{
          //.invoke_ptr =
          +[](unique_function_storage& storage, Args&&... args) {
            const auto data = fit_small<Func>
                                  ? static_cast<void*>(&storage.small)
                                  : storage.large;

            return (*static_cast<Func*>(data))(std::forward<Args>(args)...);
          },
          //.destroy_ptr =
          +[](unique_function_storage& storage) {
            if constexpr (fit_small<Func>) {
              static_cast<Func*>(static_cast<void*>(&storage.small))->~Func();
            } else {
              delete static_cast<Func*>(storage.large);
            }
          },
          //.move_ptr =
          +[](unique_function_storage& from, unique_function_storage& to) {
            if constexpr (fit_small<Func>) {
              ::new (static_cast<void*>(&to.small))
                  Func(reinterpret_cast<Func&&>(from.small));
            } else {
              to.large = std::exchange(from.large, {});
            }
          }};
      return &b;
    }
  };
};

template <typename R, typename... Args> class unique_function_base {
  static constexpr std::size_t small_size = 32;

  unique_function_storage<small_size> storage_;
  const typename unique_function_storage<small_size>::template behaviors<
      R, Args...>* behaviors_ = nullptr;

protected:
  constexpr unique_function_base() noexcept = default;

  template <
      typename Func, typename DFunc = std::decay_t<Func>,
      class = std::enable_if_t<!std::is_same_v<DFunc, unique_function_base> &&
                               std::is_move_constructible_v<DFunc>>>
  unique_function_base(Func&& func)
  {
    storage_.template emplace<DFunc>(std::forward<DFunc>(func));
    behaviors_ = unique_function_storage<small_size>::template behaviors<
        R, Args...>::template get_instance<Func>();
  }

  auto invoke(Args&&... args) const -> R
  {
    if (behaviors_ != nullptr) {
      return behaviors_->invoke_ptr(
          const_cast<unique_function_storage<small_size>&>(this->storage_),
          std::forward<Args>(args)...);
    }
    throw std::bad_function_call{};
  }

  auto reset()
  {
    if (behaviors_) {
      behaviors_->destroy_ptr(storage_);
    }
    behaviors_ = {};
  }

public:
  unique_function_base(const unique_function_base&) = delete;
  auto
  operator=(const unique_function_base&) & -> unique_function_base& = delete;

  unique_function_base(unique_function_base&& other) noexcept
  {
    if (other) {
      other.behaviors_->move_ptr(other.storage_, storage_);
      behaviors_ = std::exchange(other.behaviors_, {});
    }
  }

  ~unique_function_base()
  {
    reset();
  }

  auto operator=(unique_function_base&& rhs) & noexcept -> unique_function_base&
  {
    if (this != &rhs) {
      reset();
      if (rhs) {
        rhs.behaviors_->move_ptr(rhs.storage_, storage_);
        behaviors_ = std::exchange(rhs.behaviors_, {});
      }
    }
    return *this;
  }

  auto swap(unique_function_base& rhs) noexcept -> void
  {
    using std::swap;
    swap(storage_, rhs.storage_);
    swap(behaviors_, rhs.behaviors_);
  }

  explicit operator bool() const noexcept
  {
    return behaviors_ != nullptr;
  }

  friend auto operator==(const unique_function_base& f, std::nullptr_t) noexcept
      -> bool
  {
    return !f;
  }

  friend auto operator==(std::nullptr_t, const unique_function_base& f) noexcept
      -> bool
  {
    return !f;
  }

  friend auto operator!=(const unique_function_base& f, std::nullptr_t) noexcept
      -> bool
  {
    return bool(f);
  }

  friend auto operator!=(std::nullptr_t, const unique_function_base& f) noexcept
      -> bool
  {
    return bool(f);
  }

  friend auto swap(unique_function_base& lhs,
                   unique_function_base& rhs) noexcept -> void
  {
    lhs.swap(rhs);
  }
};

template <typename R, typename... Args>
class unique_function<R(Args...)> : public unique_function_base<R, Args...> {
public:
  using base_type = unique_function_base<R, Args...>;

  constexpr unique_function() = default;
  explicit constexpr unique_function(std::nullptr_t) noexcept {}

  template <typename Func, class DFunc = std::decay_t<Func>,
            class = std::enable_if_t<!std::is_same_v<DFunc, unique_function> &&
                                     std::is_move_constructible_v<DFunc>>>
  explicit unique_function(Func&& func) : base_type{std::forward<DFunc>(func)}
  {
  }

  /*implicit*/ unique_function(unique_function<R(Args...) const>&& other)
      : base_type{static_cast<base_type&&>(other)}
  {
  }

  auto operator()(Args&&... args) -> R
  {
    return this->invoke(std::forward<Args>(args)...);
  }
};

template <typename R, typename... Args>
class unique_function<R(Args...) const>
    : public unique_function_base<R, Args...> {
public:
  using base_type = unique_function_base<R, Args...>;

  constexpr unique_function() = default;
  explicit constexpr unique_function(std::nullptr_t) noexcept {}

  template <
      typename Func, typename DFunc = std::decay_t<Func>,
      typename = std::enable_if_t<!std::is_same_v<DFunc, unique_function> &&
                                  std::is_move_constructible_v<DFunc>>,
      typename = std::void_t<
          decltype(std::declval<const Func&>()(std::declval<Args>()...))>>
  explicit unique_function(Func&& func) : base_type{std::forward<DFunc>(func)}
  {
  }

  auto operator()(Args&&... args) const -> R
  {
    return this->invoke(std::forward<Args>(args)...);
  }
};

// deduction guides
template <class R, typename... Args>
unique_function(R (*)(Args...))->unique_function<R(Args...) const>;

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

} // namespace BEYOND_FUNCTIONS_NAMESPACE

#undef BEYOND_FUNCTIONS_NAMESPACE

#endif // BEYOND_UNIQUE_FUNCTION_HPP
