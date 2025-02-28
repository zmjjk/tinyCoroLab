#pragma once

#include <concepts>
#include <coroutine>
#include <type_traits>
#include <utility>

namespace coro::concepts
{
template<typename type, typename... types>
concept in_types = (std::same_as<type, types> || ...);

template<typename type>
concept list_type = requires(type t) {
    { t.next() } -> std::same_as<type*>;
};

template<typename T>
concept no_ref_pod =
    std::is_standard_layout_v<T> && std::is_trivial_v<T> && !std::is_reference_v<T>;

template<typename T>
concept void_no_ref_pod = no_ref_pod<T> || std::is_void_v<T>;
}; // namespace coro::concepts