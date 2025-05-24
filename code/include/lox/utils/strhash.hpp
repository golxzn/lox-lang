#pragma once

#include <span>
#include <cstdint>
#include <type_traits>
#include <string_view>

#include "lox/types.hpp"

namespace lox::utils {

constexpr hash_type offset_basis{ 0x811C9DC5 };
constexpr hash_type FNV_prime{ 0x01000193 };

template<std::integral T>
[[nodiscard]] constexpr auto fnv1a(std::span<const T> bytes) noexcept -> hash_type {
	hash_type hash{ offset_basis };
	for (size_t i{}; i < std::size(bytes); ++i) {
		hash = (hash ^ static_cast<hash_type>(bytes[i])) * FNV_prime;
	}
	return hash;
}

template<std::integral CharT>
[[nodiscard]] constexpr auto fnv1a(const std::basic_string_view<CharT> str) noexcept -> hash_type {
	return fnv1a(std::span{ std::data(str), std::size(str) });
}

namespace fnv1a_literals {

[[nodiscard]] consteval auto operator""_fnv1a(const char *str, size_t len) {
	return fnv1a(std::span{ str, len });
}

[[nodiscard]] consteval auto operator""_fnv1a(const wchar_t *str, size_t len) {
	return fnv1a(std::span{ str, len });
}

[[nodiscard]] consteval auto operator""_fnv1a(const char8_t *str, size_t len) {
	return fnv1a(std::span{ str, len });
}

[[nodiscard]] consteval auto operator""_fnv1a(const char16_t *str, size_t len) {
	return fnv1a(std::span{ str, len });
}

[[nodiscard]] consteval auto operator""_fnv1a(const char32_t *str, size_t len) {
	return fnv1a(std::span{ str, len });
}

} // namespace fnv1a_literals

} // namespace lox::utils
