#include <fast_float/fast_float.h>
#include <fast_float/parse_number.h>

#include "lox/literal.hpp"
#include "lox/utils/strhash.hpp"

namespace lox {

auto literal::is(literal_type type) const noexcept -> bool {
	return index() == _id(type);
}

auto literal::type() const noexcept -> literal_type {
	return static_cast<literal_type>(index());
}


auto to_number_literal(const std::string_view str_value) -> literal {
	if (std::empty(str_value)) return literal{ 0 };

	const auto b{ std::begin(str_value) };
	const auto e{ std::end(str_value) };

	if (double val{}; fast_float::from_chars(b, e, val).ec == std::errc{}) {
		return literal{ val };
	}

	if (int64_t val{}; fast_float::from_chars(b, e, val).ec == std::errc{}) {
		return literal{ val };
	}

	return literal{ 0 };
}

auto to_literal(const std::string_view str_value) -> literal {
	if (std::empty(str_value)) return {};

	switch (utils::fnv1a(str_value)) {
		using namespace utils::fnv1a_literals;
		case "null"_fnv1a:  return literal{};
		case "true"_fnv1a:  return literal{ true };
		case "false"_fnv1a: return literal{ false };

		// common_values
		case "0"_fnv1a:   return literal{ int64_t{ 0 } };
		case "0.0"_fnv1a: return literal{ double{ 0.0 } };

		case "1"_fnv1a:   return literal{ int64_t{ 1 } };
		case "1.0"_fnv1a: return literal{ double{ 1.0 } };

		case "-1"_fnv1a:   return literal{ int64_t{ -1 } };
		case "-1.0"_fnv1a: return literal{ double{ -1.0 } };

		default: break;
	}

	const auto b{ std::begin(str_value) };
	const auto e{ std::end(str_value) };

	if (double val{}; fast_float::from_chars(b, e, val).ec == std::errc{}) {
		return literal{ val };
	}

	if (int64_t val{}; fast_float::from_chars(b, e, val).ec == std::errc{}) {
		return literal{ val };
	}

	return literal{ str_value };
}


} // namespace lox
