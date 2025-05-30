#pragma once

#include "lox/types/token.hpp"
#include "lox/types/literal.hpp"
#include "lox/lexeme_database.hpp"

namespace lox {

struct context {
	lexeme_database lexemes{};
	std::vector<token> tokens{};
	std::vector<literal> literals{};
};


} // namespace lox
