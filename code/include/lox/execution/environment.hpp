#pragma once

#include <string>
#include <unordered_map>
#include <memory_resource>

#include "lox/types.hpp"
#include "lox/token.hpp"
#include "lox/literal.hpp"

namespace lox::execution {


class environment {
public:
	[[nodiscard]] auto define_variable(lox::token tok, lox::literal value) -> bool;
	[[nodiscard]] auto define_constant(lox::token tok, lox::literal value) -> bool;

	[[nodiscard]] auto contains(lox::token tok) const noexcept -> bool;
	[[nodiscard]] auto look_up(lox::token tok) const -> const lox::literal &;
	auto assign(lox::token tok, lox::literal value) noexcept -> bool;

private:
	std::unordered_map<lexeme_id, literal> m_values{};
	std::unordered_map<lexeme_id, literal> m_constants{};
};

} // namespace lox::execution
