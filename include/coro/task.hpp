#pragma once

#include <coroutine>
#include <optional>
#include <cassert>

namespace coro
{
  using std::addressof;
  using std::coroutine_handle;
  using std::exception_ptr;
  using std::move;
  using std::noop_coroutine;
  using std::nullopt;
  using std::optional;

  template <typename T>
  class Task;

  template <typename T>
  class PromiseBase;

  template <typename T>
  class Promise;

  template <typename T>
  class AwaiterBase
  {
  public:
    explicit AwaiterBase(coroutine_handle<Promise<T>> handle) noexcept : handle_(handle) {}

    inline bool await_ready() const noexcept
    {
      return !handle_ || handle_.done();
    }

    std::coroutine_handle<>
    await_suspend(coroutine_handle<> pare_coro) noexcept
    {
      handle_.promise().set_parent(pare_coro);
      return handle_;
    }

  protected:
    coroutine_handle<Promise<T>> handle_;
  };

  template <typename T>
  class FinalAwaiter
  {
  public:
    inline constexpr bool await_ready() noexcept { return false; }
    coroutine_handle<> await_suspend(coroutine_handle<Promise<T>> handle) noexcept
    {
      return handle.promise().pare_coro_;
    }
    inline constexpr void await_resume() const noexcept {}
  };

  // TODO: maybe different from no-void
  // template <>
  // class FinalAwaiter<void>
  // {
  // public:
  //   inline constexpr bool await_ready() noexcept { return false; }
  //   coroutine_handle<> await_suspend(coroutine_handle<Promise<void>> handle) noexcept
  //   {
  //     return handle.promise().pare_coro_;
  //   }
  //   inline constexpr void await_resume() const noexcept {}
  // };

  template <typename T>
  class PromiseBase
  {
  public:
    friend FinalAwaiter<T>;

  public:
    PromiseBase(const PromiseBase &) = delete;
    PromiseBase(PromiseBase &&) = delete;
    PromiseBase &operator=(const PromiseBase &) = delete;
    PromiseBase &operator=(PromiseBase &&) = delete;

  public:
    inline constexpr std::suspend_always initial_suspend() noexcept
    {
      return {};
    }

    inline constexpr FinalAwaiter<T> final_suspend() noexcept
    {
      return {};
    }

    void unhandled_exception() noexcept
    {
      // TODO: process exception
      assert(false); // unreachable
    }

    inline void set_pare(std::coroutine_handle<> pare_coro) noexcept
    {
      pare_coro_ = pare_coro;
    }

  private:
    coroutine_handle<> pare_coro_;
    [[__attribute_maybe_unused__]] exception_ptr exception_{nullptr};
  };

  template <typename T>
  class Promise final : public PromiseBase<T>
  {
  public:
    Task<T> get_return_object() noexcept
    {
      return Task<T>{coroutine_handle<Promise>::from_promise(*this)};
    }

    // TODO: optimize para type
    inline void return_value(T &value)
    {
      value_ = value;
    }

    inline T &get_result() & noexcept
    {
      return value_;
    }

    inline T &&get_result() && noexcept
    {
      return move(value_);
    }

  private:
    optional<T> value_{nullopt};
  };

  template <>
  class Promise<void> final : public PromiseBase<void>
  {
  public:
    friend FinalAwaiter<void>;
    friend Task<void>;

  public:
    Task<void> get_return_object() noexcept
    {
      return Task<void>{
          std::coroutine_handle<Promise>::from_promise(*this)};
    }

    inline constexpr void return_void() noexcept {}

    void get_result() noexcept
    {
      // TODO: add check
    }
  };

  template <typename T = void>
  class Task
  {
  public:
    using promise_type = Promise<T>;

  public:
    Task() noexcept = default;
    explicit Task(coroutine_handle<promise_type> handle) noexcept : handle_(handle) {}

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    Task(Task &&other) noexcept : handle_(other.handle_)
    {
      other.handle_ = nullptr;
    }

    Task &operator=(Task &&other) noexcept
    {
      if (this == addressof(other))
      {
        return;
      }

      if (handle_)
      {
        handle_.destroy();
        handle_ = other.handle_;
        other.handle_ = nullptr;
      }
    }

    ~Task() noexcept
    {
      if (handle_)
      {
        handle_.destroy();
      }
    }

    inline bool is_ready() const noexcept
    {
      !handle || handle.done();
    }

    auto operator co_await() const & noexcept
    {
      class Awaiter : public AwaiterBase
      {
      public:
        auto await_resume() noexcept
        {
          return handle_.promise().result();
        }
      };
      return Awaiter{handle_};
    }

    auto operator co_await() const && noexcept
    {
      class Awaiter : public AwaiterBase
      {
      public:
        auto await_resume() noexcept
        {
          return move(handle_.promise()).result();
        }
      };
      return Awaiter{handle_};
    }

  private:
    coroutine_handle<promise_type> handle_;
  };

};
