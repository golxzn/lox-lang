#include <cstdio>

#include "lox/token.hpp"
#include "lox/utils/strhash.hpp"

namespace lox {

auto from_keyword(const std::string_view name) noexcept -> token_type {
	using enum token_type;

	switch (utils::fnv1a(name)) {
		using namespace utils::fnv1a_literals;

		case "var"_fnv1a:    return kw_var;
		case "true"_fnv1a:   return kw_true;
		case "false"_fnv1a:  return kw_false;
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

} // namespace lox
