#pragma once

#include <limits>
#include <cstdint>
#include <string_view>

#include "lox/export.hpp"

namespace lox {

enum class token_type : uint8_t {
	invalid,

	// Single character tokens
	left_paren, right_paren,
	left_brace, right_brace,
	comma, dot, minus, plus, semicolon, slash, star,

	// One-two characters tokens
	bang, bang_equal,
	equal, equal_equal,
	less, less_equal,
	greater, greater_equal,

	// Literals
	identifier, string, number, boolean, null,

	// keywords
	kw_var,
	kw_and, kw_or, kw_if, kw_else, kw_while, kw_for,
	kw_fun, kw_return, kw_class, kw_this, kw_super,

#if defined(LOX_DEBUG)
	kw_print,
#endif // defined(LOX_DEBUG)

	end_of_file = 0xFFu
};

constexpr uint16_t invalid_literal_id{ (std::numeric_limits<uint16_t>::max)() };

#pragma pack(push, 1)
struct LOX_EXPORT token {
	uint32_t line{};
	uint32_t position{};
	uint16_t literal_id{ invalid_literal_id };
	token_type type{ token_type::invalid };
};
#pragma pack(pop)

[[nodiscard]] constexpr auto keyword_name(const token_type keyword) noexcept -> std::string_view {
	using namespace std::string_view_literals;
	using enum token_type;

	switch (keyword) {
	// Literals
		case null:          return "null"sv;

	// keywords
		case kw_var:        return "var"sv;
		case kw_and:        return "and"sv;
		case kw_or:         return "or"sv;
		case kw_if:         return "if"sv;
		case kw_else:       return "else"sv;
		case kw_while:      return "while"sv;
		case kw_for:        return "for"sv;
		case kw_fun:        return "fun"sv;
		case kw_return:     return "return"sv;
		case kw_class:      return "class"sv;
		case kw_this:       return "this"sv;
		case kw_super:      return "super"sv;

#if defined(LOX_DEBUG)
		case kw_print:      return "print"sv;
#endif // defined(LOX_DEBUG)

		default:
			break;
	}

	return ""sv;
}

[[nodiscard]] constexpr auto token_name(const token_type type) noexcept -> std::string_view {
	using namespace std::string_view_literals;
	using enum token_type;

	switch (type) {
	// Single character tokens
		case left_paren:    return "left_paren"sv;
		case right_paren:   return "right_paren"sv;
		case left_brace:    return "left_brace"sv;
		case right_brace:   return "right_brace"sv;
		case comma:         return "comma"sv;
		case dot:           return "dot"sv;
		case minus:         return "minus"sv;
		case plus:          return "plus"sv;
		case semicolon:     return "semicolon"sv;
		case slash:         return "slash"sv;
		case star:          return "star"sv;

	// One-two characters tokens
		case bang:          return "bang"sv;
		case bang_equal:    return "bang_equal"sv;
		case equal:         return "equal"sv;
		case equal_equal:   return "equal_equal"sv;
		case less:          return "less"sv;
		case less_equal:    return "less_equal"sv;
		case greater:       return "greater"sv;
		case greater_equal: return "greater_equal"sv;
		default: break;
	}

	switch (type) {
	// Literals
		case identifier:    return "identifier"sv;
		case string:        return "string"sv;
		case number:        return "number"sv;
		case boolean:       return "boolean"sv;
		case null:          return "null"sv;

		case end_of_file:   return "end_of_file"sv;
		default:
			break;
	}

	if (const auto word{ keyword_name(type) }; !std::empty(word)) {
		return word;
	}
	return "invalid"sv;
}

[[nodiscard]] constexpr auto token_string(const token_type type) noexcept -> std::string_view {
	using namespace std::string_view_literals;
	using enum token_type;

	switch (type) {
	// Single character tokens
		case left_paren:    return "("sv;
		case right_paren:   return ")"sv;
		case left_brace:    return "{"sv;
		case right_brace:   return "}"sv;
		case comma:         return ","sv;
		case dot:           return "."sv;
		case minus:         return "-"sv;
		case plus:          return "+"sv;
		case semicolon:     return ";"sv;
		case slash:         return "/"sv;
		case star:          return "*"sv;

	// One-two characters tokens
		case bang:          return "!"sv;
		case bang_equal:    return "!="sv;
		case equal:         return "="sv;
		case equal_equal:   return "=="sv;
		case less:          return "<"sv;
		case less_equal:    return "<="sv;
		case greater:       return ">"sv;
		case greater_equal: return ">="sv;

	// Literals
		case null:          return "null"sv;

		default: break;
	}

	return keyword_name(type);
}

[[nodiscard]] auto from_keyword(const std::string_view name) noexcept -> token_type;

namespace token_traits {

template<token_type>
struct is_arithmetic : std::bool_constant<false> {};

template<> struct is_arithmetic<token_type::plus> : std::bool_constant<true> {};
template<> struct is_arithmetic<token_type::minus> : std::bool_constant<true> {};
template<> struct is_arithmetic<token_type::star> : std::bool_constant<true> {};
template<> struct is_arithmetic<token_type::slash> : std::bool_constant<true> {};

template<token_type Token>
constexpr auto is_arithmetic_v{ is_arithmetic<Token>::value };


template<token_type>
struct is_logical : std::bool_constant<false> {};

template<> struct is_logical<token_type::equal_equal> : std::bool_constant<true> {};
template<> struct is_logical<token_type::bang_equal> : std::bool_constant<true> {};
template<> struct is_logical<token_type::less> : std::bool_constant<true> {};
template<> struct is_logical<token_type::less_equal> : std::bool_constant<true> {};
template<> struct is_logical<token_type::greater> : std::bool_constant<true> {};
template<> struct is_logical<token_type::greater_equal> : std::bool_constant<true> {};

template<token_type Token>
constexpr auto is_logical_v{ is_logical<Token>::value };

} // namespace token_traits

} // namespace lox
