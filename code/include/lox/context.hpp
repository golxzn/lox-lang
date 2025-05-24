#pragma once

#include "lox/token.hpp"
#include "lox/literal.hpp"
#include "lox/lexeme_database.hpp"

namespace lox {

struct context {
	lexeme_database lexemes{};
	std::vector<token> tokens{};
	std::vector<literal> literals{};
};


} // namespace lox
