#include <format>
#include <ranges>
#include <algorithm>

#include "lox/parser.hpp"
#include "lox/constants.hpp"
#include "lox/utils/vecutils.hpp"
#include "lox/utils/compile_time.hpp"

namespace lox {

parser::parser(const context &ctx, error_handler &errs) noexcept
	: ctx{ ctx }
	, errout{ errs }
{}

auto parser::parse() -> program {
	program out{};

	while (!at_end()) {
		out.add_statement(declaration(out));
	}

	return out;
}

auto parser::declaration(program &prog) -> statement_id {
	try {
		if (auto storage{ storage_declaration(prog) }; !std::empty(storage)) {
			return storage;
		}

		return stmt(prog);
	} catch (const error &e) {
		synchronize();
	}

	return statement_id{};
}

auto parser::storage_declaration(program &prog) -> statement_id {
	if (match<token_type::kw_const, token_type::kw_var>()) {
		return previous().type == token_type::kw_const
			? constant_declaration(prog)
			: variable_declaration(prog)
		;
	}
	return statement_id{};
}

auto parser::variable_declaration(program &prog) -> statement_id {
	// var IDENTIFIER[: str][{ expression }];

	auto identifier{ consume(token_type::identifier, "Expected variable name", peek()) };

	expression_id expr_{};
	if (match<token_type::left_brace>()) {
		const auto &init_start{ peek() };
		if (!match<token_type::right_brace>()) {
			expr_ = expr(prog);
			consume(token_type::right_brace, "Missed '}' brace during variable initialization",
				init_start, error_code::pe_broken_symmetry
			);
		}
	}

	match<token_type::semicolon>(); // Skip it if presented. No one cares

	return prog.emplace<statement_type::variable>(identifier, expr_);
}

auto parser::constant_declaration(program &prog) -> statement_id {
	auto identifier{ consume(token_type::identifier, "Expected variable name", peek()) };

	consume(token_type::left_brace,
		"Missed initialization braces for constant! Constant have to be initialized",
		peek(), error_code::pe_missing_const_initialization
	);

	expression_id expr_{};
	if (!match<token_type::right_brace>()) {
		const auto &init_start{ peek() };
		if (!match<token_type::right_brace>()) {
			expr_ = expr(prog);
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
		expr_ = prog.emplace<expression_type::literal>(null_literal);
	}

	match<token_type::semicolon>();

	return prog.emplace<statement_type::constant>(identifier, expr_);
}

auto parser::stmt(program &prog) -> statement_id {
	if (match<token_type::kw_if>()) {
		return branch_stmt(prog);
	}
	if (match<token_type::kw_while>()) {
		return loop_stmt(prog);
	}
	if (match<token_type::kw_for>()) {
		return for_loop_stmt(prog);
	}
	if (match<token_type::left_brace>()) {
		return scope_stmt(prog);
	}

	return make_stmt<statement_type::expression>(prog, &parser::expr);
}

auto parser::branch_stmt(program &prog) -> statement_id {
	using enum token_type;

	consume(left_paren, "Expected '(' after 'if' statement", peek());

	auto declaration{ storage_declaration(prog) };
	auto condition{ expr(prog) };

	consume(right_paren, "Expected ')' after 'if' condition", peek(), error_code::pe_broken_symmetry);

	const auto get_block{ [this, &prog] {
		consume(left_brace, "Branch requires '{' block", peek());
		return scope_stmt(prog);
	} };

	auto then_block{ get_block() };
	auto branch{ prog.emplace<statement_type::branch>(
		condition, then_block,
		match<kw_else>() ? get_block() : statement_id{}
	) };

	if (!std::empty(declaration)) {
		return prog.emplace<statement_type::scope>(std::vector<statement_id>{ declaration, branch });
	}

	return branch;
}

auto parser::loop_stmt(program &prog) -> statement_id {
	using enum token_type;

	consume(left_paren, "Expected '(' after 'while' statement", peek());

	auto declaration{ storage_declaration(prog) };
	auto condition{ expr(prog) };

	consume(right_paren, "Expected ')' after 'while' condition", peek(), error_code::pe_broken_symmetry);

	const auto get_block{ [this, &prog] {
		consume(left_brace, "'while' requires '{' block", peek());
		return scope_stmt(prog);
	} };

	return make_loop(prog, declaration, condition, get_block());
}

auto parser::for_loop_stmt(program &prog) -> statement_id {
	using enum token_type;

	consume(left_paren, "Expected '(' after 'while' statement", peek());

	auto declaration{ make_declaration_or_expression_stmt(prog) };
	auto condition{ match<semicolon>() ? prog.emplace<expression_type::literal>(true) : expr(prog) };
	consume(semicolon, "Expected ';' after 'for' condition", peek(),
		error_code::pe_missing_end_of_statement
	);

	auto increment_expr{ match<right_paren>() ? expression_id{} : expr(prog) };
	consume(right_paren, "Expected ')' after 'while' condition", peek(), error_code::pe_broken_symmetry);

	consume(left_brace, "'for' requires '{' block", peek());

	auto body{ scope_stmt(prog) };
	if (!std::empty(increment_expr)) {
		prog.get_statements<statement_type::scope>(body).statements.emplace_back(
			prog.emplace<statement_type::expression>(increment_expr)
		);
	}
	return make_loop(prog, declaration, condition, body);
}

auto parser::scope_stmt(program &prog) -> statement_id {
	using enum token_type;

	std::vector<statement_id> statements{};
	while (!at_end() && !utils::ct::any_from(right_brace, peek().type, previous().type)) {
		statements.emplace_back(declaration(prog));
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
	return prog.emplace<statement_type::scope>(std::move(statements));
}

auto parser::make_declaration_or_expression_stmt(program &prog) -> statement_id {
	if (match<token_type::semicolon>()) {
		return statement_id{};
	}

	if (auto declaration{ storage_declaration(prog) }; !std::empty(declaration)) {
		return declaration;
	}

	return make_stmt<statement_type::expression>(prog, &parser::expr);
}

auto parser::make_loop(program &prog,
	statement_id declaration,
	expression_id condition,
	statement_id body
) -> statement_id {
	auto loop{ prog.emplace<statement_type::loop>(condition, body) };
	if (!std::empty(declaration)) {
		return prog.emplace<statement_type::scope>(std::vector<statement_id>{ declaration, loop });
	}

	return std::move(loop);
}

auto parser::expr(program &prog) -> expression_id {
	return incdec(prog);
}

auto parser::incdec(program &prog) -> expression_id {
	using enum token_type;

	if (match<increment, decrement>()) {
		const auto &op{ previous() };
		auto identifier{ logical_or(prog) };
		if (identifier.type != expression_type::identifier) {
			make_error(std::format("Invalid {} target.", token_name(op.type)),
				lox::error_code::pe_lvalue_assignment, op
			);
			return std::move(identifier);
		}
		const auto &name{ prog.get_expressions<expression_type::identifier>(identifier).name };
		return prog.emplace<expression_type::incdec>(name, op);
	}

	return assignment(prog);
}

auto parser::assignment(program &prog) -> expression_id {
	using enum token_type;

	auto expr_{ logical_or(prog) };

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
		auto value{ assignment(prog) };
		if (expr_.type != expression_type::identifier) {
			make_error("Invalid assignment target.", lox::error_code::pe_lvalue_assignment, equals_token);
			return expr_;
		}

		const auto &name{ prog.get_expressions<expression_type::identifier>(expr_).name };
		const auto operation_type{ convert_to_binary_op(equals_token.type) };
		if (operation_type == equal) {
			return prog.emplace<expression_type::assignment>(name, value);
		}

		return prog.emplace<expression_type::assignment>(name,
			prog.emplace<expression_type::binary>(token{
				.line = equals_token.line,
				.position = equals_token.position,
				.type = operation_type
			}, expr_, value)
		);
	}

	return expr_;
}

auto parser::logical_or(program &prog) -> expression_id {
	return iterate_if<expression_type::logical, token_type::kw_or>(prog, &parser::logical_and);
}

auto parser::logical_and(program &prog) -> expression_id {
	return iterate_if<expression_type::logical, token_type::kw_and>(prog, &parser::equality);
}

auto parser::equality(program &prog) -> expression_id {
	using enum token_type;
	return iterate_through<bang_equal, equal_equal>(prog, &parser::comparison);
}

auto parser::comparison(program &prog) -> expression_id {
	using enum token_type;
	return iterate_through<greater, greater_equal, less, less_equal>(prog, &parser::term);
}

auto parser::term(program &prog) -> expression_id {
	using enum token_type;
	return iterate_through<minus, plus>(prog, &parser::factor);
}

auto parser::factor(program &prog) -> expression_id {
	using enum token_type;
	return iterate_through<slash, star>(prog, &parser::unary);
}

auto parser::unary(program &prog) -> expression_id {
	if (match<token_type::bang, token_type::minus>()) {
		const auto &op{ previous() };
		return prog.emplace<expression_type::unary>(op, unary(prog));
	}
	return call(prog);
}

auto parser::call(program &prog) -> expression_id {
	auto expr_{ primary(prog) };

	// This infinity loop is here for support object notations later
	while (true) {
		if (match<token_type::left_paren>()) {
			expr_ = call_finish(prog, expr_);
		} else {
			break;
		}
	}

	return expr_;
}

auto parser::call_finish(program &prog, expression_id caller) -> expression_id {
	std::vector<expression_id> arguments{};
	if (!check(token_type::right_paren)) {
		bool many_args_logged{ false };
		do {
			if (!many_args_logged && std::size(arguments) > constants::max_call_stack_depth) {
				make_error("Too many arguments for this call!",
					error_code::pe_too_many_arguments, peek()
				);
				many_args_logged = true;
			}

			arguments.emplace_back(expr(prog));
		} while (match<token_type::comma>());
	}

	const auto &paren{ consume(token_type::right_paren,
		"Expected ')' after arguments.", previous(), error_code::pe_broken_symmetry
	) };
	return prog.emplace<expression_type::call>(paren, caller, std::move(arguments));
}

auto parser::primary(program &prog) -> expression_id {
	using enum token_type;

	if (match<string, number, boolean, null>()) {
		const auto &token{ previous() };
		const auto id{ token.literal_id };
		if (id < std::size(ctx.literals)) {
			return prog.emplace<expression_type::literal>(ctx.literals.at(id));
		}
		throw make_error(
			std::format(R"(Missing literal #{} of the "{}" token!)",
				id, token_name(token.type)
			),
			error_code::pe_missing_literal, token
		);
	}

	if (match<identifier>()) {
		return prog.emplace<expression_type::identifier>(previous());
	}

	if (match<left_paren>()) {
		const auto &token{ peek() };
		auto expr_{ expr(prog) };

		consume(right_paren, "Expected ')' after expression", token, error_code::pe_broken_symmetry);
		return prog.emplace<expression_type::grouping>(expr_);
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
