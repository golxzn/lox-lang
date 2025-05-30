#pragma once

#include <string>
#include <unordered_map>
#include <memory_resource>

#include "lox/types/function.hpp"

namespace lox::execution {

/*
environment.push_scope();

environment.pop_scope();

std::vector<lexeme_id> keys;
std::vector<lox::literal> literals;
std::vector<size_t> scopes;

look_up {
}

*/

namespace search_range {

constexpr struct globally_type      final {} globally{};
constexpr struct current_scope_type final {} current_scope{};

} // namespace search_range

class LOX_EXPORT environment {
public:
	void push_scope();
	void pop_scope();

	[[nodiscard]] auto define_variable(lexeme_id id, lox::literal value) -> bool;
	[[nodiscard]] auto define_constant(lexeme_id id, lox::literal value) -> bool;

	[[nodiscard]] auto contains(lexeme_id id, search_range::globally_type) const noexcept -> bool;
	[[nodiscard]] auto contains(lexeme_id id, search_range::current_scope_type _ = {}) const noexcept -> bool;

	[[nodiscard]] auto look_up(lexeme_id id) const -> const lox::literal &;

	enum class assignment_status : uint8_t { ok, not_found, constant };
	[[nodiscard]] auto assign(lexeme_id id, lox::literal value) noexcept -> assignment_status;

	[[nodiscard]] auto has_function(lexeme_id id) const noexcept -> bool;
	[[nodiscard]] auto get_function(lexeme_id id) const -> function;
	[[nodiscard]] auto register_function(lexeme_id id, function fun) -> bool;

	[[nodiscard]] auto function_at(size_t id) const noexcept -> std::optional<function>;

private:
	enum class mutability : uint8_t {
		constant,
		variable,
	};
	struct value_container {
		lox::literal value{};
		mutability type{ mutability::constant };
	};


	std::vector<lexeme_id> m_keys;
	std::vector<value_container> m_values;
	std::vector<function> m_functions;
	std::vector<size_t> m_scopes;

	auto index_of(lexeme_id id) const noexcept -> int64_t;
	auto push_value(lexeme_id id, lox::literal value, mutability type) -> bool;
	auto get_rewind_point() const noexcept -> size_t;
};

} // namespace lox::execution
