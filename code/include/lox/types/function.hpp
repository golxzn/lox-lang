#pragma once

#include <span>
#include <array>
#include <optional>
#include <functional>

#include "lox/export.hpp"
#include "lox/aliases.hpp"
#include "lox/types/token.hpp"
#include "lox/types/literal.hpp"

namespace lox {

class LOX_EXPORT function {
public:
	using signature = literal(void *, std::span<const literal>);

	explicit function(std::function<signature> function, std::optional<size_t> arity = {}) noexcept
		: m_func{ std::move(function) }, m_arity{ arity } {}

	[[nodiscard]] auto call(auto *runner, std::span<const literal> args = {}) -> literal {
		if (valid() && enough_arguments_count(std::size(args))) {
			return m_func(runner, args);
		}
		return null_literal;
	}

	// template<class ...Args>
	// [[nodiscard]] constexpr auto call(auto *runner, Args ...args) -> literal {
	// 	const std::array<literal, sizeof...(args)> params{
	// 		(literal{ std::forward<Args>(args) }, ...)
	// 	};
	// 	return call(runner, params);
	// }

	[[nodiscard]] auto arity() const noexcept -> std::optional<size_t> { return m_arity; }
	[[nodiscard]] auto valid() const noexcept -> bool { return m_func != nullptr; }

	[[nodiscard]] auto enough_arguments_count(const size_t args_count) const noexcept -> bool {
		return !m_arity.has_value() || m_arity == args_count;
	}

private:
	std::function<signature> m_func{};
	std::optional<size_t> m_arity{};
};

} // namespace lox
