#include <cmath>
#include <limits>
#include <format>
#include <functional>

#if defined(LOX_DEBUG)
#include <cstdio>
#endif // defined(LOX_DEBUG)

#include <tl/expected.hpp>

#include "lox/execution/syntax_tree_interpreter.hpp"
#include "lox/utils/compile_time.hpp"

namespace lox::execution {

namespace {

using evaluation_result = tl::expected<literal, std::string>;

auto make_no_operator_error(token_type op, const literal &lhv, const literal &rhv) -> std::string {
	return std::format("No operator '{0}' for literals with types: '{1}' and '{2}':\n\t{3} {0} {4}",
		token_string(op),
		type_name(lhv.type()), type_name(rhv.type()),
		to_string(lhv), to_string(rhv)
	);
}

namespace traits {

template<token_type Type>
struct operation { using type = void; };

template<> struct operation<token_type::plus>          { using type = std::plus<>;               };
template<> struct operation<token_type::minus>         { using type = std::minus<>;              };
template<> struct operation<token_type::star>          { using type = std::multiplies<>;         };
template<> struct operation<token_type::slash>         { using type = std::divides<>;            };

template<> struct operation<token_type::equal_equal>   { using type = std::equal_to<>;           };
template<> struct operation<token_type::bang_equal>    { using type = std::not_equal_to<>;       };
template<> struct operation<token_type::less>          { using type = std::less<>;               };
template<> struct operation<token_type::less_equal>    { using type = std::less_equal<>;         };
template<> struct operation<token_type::greater>       { using type = std::greater<>;            };
template<> struct operation<token_type::greater_equal> { using type = std::greater_equal<>;      };

} // namespace traits

template<token_type Type, class Operation = traits::operation<Type>::type>
auto eval_numeric(const literal &lhv, const literal &rhv) -> std::optional<literal> {
	const Operation oper{};
	if (auto integer{ lhv.as<int64_t>() }; integer) {
		if (rhv.is(literal_type::integral)) {
			return oper(*integer, std::get<int64_t>(rhv));
		}
		if (rhv.is(literal_type::number)) {
			return oper(static_cast<double>(*integer), std::get<double>(rhv));
		}
	} else if (auto num{ lhv.as<double>() }; num) {
		if (rhv.is(literal_type::number)) {
			return oper(*num, std::get<double>(rhv));
		}
		if (rhv.is(literal_type::integral)) {
			return oper(*num, static_cast<double>(std::get<int64_t>(rhv)));
		}
	}
	return std::nullopt;
}



template<token_type Type, class Operation = traits::operation<Type>::type>
auto compare(const literal &lhv, const literal &rhv) noexcept -> evaluation_result {
	const Operation oper{};
	if (auto value{ eval_numeric<Type, Operation>(lhv, rhv) }; value.has_value()) {
		return std::move(value).value();
	} else if (lhv.is(literal_type::string) && rhv.is(literal_type::string)) {
		return oper(lhv, rhv);
	}

	return tl::make_unexpected(make_no_operator_error(Type, lhv, rhv));
}


template<token_type Operation>
struct operation {
	[[nodiscard]]
	static auto eval(const literal &lhv, const literal &rhv) noexcept -> evaluation_result {
		if constexpr (token_traits::is_arithmetic_v<Operation>) {
			if (auto value{ eval_numeric<Operation>(lhv, rhv) }; value.has_value()) {
				return std::move(value).value();
			}

			return tl::make_unexpected(make_no_operator_error(Operation, lhv, rhv));
		} else if constexpr (token_traits::is_logical_v<Operation>) {
			return compare<Operation>(lhv, rhv);
		} else {
			return tl::make_unexpected(std::format("Unsupported token '{}' (aka '{}')",
				token_string(Operation), token_name(Operation)
			));
		}
	}
};

template<>
struct operation<token_type::plus> {
	[[nodiscard]]
	static auto eval(const literal &lhv, const literal &rhv) noexcept -> evaluation_result {
		if (auto value{ eval_numeric<token_type::plus>(lhv, rhv) }; value.has_value()) {
			return std::move(value).value();
		}

		if (lhv.is(literal_type::string) && rhv.is(literal_type::string)) {
			return concat(std::get<std::string>(lhv), std::get<std::string>(rhv));
		}

		return tl::make_unexpected(make_no_operator_error(token_type::plus, lhv, rhv));
	}

