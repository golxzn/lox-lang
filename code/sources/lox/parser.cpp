#include <format>
#include <ranges>
#include <algorithm>

#include "lox/parser.hpp"
#include "lox/utils/vecutils.hpp"
#include "lox/utils/compile_time.hpp"

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
		if (auto storage{ storage_declaration() }; storage) {
			return std::move(storage);
		}

		return stmt();
	} catch (const error &e) {
		synchronize();
	}
	return nullptr;
}

auto parser::storage_declaration() -> std::unique_ptr<statement> {
	if (match<token_type::kw_const, token_type::kw_var>()) {
		return previous().type == token_type::kw_const
			? constant_declaration()
			: variable_declaration()
		;
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
	if (match<token_type::kw_if>()) {
		return branch_stmt();
	}
#if defined(LOX_DEBUG)
	if (match<token_type::kw_print>()) {
		return make_stmt<statement::print>(&parser::expr);
	}
#endif // defined(LOX_DEBUG)
	if (match<token_type::kw_while>()) {
		return loop_stmt();
	}
	if (match<token_type::kw_for>()) {
		return for_loop_stmt();
	}
	if (match<token_type::left_brace>()) {
		return scope_stmt();
	}

	return make_stmt<statement::expression>(&parser::expr);
}

auto parser::branch_stmt() -> std::unique_ptr<statement> {
	using enum token_type;

	consume(left_paren, "Expected '(' after 'if' statement", peek());

	auto declaration{ storage_declaration() };
	auto condition{ expr() };

	consume(right_paren, "Expected ')' after 'if' condition", peek(), error_code::pe_broken_symmetry);

	const auto get_block{ [this] {
		consume(left_brace, "Branch requires '{' block", peek());
		return scope_stmt();
	} };

	auto then_block{ get_block() };

	auto branch{ std::make_unique<statement::branch>(
		std::move(condition),
		std::move(then_block),
		match<kw_else>() ? get_block() : nullptr
	) };

	if (declaration) {
		return std::make_unique<statement::scope>(utils::make_pointers_vector<statement>(
			std::move(declaration),
			std::move(branch)
		));
	}

	return std::move(branch);
}

auto parser::loop_stmt() -> std::unique_ptr<statement> {
	using enum token_type;

	consume(left_paren, "Expected '(' after 'while' statement", peek());

	auto declaration{ storage_declaration() };
	auto condition{ expr() };

	consume(right_paren, "Expected ')' after 'while' condition", peek(), error_code::pe_broken_symmetry);

	const auto get_block{ [this] {
		consume(left_brace, "'while' requires '{' block", peek());
		return scope_stmt();
	} };

	return make_loop(std::move(declaration), std::move(condition), get_block());
}

auto parser::for_loop_stmt() -> std::unique_ptr<statement> {
	using enum token_type;

	consume(left_paren, "Expected '(' after 'while' statement", peek());

	auto declaration{ make_declaration_or_expression_stmt() };
	auto condition{ match<semicolon>() ? std::make_unique<expression::literal>(true) : expr() };
	consume(semicolon, "Expected ';' after 'for' condition", peek(),
		error_code::pe_missing_end_of_statement
	);

	auto increment_expr{ match<right_paren>() ? nullptr : expr() };
	consume(right_paren, "Expected ')' after 'while' condition", peek(), error_code::pe_broken_symmetry);

	consume(left_brace, "'for' requires '{' block", peek());

	auto body{ scope_stmt() };
	if (increment_expr) {
		body->statements.emplace_back(std::make_unique<statement::expression>(std::move(increment_expr)));
	}
	return make_loop(std::move(declaration), std::move(condition), std::move(body));
}

auto parser::scope_stmt() -> std::unique_ptr<statement::scope> {
	using enum token_type;

	std::vector<std::unique_ptr<statement>> statements{};
	while (!at_end() && !utils::ct::any_from(right_brace, peek().type, previous().type)) {
		statements.emplace_back(declaration());
	}
	if (const auto &prev{ previous() }; prev.type == right_brace) {
		make_error("It seems like there should be ';' before '}'",
			error_code::pe_missing_end_of_statement, prev
		);
	} else {
		consume(right_brace, "Expected '}' after block", peek(),
			error_code::pe_broken_symmetry
		);
	}
	return std::make_unique<statement::scope>(std::move(statements));
}

auto parser::make_declaration_or_expression_stmt() -> std::unique_ptr<statement> {
	if (match<token_type::semicolon>()) {
		return nullptr;
	}

	if (auto declaration{ storage_declaration() }; declaration) {
		return std::move(declaration);
	}

	return make_stmt<statement::expression>(&parser::expr);
}

auto parser::make_loop(
	std::unique_ptr<statement> declaration,
	std::unique_ptr<expression> condition,
	std::unique_ptr<statement> body
) -> std::unique_ptr<statement> {
	auto loop{ std::make_unique<statement::loop>(std::move(condition), std::move(body)) };
	if (declaration) {
		return std::make_unique<statement::scope>(utils::make_pointers_vector<statement>(
			std::move(declaration),
			std::move(loop)
		));
	}

	return std::move(loop);
}

auto parser::expr() -> std::unique_ptr<expression> {
	return incdec();
}

auto parser::incdec() -> std::unique_ptr<expression> {
	using enum token_type;

	if (match<increment, decrement>()) {
		const auto &op{ previous() };
		auto identifier{ logical_or() };
		if (identifier->type() != expression::tag::identifier) {
			make_error(std::format("Invalid {} target.", token_name(op.type)),
				lox::error_code::pe_lvalue_assignment, op
			);
			return std::move(identifier);
		}
		const auto &name{ static_cast<const expression::identifier *>(identifier.get())->name };
		return std::make_unique<expression::incdec>(name, op);
	}

	return assignment();
}

auto parser::assignment() -> std::unique_ptr<expression> {
	using enum token_type;

	auto expr_{ logical_or() };

	static constexpr auto convert_to_binary_op{ [] (const auto type) {
		switch (type) {
			case equal: return equal;
			case plus_equal: return plus;
			case minus_equal: return minus;
			case star_equal: return star;
			case slash_equal: return slash;
			default: break;
		}
		return invalid;
	} };


	if (match<equal, plus_equal, minus_equal, star_equal, slash_equal>()) {
		const auto &equals_token{ previous() };
		auto value{ assignment() };
		if (expr_->type() != expression::tag::identifier) {
			make_error("Invalid assignment target.", lox::error_code::pe_lvalue_assignment, equals_token);
			return std::move(expr_);
		}

		const auto &name{ static_cast<const expression::identifier *>(expr_.get())->name };
		const auto operation_type{ convert_to_binary_op(equals_token.type) };
		if (operation_type == equal) {
			return std::make_unique<expression::assignment>(name, std::move(value));
		}

		return std::make_unique<expression::assignment>(name,
			std::make_unique<expression::binary>(token{
				.line = equals_token.line,
				.position = equals_token.position,
				.type = operation_type
			}, std::move(expr_), std::move(value))
		);
	}

	return expr_;
}

auto parser::logical_or() -> std::unique_ptr<expression> {
	return iterate_if<expression::logical, token_type::kw_or>(&parser::logical_and);
}

auto parser::logical_and() -> std::unique_ptr<expression> {
	return iterate_if<expression::logical, token_type::kw_and>(&parser::equality);
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
			case semicolon: [[fallthrough]];
			case right_brace:
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
