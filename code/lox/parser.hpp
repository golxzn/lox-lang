#pragma once

#include <vector>
#include <concepts>
#include <type_traits>

#include "lox/token.hpp"
#include "lox/literal.hpp"
#include "lox/expression.hpp"
#include "lox/error_handler.hpp"

namespace lox {

class parser {
public:
	struct error : public std::runtime_error {
		explicit error(std::string_view msg) noexcept : std::runtime_error{ std::data(msg) } {}
	};

	parser(
		std::vector<token> tokens,
		std::vector<literal> literals,
		file_id file_id,
		error_handler &errs
	) noexcept;

	[[nodiscard]] auto parse() -> std::unique_ptr<expression>;

private:
	std::vector<token> m_tokens;
	std::vector<literal> m_literals;
	error_handler &errout;
	size_t m_current{};
	file_id m_file_id{};

	template<token_type ...Types>
	auto iterate_through(auto next) -> std::unique_ptr<expression> {
		auto expr_{ std::invoke(next, this) };

		while (match<Types...>()) {
			const auto &op{ previous() };
			expr_ = std::make_unique<expression::binary>(
				op, std::move(expr_), std::invoke(next, this)
			);
		}

		return std::move(expr_);
	}

	template<token_type ...Types>
	auto match() -> bool {
		if (at_end()) return false;

		if (const auto type{ peek().type }; ((type == Types) || ...)) {
			++m_current;
			return true;
		}
		return false;
	}

	auto expr() -> std::unique_ptr<expression>;
	auto equality() -> std::unique_ptr<expression>;
	auto comparison() -> std::unique_ptr<expression>;
	auto term() -> std::unique_ptr<expression>;
	auto factor() -> std::unique_ptr<expression>;
	auto unary() -> std::unique_ptr<expression>;
	auto primary() -> std::unique_ptr<expression>;

	void synchronize();

	auto advice() -> const token &;
	auto check(token_type type) const -> bool;
	auto at_end() const -> bool;
	auto peek() const -> const token &;
	auto previous() const -> const token &;

	auto make_error(std::string_view msg, error_code code, const token &tok) const -> error;
};

} // namespace lox
