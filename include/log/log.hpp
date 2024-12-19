#include <iostream>
#include <memory>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "config.hpp"

namespace coro::log
{
#define CONFIG_LOG_LEVEL(log_level) spdlog::level::log_level

  using std::make_shared;
  using std::shared_ptr;
  using spdlogger = shared_ptr<spdlog::logger>;

  class Logger
  {
  public:
    static spdlogger &get_logger() noexcept
    {
      static Logger log;
      return log.logger_;
    };

    Logger(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger &operator=(Logger &&) = delete;

  private:
    Logger() noexcept
    {
      logger_ = spdlog::create<spdlog::sinks::basic_file_sink_mt>("corolog",
                                                                  config::kLogFileName, false);
      logger_->set_pattern("[%n][%Y-%m-%d %H:%M:%S.%e] [%l] [%t]  %v");
      logger_->set_level(CONFIG_LOG_LEVEL(LOG_LEVEL));
      spdlog::flush_every(std::chrono::seconds(5));
    }

    ~Logger() noexcept
    {
      logger_->flush_on(CONFIG_LOG_LEVEL(LOG_LEVEL));
    }

  private:
    spdlogger logger_;
  };

  // TODO: can you switch to use macro to avoid duplicate function calls?
  template <typename... T>
  void trace(const char *__restrict__ fmt, const T &...args)
  {
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::trace)
    {
#ifdef LOGTOFILE
      Logger::get_logger()->trace(spdlog::fmt_runtime_string<char>{fmt}, args...);
#else
      spdlog::trace(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
    }
  }

  template <typename... T>
  void debug(const char *__restrict__ fmt, const T &...args)
  {
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::debug)
    {
#ifdef LOGTOFILE
      Logger::get_logger()->debug(spdlog::fmt_runtime_string<char>{fmt}, args...);
#else
      spdlog::debug(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
    }
  }

  template <typename... T>
  void info(const char *__restrict__ fmt, const T &...args)
  {
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::info)
    {
#ifdef LOGTOFILE
      Logger::get_logger()->info(spdlog::fmt_runtime_string<char>{fmt}, args...);
#else
      spdlog::info(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
    }
  }

  template <typename... T>
  void warn(const char *__restrict__ fmt, const T &...args)
  {
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::warn)
    {
#ifdef LOGTOFILE
      Logger::get_logger()->warn(spdlog::fmt_runtime_string<char>{fmt}, args...);
#else
      spdlog::warn(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
    }
  }

  template <typename... T>
  void error(const char *__restrict__ fmt, const T &...args)
  {
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::err)
    {
#ifdef LOGTOFILE
      Logger::get_logger()->error(spdlog::fmt_runtime_string<char>{fmt}, args...);
#else
      spdlog::error(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
    }
  }

  template <typename... T>
  void critical(const char *__restrict__ fmt, const T &...args)
  {
    if constexpr ((CONFIG_LOG_LEVEL(LOG_LEVEL)) <= spdlog::level::critical)
    {
#ifdef LOGTOFILE
      Logger::get_logger()->critical(spdlog::fmt_runtime_string<char>{fmt}, args...);
#else
      spdlog::critical(spdlog::fmt_runtime_string<char>{fmt}, args...);
#endif
    }
  }

};