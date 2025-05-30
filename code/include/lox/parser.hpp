#pragma once

#include <concepts>
#include <type_traits>

#include "lox/types/context.hpp"
#include "lox/error_handler.hpp"
#include "lox/program.hpp"

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

	auto declaration(program &out) -> statement_id;
	auto storage_declaration(program &out) -> statement_id;
	auto variable_declaration(program &out) -> statement_id;
	auto constant_declaration(program &out) -> statement_id;

	auto object_declaration(program &out, std::string_view name) -> std::pair<token, expression_id>;

	auto stmt(program &out) -> statement_id;
	auto branch_stmt(program &out) -> statement_id;
	auto loop_stmt(program &out) -> statement_id;
	auto for_loop_stmt(program &out) -> statement_id;
	auto scope_stmt(program &out) -> statement_id;

	template<statement_type Type>
	auto make_stmt(program &out, auto content_gen) -> statement_id {
		auto content{ std::invoke(content_gen, this, out) };
		consume(token_type::semicolon, "Expected ';' after statement", previous());
		return out.emplace<Type>(content);
	}

	auto make_declaration_or_expression_stmt(program &out) -> statement_id;
	static auto make_loop(
		program &out,
		statement_id declaration,
		expression_id condition,
		statement_id body
	) -> statement_id;

	template<token_type ...Types>
	auto iterate_through(program &out, auto next) -> expression_id {
		auto expr_{ std::invoke(next, this, out) };

		while (match<Types...>()) {
			const auto &op{ previous() };
			expr_ = out.emplace<expression_type::binary>(
				op, expr_, std::invoke(next, this, out)
			);
		}

		return expr_;
	}

	template<expression_type Expr, token_type ...Types>
	auto iterate_if(program &out, auto next) -> expression_id {
		auto expr_{ std::invoke(next, this, out) };

		if (match<Types...>()) {
			const auto &op{ previous() };
			expr_ = out.emplace<Expr>(op, expr_, std::invoke(next, this, out));
		}

		return expr_;
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

	auto expr(program &out) -> expression_id;
	auto incdec(program &out) -> expression_id;
	auto assignment(program &out) -> expression_id;
	auto logical_or(program &out) -> expression_id;
	auto logical_and(program &out) -> expression_id;
	auto equality(program &out) -> expression_id;
	auto comparison(program &out) -> expression_id;
	auto term(program &out) -> expression_id;
	auto factor(program &out) -> expression_id;
	auto unary(program &out) -> expression_id;
	auto call(program &out) -> expression_id;
	auto call_finish(program &out, expression_id caller) -> expression_id;
	auto primary(program &out) -> expression_id;

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
