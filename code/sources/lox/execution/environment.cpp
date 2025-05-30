#include <format>
#include <algorithm>
#include <stdexcept>

#include "lox/execution/environment.hpp"

namespace lox::execution {

void environment::push_scope() {
	m_scopes.emplace_back(std::size(m_keys));
}

void environment::pop_scope() {
	const size_t rewind_point{ get_rewind_point() };
	m_keys.resize(rewind_point);
	m_values.resize(rewind_point);
	if (!std::empty(m_scopes)) m_scopes.pop_back();
}


auto environment::define_variable(lexeme_id id, lox::literal value) -> bool {
	return push_value(id, std::move(value), mutability::variable);
}

auto environment::define_constant(lexeme_id id, lox::literal value) -> bool {
	return push_value(id, std::move(value), mutability::constant);
}

auto environment::contains(lexeme_id id, search_range::globally_type) const noexcept -> bool {
	const auto end{ std::rend(m_keys) };
	return std::find(std::rbegin(m_keys), end, id) != end;
}

auto environment::contains(lexeme_id id, search_range::current_scope_type) const noexcept -> bool {
	const auto end{ std::next(std::rbegin(m_keys),
		static_cast<std::ptrdiff_t>(std::size(m_keys) - get_rewind_point())
	) };

	return std::find(std::rbegin(m_keys), end, id) != end;
}

auto environment::look_up(lexeme_id id) const -> const lox::literal & {
	if (const auto index{ index_of(id) }; index >= 0ll) {
		return m_values[index].value;
	}

	throw std::out_of_range{ R"(Undefined variable or constant)" };
}

auto environment::assign(lexeme_id id, lox::literal value) noexcept -> assignment_status {
	const auto index{ index_of(id) };
	if (index < 0ll) {
		return assignment_status::not_found;
	}

	if (auto &container{ m_values[index] }; container.type == mutability::variable) {
		container.value = std::move(value);
		return assignment_status::ok;
	}

	return assignment_status::constant;
}

auto environment::has_function(lexeme_id address) const noexcept -> bool {
	if (!contains(address, search_range::globally)) return false;

	if (const auto id{ look_up(address).as<int64_t>() }; id) {
		return *id < std::size(m_functions);
	}

	return false;
}

auto environment::get_function(lexeme_id address) const -> function {
	return m_functions.at(std::get<int64_t>(look_up(address)));
}

auto environment::register_function(lexeme_id address, function fun) -> bool {
	const literal id{ static_cast<int64_t>(std::size(m_functions)) };
	m_functions.emplace_back(fun);
	return define_constant(address, id);
}

auto environment::function_at(size_t id) const noexcept -> std::optional<function> {
	if (id >= std::size(m_functions)) return std::nullopt;
	return m_functions[id];
}

auto environment::index_of(lexeme_id id) const noexcept -> int64_t {
	int64_t index{ static_cast<int64_t>(std::size(m_keys) - 1ull) };
	for (; index >= 0ll && id != m_keys[index]; --index) { }
	return index;
}

auto environment::push_value(lexeme_id id, lox::literal value, mutability type) -> bool {
	if (contains(id)) {
		return false;
	}

	m_keys.emplace_back(id);
	m_values.emplace_back(std::move(value), type);
	return true;
}

auto environment::get_rewind_point() const noexcept -> size_t {
	return std::empty(m_scopes) ? 0ull : m_scopes.back();
}

} // namespace lox::execution
