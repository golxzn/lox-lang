#pragma once

#include <concepts>
#include <type_traits>

#include "lox/program.hpp"
#include "lox/context.hpp"
#include "lox/error_handler.hpp"

namespace lox {

class LOX_EXPORT parser {
public:
	struct error : public std::runtime_error {
		explicit error(std::string_view msg) noexcept : std::runtime_error{ std::data(msg) } {}
	};

	parser(const context &ctx, error_handler &errs) noexcept;

	[[nodiscard]] auto parse() -> program;

private:
	const context &ctx;
	error_handler &errout;
	size_t m_current{};

	auto declaration() -> std::unique_ptr<statement>;
	auto variable_declaration() -> std::unique_ptr<statement>;
	auto constant_declaration() -> std::unique_ptr<statement>;

	auto stmt() -> std::unique_ptr<statement>;
	auto scope_stmt() -> std::unique_ptr<statement>;

	template<std::derived_from<statement> Type>
	auto make_stmt(auto content_gen) -> std::unique_ptr<statement> {
		auto content{ std::invoke(content_gen, this) };
		consume(token_type::semicolon, "Expected ';' after statement", peek());
		return std::make_unique<Type>(std::move(content));
	}

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
	auto assignment() -> std::unique_ptr<expression>;
	auto equality() -> std::unique_ptr<expression>;
	auto comparison() -> std::unique_ptr<expression>;
	auto term() -> std::unique_ptr<expression>;
	auto factor() -> std::unique_ptr<expression>;
	auto unary() -> std::unique_ptr<expression>;
	auto primary() -> std::unique_ptr<expression>;

	void synchronize();

	auto consume(token_type type, std::string_view on_error,
		const token &tok, error_code code = error_code::pe_unexpected_token
	) -> const token &;

	auto advice() -> const token &;
	auto check(token_type type) const -> bool;
	auto at_end() const -> bool;
	auto peek() const -> const token &;
	auto previous() const -> const token &;

	auto make_error(std::string_view msg, error_code code, const token &tok) const -> error;
};

} // namespace lox