	[[nodiscard]]
	static auto concat(std::string_view lhv, std::string_view rhv) -> std::string {
		std::string result(std::size(lhv) + std::size(rhv), '\0');
		auto [_, pos]{ std::ranges::copy(lhv, std::begin(result)) };
		std::ranges::copy(rhv, pos);
		return result;
	}
};

} // namespace


syntax_tree_interpreter::syntax_tree_interpreter(const lexeme_database &lexemes, error_handler &handler) noexcept
	: lexemes{ lexemes }, errout{ handler } {}

auto syntax_tree_interpreter::run(program &prog) -> status try {
	for (const auto &stmt : prog) {
		if (!stmt) [[unlikely]] {
			return status::invalid_program;
		}
		execute(*stmt);
	}
	return status::ok;
} catch (const execution_error &) {
	return status::runtime_error;
}

auto syntax_tree_interpreter::evaluate(expression &expr) -> literal {
	m_output = {};
	try {
		expr.accept(*this);
	} catch (const execution_error &err) {
		got_runtime_error = true;
	}
	return m_output;
}

void syntax_tree_interpreter::execute(statement &stmt) {
	stmt.accept(*this);
}

#pragma region expression::visitor_interface methods

void syntax_tree_interpreter::accept(const expression::unary &unary) {
	if (!unary.expr) {
		error(error_code::ee_missing_expression, "", unary.op);
	}

	auto value{ evaluate(*unary.expr) };
	if (!is_suitable_for(unary.op.type, value.type())) {
		throw error_no_suitable(unary.op, value);
	}

	switch (unary.op.type) {
		using enum token_type;

		case plus: break;
		case minus: if (!negate_number(value)) value = {}; break;
		case bang:  if (!inverse_boolean(value)) value = {}; break;

		default:
			return;
	}

	m_output = std::move(value);
}

void syntax_tree_interpreter::accept(const expression::incdec &incdec) {
	const auto id{ incdec.name.lexeme_id };

	if (!m_env.contains(id, execution::search_range::globally)) {
		error(error_code::ee_undefined_identifier, std::format(
			R"(Undefined variable "{}")", lexemes.get().get(id)
		), incdec.name);
	}

	const auto &value{ m_env.look_up(id) };
	if (!is_suitable_for(incdec.op.type, value.type())) {
		throw error_no_suitable(incdec.op, value);
		return;
	}

	auto assign_output{ [&name{ incdec.name }, this] (auto value) {
		safe_assign(name, std::move(value));
		return evaluation_result{};
	} };
	auto throw_error{ [this, &op{ incdec.op }] (const auto &msg) {
		throw error(error_code::ee_runtime_error, msg, op);
	} };

	[op{ incdec.op.type }, &value] {
		switch (op) {
			using enum token_type;
			case increment: return operation<plus>::eval(value, literal{ 1 });
			case decrement: return operation<minus>::eval(value, literal{ 1 });
			default: break;
		}

		return evaluation_result{ tl::make_unexpected(
			std::format("Unknown operation '{}' ({})", token_string(op), token_name(op))
		) };
	}()
		.and_then(std::move(assign_output))
		.or_else(std::move(throw_error))
	;
}

void syntax_tree_interpreter::accept(const expression::assignment &assign) {
	safe_assign(assign.name, evaluate(*assign.value));
}

void syntax_tree_interpreter::safe_assign(const token &tok, literal value) {
	switch (m_env.assign(tok.lexeme_id, std::move(value))) {
		using enum environment::assignment_status;

		case not_found:
			error(error_code::ee_undefined_identifier, std::format(
				R"(Undefined variable "{}")", lexemes.get().get(tok.lexeme_id)
			), tok);
			break;

		case constant:
			error(error_code::ee_constant_assignment, std::format(
				R"(Attempt to assign "{}" constant)", lexemes.get().get(tok.lexeme_id)
			), tok);
			break;

		default: break;
	}
}

void syntax_tree_interpreter::accept(const expression::binary &expr) {
	if (!expr.left || !expr.right) {
		error(error_code::ee_missing_expression, "", expr.op);
	}

	auto lhv{ evaluate(*expr.left) };
	auto rhv{ evaluate(*expr.right) };
	if (!is_suitable_for(expr.op.type, lhv.type(), rhv.type())) {
		throw error_no_suitable(expr.op, lhv, rhv);
	}

	auto assign_output{ [&out{ m_output }] (auto value) {
		out = std::move(value);
		return evaluation_result{};
	} };
	auto throw_error{ [this, &op{ expr.op }] (const auto &msg) {
		throw error(error_code::ee_runtime_error, msg, op);
	} };

	[op{ expr.op.type }, &lhv, &rhv] {
		switch (op) {
			using enum token_type;

			case plus:  return operation<plus>::eval(lhv, rhv);
			case minus: return operation<minus>::eval(lhv, rhv);
			case star:  return operation<star>::eval(lhv, rhv);
			case slash: return operation<slash>::eval(lhv, rhv);

			case equal_equal:   return operation<equal_equal>::eval(lhv, rhv);
			case bang_equal:    return operation<bang_equal>::eval(lhv, rhv);
			case less:          return operation<less>::eval(lhv, rhv);
			case less_equal:    return operation<less_equal>::eval(lhv, rhv);
			case greater:       return operation<greater>::eval(lhv, rhv);
			case greater_equal: return operation<greater_equal>::eval(lhv, rhv);

			default: break;
		}

		return evaluation_result{ tl::make_unexpected(
			std::format("Unknown operation '{}' ({})", token_string(op), token_name(op))
		) };
	}()
		.and_then(std::move(assign_output))
		.or_else(std::move(throw_error))
	;
}

void syntax_tree_interpreter::accept(const expression::grouping &group) {
	if (group.expr) {
		m_output = evaluate(*group.expr);
	} else {
		error(error_code::ee_missing_expression, "");
	}
}

void syntax_tree_interpreter::accept(const expression::literal &value) {
	m_output = value.value;
}

void syntax_tree_interpreter::accept(const expression::logical &logic) {
	m_output = evaluate(*logic.left);

	const auto truth{ is_truth(m_output) };
	if (!truth.has_value()) {
		error(lox::error_code::ee_condition_is_not_logical,
			"Non-logical expression couldn't be used", logic.op
		);
		m_output = null_literal;
		return;
	}

	switch (logic.op.type) {
		case token_type::kw_or:
			if (truth.value()) return;
			break;

		case token_type::kw_and:
			if (!truth.value()) return;
			break;

		default:
			// error(lox::error_code::ee_)
			return;
	}

	m_output = evaluate(*logic.right);
}

void syntax_tree_interpreter::accept(const expression::identifier &id) {
	if (m_env.contains(id.name.lexeme_id, execution::search_range::globally)) {
		m_output = m_env.look_up(id.name.lexeme_id);
		return;
	}

	error(error_code::ee_undefined_identifier, std::format(
		R"(Undefined identifier "{}")", lexemes.get().get(id.name.lexeme_id)
	), id.name);
}

#pragma endregion expression::visitor_interface methods

#pragma region statement::visitor_interface methods

void syntax_tree_interpreter::accept(const statement::scope &scope) {
	execute_block(scope.statements);
}

void syntax_tree_interpreter::accept(const statement::expression &expr) {
	if (expr.expr) {
		m_output = evaluate(*expr.expr);
	} else {
		error(error_code::ee_missing_expression, "");
	}
}

void syntax_tree_interpreter::accept(const statement::branch &branch) {
	if (auto truth{ is_truth(evaluate(*branch.condition)) }; truth.has_value()) {
		if (truth.value()) {
			if (branch.then_branch) execute(*branch.then_branch);
		} else {
			if (branch.else_branch) execute(*branch.else_branch);
		}
	} else {
		error(lox::error_code::ee_condition_is_not_logical,
			"The condition of branch couldn't be converted to boolean type!"
		);
	}
}

void syntax_tree_interpreter::accept(const statement::variable &var) {
	if (m_env.contains(var.identifier.lexeme_id)) {
		error(error_code::ee_identifier_already_exists, std::format(
			R"(Variable "{}" is already defined)", lexemes.get().get(var.identifier.lexeme_id)
		), var.identifier);
	}
	std::ignore = m_env.define_variable(var.identifier.lexeme_id,
		var.initializer ? evaluate(*var.initializer) : null_literal
	);
}

void syntax_tree_interpreter::accept(const statement::constant &con) {
	if (m_env.contains(con.identifier.lexeme_id)) {
		error(error_code::ee_identifier_already_exists, std::format(
			R"(Constant "{}" is already defined)", lexemes.get().get(con.identifier.lexeme_id)
		), con.identifier);
	}
	if (!con.initializer) {
		error(error_code::ee_missing_expression, std::format(
			R"(Constant "{}" wasn't initialized)", lexemes.get().get(con.identifier.lexeme_id)
		), con.identifier);
	}

	std::ignore = m_env.define_constant(con.identifier.lexeme_id, evaluate(*con.initializer));
}

void syntax_tree_interpreter::accept(const statement::loop &loop) {
	const auto make_condition{ [this, &loop] { return is_truth(evaluate(*loop.condition)); }};

	auto cond{ make_condition() };
	if (loop.body) {
		for (; cond.value_or(false); cond = make_condition()) {
			execute(*loop.body);
		}
	} else {
		while (cond.value_or(false)) {
			cond = make_condition();
		}
	}

	if (!cond.has_value()) {
		error(lox::error_code::ee_condition_is_not_logical,
			"The condition of 'while' loop couldn't be converted to boolean type!"
		);
		return;
	}
}

#if defined(LOX_DEBUG)

void syntax_tree_interpreter::accept(const statement::print &print) {
	if (print.expr) {
		m_output = evaluate(*print.expr);
		std::fprintf(stdout, "%s\n", std::data(lox::to_string(m_output)));
	} else {
		error(error_code::ee_missing_expression, "");
	}
}

#endif // defined(LOX_DEBUG)


#pragma endregion statement::visitor_interface methods

void syntax_tree_interpreter::execute_block(const std::vector<std::unique_ptr<statement>> &statements) {
	m_env.push_scope();
	try {
		for (const auto &statement : statements) {
			if (statement) execute(*statement);
		}
	} catch (...) {}
	m_env.pop_scope();
}

auto syntax_tree_interpreter::error_no_suitable(const token &op, const literal &value) const -> execution_error {
	return error(error_code::ee_literal_not_suitable_for_operation,
		std::format("Value '{}' is not suitable for '{}' ('{}') unary operation",
			to_string(value), token_string(op.type), token_name(op.type)
		), op
	);
}

auto syntax_tree_interpreter::error_no_suitable(const token &op, const literal &lhv, const literal &rhv) const -> execution_error {
	return error(error_code::ee_literal_not_suitable_for_operation,
		make_no_operator_error(op.type, lhv, rhv), op
	);
}

auto syntax_tree_interpreter::error(error_code err_no, std::string_view msg, const token &tok) const -> execution_error {
	errout.get().report(msg, error_record{
		.code = err_no,
		.line = tok.line,
		.from = tok.position,
		.to   = static_cast<uint32_t>(std::size(token_string(tok.type)))
	});
	return execution_error{ std::data(msg) };
}

auto syntax_tree_interpreter::error(error_code err_no, std::string_view msg) const -> execution_error {
	errout.get().report(msg, error_record{
		.code = err_no
	});
	return execution_error{ std::data(msg) };
}

auto syntax_tree_interpreter::is_suitable_for(token_type op, literal_type type) -> bool {
	/* THIS IS FOR UNARY OPERATIONS */

	using enum literal_type;
	switch (op) {
		case token_type::plus: [[fallthrough]];
		case token_type::minus: [[fallthrough]];
		case token_type::slash: [[fallthrough]];
		case token_type::star:
			return utils::ct::any_from<number, integral>(type);

		case token_type::increment: [[fallthrough]];
		case token_type::decrement:
			return type == integral;

		case token_type::bang: [[fallthrough]];
		case token_type::bang_equal: [[fallthrough]];
		case token_type::equal_equal:
			return true;

		default: break;
	}

	return false;
}

auto syntax_tree_interpreter::is_suitable_for(token_type op, literal_type lhv, literal_type rhv) -> bool {
	/* THIS IS FOR BINARY OPERATIONS */
	using enum literal_type;

	const auto both_numbers{
		utils::ct::any_from<number, integral>(lhv) &&
		utils::ct::any_from<number, integral>(rhv)
	};

	switch (op) {
		case token_type::minus: [[fallthrough]];
		case token_type::slash: [[fallthrough]];
		case token_type::star:
			return both_numbers;

		case token_type::plus:
			return both_numbers || (lhv == literal_type::string && rhv == lhv);

		case token_type::bang: [[fallthrough]];
		case token_type::bang_equal: [[fallthrough]];
		case token_type::equal_equal: [[fallthrough]];
		case token_type::less: [[fallthrough]];
		case token_type::less_equal: [[fallthrough]];
		case token_type::greater: [[fallthrough]];
		case token_type::greater_equal:
			return lhv == rhv || both_numbers;

		default: break;
	}

	return false;
}

auto syntax_tree_interpreter::is_truth(const literal &value) -> std::optional<bool> {
	if (value.is(literal_type::null)) {
		return false;
	}
	if (auto boolean{ value.as<bool>() }; boolean) {
		return *boolean;
	}
	if (auto real_number{ value.as<double>() }; real_number) {
		return std::abs(*real_number) > std::numeric_limits<double>::epsilon();
	}
	if (auto integer{ value.as<int64_t>() }; integer) {
		return *integer != 0ll;
	}
	if (auto str{ value.as<std::string>() }; str) {
		return !std::empty(*str);
	}
	return std::nullopt;
}

auto syntax_tree_interpreter::negate_number(literal &value) -> bool {
	if (auto real_number{ value.as<double>() }; real_number) {
		*real_number = -*real_number;
		return true;
	}
	if (auto integer{ value.as<int64_t>() }; integer) {
		*integer = -*integer;
		return true;
	}
	return false;
}

auto syntax_tree_interpreter::inverse_boolean(literal &value) -> bool {
	if (auto res{ is_truth(value) }; res.has_value()) {
		value = res.value();
		return true;
	}
	return false;
}


} // namespace lox::execution
