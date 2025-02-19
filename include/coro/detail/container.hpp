#pragma once

#include <exception>
#include <variant>

namespace coro::detail
{
template<typename T>
struct container
{
private:
    struct unset_return_value
    {
        unset_return_value() {}
        unset_return_value(unset_return_value&&)      = delete;
        unset_return_value(const unset_return_value&) = delete;
        auto operator=(unset_return_value&&)          = delete;
        auto operator=(const unset_return_value&)     = delete;
    };

public:
    static constexpr bool return_type_is_reference = std::is_reference_v<T>;
    using stored_type =
        std::conditional_t<return_type_is_reference, std::remove_reference_t<T>*, std::remove_const_t<T>>;
    using variant_type = std::variant<unset_return_value, stored_type, std::exception_ptr>;

    template<typename value_type>
        requires(return_type_is_reference and std::is_constructible_v<T, value_type &&>) or
                (not return_type_is_reference and std::is_constructible_v<stored_type, value_type &&>)
    auto return_value(value_type&& value) -> void
    {
        if constexpr (return_type_is_reference)
        {
            T ref = static_cast<value_type&&>(value);
            m_storage.template emplace<stored_type>(std::addressof(ref));
        }
        else
        {
            m_storage.template emplace<stored_type>(std::forward<value_type>(value));
        }
    }

    auto return_value(stored_type&& value) -> void
        requires(not return_type_is_reference)
    {
        if constexpr (std::is_move_constructible_v<stored_type>)
        {
            m_storage.template emplace<stored_type>(std::move(value));
        }
        else
        {
            m_storage.template emplace<stored_type>(value);
        }
    }

    auto result() & -> decltype(auto)
    {
        if (std::holds_alternative<stored_type>(m_storage))
        {
            if constexpr (return_type_is_reference)
            {
                return static_cast<T>(*std::get<stored_type>(m_storage));
            }
            else
            {
                return static_cast<const T&>(std::get<stored_type>(m_storage));
            }
        }
        else if (std::holds_alternative<std::exception_ptr>(m_storage))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }
        else
        {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto result() const& -> decltype(auto)
    {
        if (std::holds_alternative<stored_type>(m_storage))
        {
            if constexpr (return_type_is_reference)
            {
                return static_cast<std::add_const_t<T>>(*std::get<stored_type>(m_storage));
            }
            else
            {
                return static_cast<const T&>(std::get<stored_type>(m_storage));
            }
        }
        else if (std::holds_alternative<std::exception_ptr>(m_storage))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }
        else
        {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto result() && -> decltype(auto)
    {
        if (std::holds_alternative<stored_type>(m_storage))
        {
            if constexpr (return_type_is_reference)
            {
                return static_cast<T>(*std::get<stored_type>(m_storage));
            }
            else if constexpr (std::is_move_constructible_v<T>)
            {
                return static_cast<T&&>(std::get<stored_type>(m_storage));
            }
            else
            {
                return static_cast<const T&&>(std::get<stored_type>(m_storage));
            }
        }
        else if (std::holds_alternative<std::exception_ptr>(m_storage))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }
        else
        {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto set_exception() noexcept -> void { new (&m_storage) variant_type(std::current_exception()); }

    inline auto value_ready() noexcept -> bool { return std::holds_alternative<stored_type>(m_storage); }

    inline auto value_exception() noexcept -> bool { return std::holds_alternative<std::exception_ptr>(m_storage); }

    inline auto value_unset() noexcept -> bool { return !value_ready() && !value_exception(); }

private:
    variant_type m_storage{};
};

}; // namespace coro::detail
