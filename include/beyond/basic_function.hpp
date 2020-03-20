#ifndef BEYOND_BASIC_FUNCTION_HPP
#define BEYOND_BASIC_FUNCTION_HPP

#ifndef BEYOND_FUNCTIONS_NAMESPACE
#define BEYOND_FUNCTIONS_NAMESPACE beyond
#endif

#include <functional>
#include <type_traits>

namespace BEYOND_FUNCTIONS_NAMESPACE {

//namespace detail {
//
//enum class function_behaviors { move_to, destory };
//
//} // namespace detail
//
//template <typename Storage, typename R, typename... Args> class basic_function {
//  Storage storage_;
//  const typename Storage::template behaviors<R, Args...>* behaviors_ = nullptr;
//
//public:
//  using result_type = R;
//
//  template <typename Func, class DFunc = std::decay_t<Func>,
//            class = std::enable_if_t<!std::is_same_v<DFunc, basic_function> &&
//                                     std::is_move_constructible_v<DFunc>>>
//  explicit basic_function(Func&& func)
//  {
//    static_assert(std::is_invocable_r_v<R, DFunc, Args...>);
//
//    storage_.template emplace<DFunc>(std::forward<DFunc>(func));
//    behaviors_ =
//        Storage::template behaviors<R, Args...>::template get_instance<Func>();
//  }
//
//  basic_function(const basic_function&) = delete;
//  auto operator=(const basic_function&) & -> basic_function& = delete;
//
//  basic_function(basic_function&& other) noexcept
//  {
//    if (other) {
//      other.behaviors_->move_ptr(other.storage_, storage_);
//      behaviors_ = std::exchange(other.behaviors_, {});
//    }
//  }
//
//  auto operator=(basic_function&& rhs) & noexcept -> basic_function&
//  {
//    if (this != &rhs) {
//      reset();
//      if (rhs) {
//        rhs.behaviors_->move_ptr(rhs.storage_, storage_);
//        behaviors_ = std::exchange(rhs.behaviors_, {});
//      }
//      return *this;
//    }
//    return *this;
//  }
//
//  [[nodiscard]] operator bool() const noexcept // NOLINT
//  {
//    return behaviors_ != nullptr;
//  }
//
//  auto swap(basic_function& rhs) noexcept -> void
//  {
//    using std::swap;
//    swap(storage_, rhs.storage_);
//    swap(behaviors_, rhs.behaviors_);
//  }
//
//protected:
//  basic_function() = default;
//  ~basic_function()
//  {
//    reset();
//  }
//
//  auto invoke(Args... args) const -> R
//  {
//#ifndef BEYOND_FUNCTIONS_NO_EXCEPTION
//    if (behaviors_ != nullptr) {
//      return behaviors_->invoke_ptr(const_cast<Storage&>(this->storage_),
//                                    std::forward<Args>(args)...);
//    }
//    throw std::bad_function_call{};
//#else
//    return behaviors_->invoke_ptr(const_cast<Storage&>(this->storage_),
//                                  std::forward<Args>(args)...);
//#endif
//  }
//
//private:
//  void reset()
//  {
//    if (behaviors_) {
//      behaviors_->destroy_ptr(storage_);
//    }
//    behaviors_ = {};
//  }
//};
//
//template <typename Storage, typename R, typename... Args>
//auto swap(basic_function<Storage, R, Args...>& lhs,
//          basic_function<Storage, R, Args...>& rhs) noexcept -> void
//{
//  lhs.swap(rhs);
//}
//
//template <typename Storage, typename R, typename... Args>
//auto operator==(const basic_function<Storage, R, Args...>& lhs,
//                std::nullptr_t) noexcept -> bool
//{
//  return !lhs;
//}
//
//template <typename Storage, typename R, typename... Args>
//auto operator==(std::nullptr_t,
//                const basic_function<Storage, R, Args...>& lhs) noexcept -> bool
//{
//  return !lhs;
//}
//
//template <typename Storage, typename R, typename... Args>
//auto operator!=(const basic_function<Storage, R, Args...>& lhs,
//                std::nullptr_t) noexcept -> bool
//{
//  return lhs;
//}
//
//template <typename Storage, typename R, typename... Args>
//auto operator!=(std::nullptr_t,
//                const basic_function<Storage, R, Args...>& lhs) noexcept -> bool
//{
//  return lhs;
//}

} // namespace BEYOND_FUNCTIONS_NAMESPACE

#endif // BEYOND_BASIC_FUNCTION_HPP
