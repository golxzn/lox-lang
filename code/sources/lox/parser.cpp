#include <format>
#include <ranges>
#include <algorithm>

#include "lox/parser.hpp"

namespace lox {

parser::parser(std::vector<token> tokens, std::vector<literal> literals, error_handler &errs) noexcept
	: m_tokens{ std::move(tokens) }
	, m_literals{ std::move(literals) }
	, errout{ errs }
{}

auto parser::parse() -> program {
	program prog{};
	while (!at_end()) {
		prog.emplace_back(stmt());
	}
	return prog;
}

auto parser::stmt() -> std::unique_ptr<statement> {
#if defined(LOX_DEBUG)
	if (match<token_type::kw_print>()) {
		return make_stmt<statement::print>(&parser::expr);
	}
#endif // defined(LOX_DEBUG)

	return make_stmt<statement::expression>(&parser::expr);
}

auto parser::expr() -> std::unique_ptr<expression> {
	return equality();
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
		if (id < std::size(m_literals)) {
			return std::make_unique<expression::literal>(m_literals.at(id));
		}
		throw make_error(
			std::format(R"(Missing literal #{} of the "{}" token!)",
				id, token_name(token.type)
			),
			error_code::pe_missing_literal, token
		);
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

void parser::consume(token_type type, std::string_view on_error, const token &tok, error_code code) {
	if (!check(type)) {
		make_error(on_error, code, tok);
	}
	advice();
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
	return m_tokens.at(m_current);
}

auto parser::previous() const -> const token & {
	return m_tokens.at(m_current - 1ull);
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
