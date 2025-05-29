#pragma once

#include <concepts>
#include <type_traits>

namespace lox::utils::ct {

template<class T, class ...Ts>
struct are_same : std::bool_constant<(std::is_same_v<T, Ts> && ...)> {};

template<class T, class ...Ts>
constexpr auto are_same_v{ are_same<T, Ts...>::value };

template<class T, class ...Ts>
struct are_convertible : std::bool_constant<(std::is_convertible_v<T, Ts> && ...)> {};

template<class T, class ...Ts>
constexpr auto are_convertible_v{ are_convertible<T, Ts...>::value };

template<class T, class ...Ts>
struct are_base_of : std::bool_constant<(std::is_base_of_v<T, Ts> && ...)> {};

template<class T, class ...Ts>
constexpr auto are_base_of_v{ are_base_of<T, Ts...>::value };

[[nodiscard]] constexpr bool any_from(const auto &t, const auto &...ts) noexcept {
	return ((t == ts) | ...);
}
[[nodiscard]] constexpr bool none_from(const auto &t, const auto &...ts) noexcept {
	return ((t != ts) & ...);
}
[[nodiscard]] constexpr bool all_from(const auto &t, const auto &...ts) noexcept {
	return ((t == ts) & ...);
}

template<class T>
concept enumeration = std::is_enum_v<T>;

template<auto ...Values>
	requires are_same_v<decltype(Values)...> && (std::is_enum_v<decltype(Values)> && ...)
[[nodiscard]] constexpr bool any_from(const auto value) noexcept {
	return ((Values == value) | ...);
}


} // namespace lox::utils::ct
