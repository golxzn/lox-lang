#include <cstdio>

#include "lox/types/token.hpp"
#include "lox/utils/strhash.hpp"
#include "lox/utils/strutils.hpp"

namespace lox {

auto from_keyword(const std::string_view name) noexcept -> token_type {
	using enum token_type;

	switch (utils::fnv1a(name)) {
		using namespace utils::fnv1a_literals;

		case "var"_fnv1a:    return kw_var;
		case "const"_fnv1a:  return kw_const;
		case "and"_fnv1a:    return kw_and;
		case "or"_fnv1a:     return kw_or;
		case "if"_fnv1a:     return kw_if;
		case "else"_fnv1a:   return kw_else;
		case "while"_fnv1a:  return kw_while;
		case "for"_fnv1a:    return kw_for;
		case "fun"_fnv1a:    return kw_fun;
		case "return"_fnv1a: return kw_return;
		case "class"_fnv1a:  return kw_class;
		case "this"_fnv1a:   return kw_this;
		case "super"_fnv1a:  return kw_super;

		default: break;
	}

	return identifier;
}

[[nodiscard]] auto name_from_script(const token &tok, const std::string_view script) noexcept -> std::string_view {
	if (tok.type != token_type::identifier || tok.position >= std::size(script)) return {};

	const auto whitespace_pos{ script.find_first_of(utils::whitespaces, tok.position) };
	const auto other_pos{ script.find_first_of(";:{", tok.position) };

	return script.substr(tok.position, (std::min)(whitespace_pos, other_pos) - tok.position);
}


} // namespace lox
