#pragma once

#include <span>
#include <array>
#include <optional>

#include "lox/export.hpp"
#include "lox/aliases.hpp"
#include "lox/types/token.hpp"
#include "lox/types/literal.hpp"

namespace lox {

class LOX_EXPORT function {
public:
	using signature = literal(*)(void *, std::span<const literal> args);

	constexpr explicit function(const signature function, std::optional<size_t> arity = {}) noexcept
		: m_func{ function }, m_arity{ arity } {}

	[[nodiscard]] constexpr auto call(auto *runner, std::span<const literal> args = {}) -> literal {
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

	[[nodiscard]] constexpr auto arity() const noexcept -> std::optional<size_t> { return m_arity; }
	[[nodiscard]] constexpr auto valid() const noexcept -> bool { return m_func != nullptr; }

	[[nodiscard]] constexpr auto enough_arguments_count(const size_t args_count) const noexcept -> bool {
		return !m_arity.has_value() || m_arity == args_count;
	}

private:
	signature m_func{ nullptr };
	std::optional<size_t> m_arity{};
};

} // namespace lox
