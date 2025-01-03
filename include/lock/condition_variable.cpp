#pragma once
#include "lock/condition_variable.hpp"

namespace coro
{
  bool CondVar::CvAwaiter::register_lock() noexcept
  {
    if (cond_ && cond_())
    {
      return false;
    }
    register_cv();
    mtx_.unlock();
    return true;
  }

  void CondVar::CvAwaiter::register_cv() noexcept
  {
    // no need lock
    if (cv_.tail_ == nullptr)
    {
      cv_.head_ = cv_.tail_ = this;
    }
    else
    {
      cv_.tail_->next_ = this;
      cv_.tail_ = this;
    }
  }

  void CondVar::CvAwaiter::wake_up() noexcept
  {
    LockAwaiter::register_lock();
  }

  void CondVar::CvAwaiter::submit_task() noexcept
  {
    if (cond_ && !cond_())
    {
      register_cv();
      mtx_.unlock();
      return;
    }
    LockAwaiter::submit_task();
  }

  void CondVar::notify_one() noexcept
  {
    auto cur = head_;
    if (cur != nullptr)
    {
      head_ = reinterpret_cast<CvAwaiter *>(head_->next_);
      if (head_ == nullptr)
      {
        tail_ = nullptr;
      }
      cur->wake_up();
    }
  }

  void CondVar::notify_all() noexcept
  {
    while (head_ != nullptr)
    {
      head_->wake_up();
      head_ = reinterpret_cast<CvAwaiter *>(head_->next_);
    }
    tail_ = nullptr;
  }

};