#include <format>
#include <stdexcept>

#include "lox/execution/environment.hpp"

namespace lox::execution {

auto environment::define_variable(lox::token tok, lox::literal value) -> bool {
	return m_values.try_emplace(tok.lexeme_id, std::move(value)).second;
}

auto environment::define_constant(lox::token tok, lox::literal value) -> bool {
	return m_constants.try_emplace(tok.lexeme_id, std::move(value)).second;
}

auto environment::contains(lox::token tok) const noexcept -> bool {
	return m_constants.contains(tok.lexeme_id)
		|| m_values.contains(tok.lexeme_id);
}

auto environment::look_up(lox::token tok) const -> const lox::literal & {
	if (const auto found{ m_constants.find(tok.lexeme_id) }; found != std::end(m_constants)) {
		return found->second;
	}
	if (const auto found{ m_values.find(tok.lexeme_id) }; found != std::end(m_values)) {
		return found->second;
	}

	/// @todo It's essential to provide WHICH FUCKING VARIABLE is undefined. The NAME, Carl
	throw std::out_of_range{ R"(Undefined variable or constant)" };
}

auto environment::assign(lox::token tok, lox::literal value) noexcept -> bool {
	if (auto found{ m_values.find(tok.lexeme_id) }; found != std::end(m_values)) {
		found->second = std::move(value);
		return true;
	}

	return false;
}

} // namespace lox::execution
