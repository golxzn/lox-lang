#include <format>
#include <ranges>
#include <algorithm>

#include "lox/parser.hpp"

namespace lox {

parser::parser(const context &ctx, error_handler &errs) noexcept
	: ctx{ ctx }
	, errout{ errs }
{}

auto parser::parse() -> program {
	program prog{};
	while (!at_end()) {
		prog.emplace_back(declaration());
	}
	return prog;
}

auto parser::declaration() -> std::unique_ptr<statement> {
	try {
		if (match<token_type::kw_var>()) {
			return variable_declaration();
		}
		if (match<token_type::kw_const>()) {
			return constant_declaration();
		}

		return stmt();
	} catch (const error &e) {
		synchronize();
	}
	return nullptr;
}

auto parser::variable_declaration() -> std::unique_ptr<statement> {
	// var IDENTIFIER[: str][{ expression }];

	auto identifier{ consume(token_type::identifier, "Expected variable name", peek()) };

	std::unique_ptr<expression> expr_{};
	if (match<token_type::left_brace>()) {
		const auto &init_start{ peek() };
		if (!match<token_type::right_brace>()) {
			expr_ = expr();
			consume(token_type::right_brace, "Missed '}' brace during variable initialization",
				init_start, error_code::pe_broken_symmetry
			);
		}
	}

	match<token_type::semicolon>(); // Skip it if presented. No one cares

	return std::make_unique<statement::variable>(identifier, std::move(expr_));
}

auto parser::constant_declaration() -> std::unique_ptr<statement> {
	auto identifier{ consume(token_type::identifier, "Expected variable name", peek()) };

	consume(token_type::left_brace,
		"Missed initialization braces for constant! Constant have to be initialized",
		peek(), error_code::pe_missing_const_initialization
	);

	std::unique_ptr<expression> expr_{};
	if (!match<token_type::right_brace>()) {
		const auto &init_start{ peek() };
		if (!match<token_type::right_brace>()) {
			expr_ = expr();
			consume(token_type::right_brace, "Missed '}' brace during variable initialization",
				init_start, error_code::pe_broken_symmetry
			);
		} else {
			// INITIALIZE USING TYPE!
			make_error("Missed initialization value for constant!",
				error_code::pe_missing_const_initialization, init_start
			);
		}
	} else {
		expr_ = std::make_unique<expression::literal>(null_literal);
	}

	match<token_type::semicolon>();

	return std::make_unique<statement::constant>(identifier, std::move(expr_));
}

auto parser::stmt() -> std::unique_ptr<statement> {
#if defined(LOX_DEBUG)
	if (match<token_type::kw_print>()) {
		return make_stmt<statement::print>(&parser::expr);
	}
#endif // defined(LOX_DEBUG)
	if (match<token_type::left_brace>()) {
		return scope_stmt();
	}

	return make_stmt<statement::expression>(&parser::expr);
}

auto parser::scope_stmt() -> std::unique_ptr<statement> {
	std::vector<std::unique_ptr<statement>> statements{};
	while (!check(token_type::right_brace)) {
		statements.emplace_back(declaration());
	}
	consume(token_type::right_brace, "Expected '}' after block", peek(),
		error_code::pe_broken_symmetry
	);
	return std::make_unique<statement::scope>(std::move(statements));
}

auto parser::expr() -> std::unique_ptr<expression> {
	return assignment();
}

auto parser::assignment() -> std::unique_ptr<expression> {
	auto expr_{ equality() };

	if (match<token_type::equal>()) {
		const auto &equals_token{ previous() };
		if (auto value{ assignment() }; expr_->type() != expression::tag::assignment) {
			return std::make_unique<expression::assignment>(
				static_cast<const expression::identifier *>(expr_.get())->name,
				std::move(value)
			);
		}

		make_error("Invalid assignment target.", lox::error_code::pe_lvalue_assignment, equals_token);
	}

	return expr_;
}

auto parser::equality() -> std::unique_ptr<expression> {
	using enum token_type;
	return iterate_through<bang_equal, equal_equal>(&parser::comparison);
}

auto parser::comparison() -> std::unique_ptr<expression> {
	using enum token_type;
	return iterate_through<greater, greater_equal, less, less_equal>(&parser::term);
}

auto parser::term() -> std::unique_ptr<expression> {
	using enum token_type;
	return iterate_through<minus, plus>(&parser::factor);
}

auto parser::factor() -> std::unique_ptr<expression> {
	using enum token_type;
	return iterate_through<slash, star>(&parser::unary);
}

auto parser::unary() -> std::unique_ptr<expression> {
	if (match<token_type::bang, token_type::minus>()) {
		const auto &op{ previous() };
		return std::make_unique<expression::unary>(op, unary());
	}
	return primary();
}

auto parser::primary() -> std::unique_ptr<expression> {
	using enum token_type;

	if (match<string, number, boolean, null>()) {
		const auto &token{ previous() };
		const auto id{ token.literal_id };
		if (id < std::size(ctx.literals)) {
			return std::make_unique<expression::literal>(ctx.literals.at(id));
		}
		throw make_error(
			std::format(R"(Missing literal #{} of the "{}" token!)",
				id, token_name(token.type)
			),
			error_code::pe_missing_literal, token
		);
	}

	if (match<identifier>()) {
		return std::make_unique<expression::identifier>(previous());
	}

	if (match<left_paren>()) {
		const auto &token{ peek() };
		auto expr_{ expr() };

		consume(right_paren, "Expected ')' after expression", token, error_code::pe_broken_symmetry);
		return std::make_unique<expression::grouping>(std::move(expr_));
	}

	throw make_error("Unexpected token!", error_code::pe_unexpected_token, peek());
}

void parser::synchronize() {
	do {
		switch (peek().type) {
			using enum token_type;
			case kw_class: [[fallthrough]];
			case kw_fun: [[fallthrough]];
			case kw_var: [[fallthrough]];
			case kw_for: [[fallthrough]];
			case kw_if: [[fallthrough]];
			case kw_while: [[fallthrough]];
			case kw_return: [[fallthrough]];
			case semicolon:
				return;

			default: break;
		}

		advice();
	} while (!at_end());
}

auto parser::consume(token_type type, std::string_view on_error, const token &tok, error_code code) -> const token & {
	if (!check(type)) {
		make_error(on_error, code, tok);
	}
	return advice();
}

auto parser::advice() -> const token & {
	m_current += static_cast<size_t>(!at_end());
	return previous();
}

auto parser::check(const token_type type) const -> bool {
	return !at_end() && peek().type == type;
}

auto parser::at_end() const -> bool {
	/* m_current < std::size(m_tokens) || */
	return peek().type == token_type::end_of_file;
}

auto parser::peek() const -> const token & {
	return ctx.tokens.at(m_current);
}

auto parser::previous() const -> const token & {
	return ctx.tokens.at(m_current - 1ull);
}

auto parser::make_error(std::string_view msg, error_code code, const token &tok) const -> error {
	errout.report(msg, lox::error_record{
		.code    = code,
		.line    = tok.line,
		.from    = tok.position,
		.to      = tok.position + static_cast<uint32_t>(std::size(token_string(tok.type)))
	});

	return error{ msg };
}

} // namespace lox
