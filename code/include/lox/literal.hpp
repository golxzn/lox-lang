#pragma once

#include <cstdint>
#include <variant>
#include <string_view>

#include "lox/export.hpp"

namespace lox {

enum class literal_type : uint8_t {
	null,
	boolean,
	number,
	integral,
	string
};

using literal_base = std::variant<std::monostate,
	bool, double, int64_t, std::string
>;

struct LOX_EXPORT literal : public literal_base {
	using literal_base::variant;

	[[nodiscard]] auto is(literal_type type) const noexcept -> bool;
	[[nodiscard]] auto type() const noexcept -> literal_type;

	template<class T>
	[[nodiscard]] auto as() const noexcept -> const T * { return std::get_if<T>(this); }

	template<class T>
	[[nodiscard]] auto as() noexcept -> T * { return std::get_if<T>(this); }

private:
	static constexpr auto _id(const literal_type t) noexcept -> size_t {
		return static_cast<size_t>(t);
	}
};

[[nodiscard]] auto to_number_literal(const std::string_view str_value) -> literal;
[[nodiscard]] auto to_literal(const std::string_view str_value) -> literal;
[[nodiscard]] auto to_string(const literal &lit) -> std::string;
[[nodiscard]] constexpr auto type_name(const literal_type type) noexcept -> std::string_view {
	using namespace std::string_view_literals;
	switch (type) {
		using enum literal_type;

		case null:     return "null"sv;
		case boolean:  return "boolean"sv;
		case number:   return "number"sv;
		case integral: return "integer"sv;
		case string:   return "string"sv;

		default:
			break;
	}
	return ""sv;
}

} // namespace lox
