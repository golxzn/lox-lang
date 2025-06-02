#include <cmath>
#include <limits>
#include <format>
#include <algorithm>
#include <functional>

#if defined(LOX_DEBUG)
#include <cstdio>
#endif // defined(LOX_DEBUG)

#include <tl/expected.hpp>

#include "lox/execution/interpreter.hpp"
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


interpreter::interpreter(
	environment env,
	const program &prog,
	const lexeme_database &lexemes,
	error_handler &handler
) noexcept : m_env{ std::move(env) }, prog{ prog }, lexemes{ lexemes }, errout{ handler } {}

auto interpreter::run() -> status try {
	for (const auto &stmt : prog.get()) {
		execute(stmt);
	}
	return status::ok;
} catch (const execution_error &err) {
	return err.reason;
}

void interpreter::execute(statement_id stmt) {
	if (std::empty(stmt)) return; /// @todo log error
	// if (!prog.get().contains(stmt)) return;

	prog.get().accept(*this, stmt);
}

auto interpreter::evaluate(expression_id expr) -> literal try {
	if (std::empty(expr)) return null_literal; /// @todo log error

	return prog.get().accept(*this, expr);
} catch (const execution_error &) {
	return null_literal;
}

#pragma region expression::visitor_interface methods

auto interpreter::accept(const expression_unary &unary) -> literal {
	if (std::empty(unary.expr)) {
		error(error_code::ee_missing_expression, "", unary.op);
	}

	auto value{ evaluate(unary.expr) };
	if (!is_suitable_for(unary.op.type, value.type())) {
		throw error_no_suitable(unary.op, value);
	}

	switch (unary.op.type) {
		using enum token_type;

		case plus: break;
		case minus: if (negate_number(value)) return value; break;
		case bang:  if (inverse_boolean(value)) return value; break;

		default:
			return null_literal;
	}

	return null_literal;
}

auto interpreter::accept(const expression_call &call) -> literal {
	const auto caller_address{ evaluate(call.caller) };
	if (!caller_address.is(literal_type::integral)) {
		/// @todo info
		throw error(error_code::ee_invalid_callable, "Invalid callable expression", call.paren);
	}

	auto function{ m_env.function_at(std::get<int64_t>(caller_address)) };
	if (!function.has_value()) {
		throw error(error_code::ee_callable_not_found, "Cannot find function", call.paren);
	}

	if (!function->enough_arguments_count(std::size(call.args))) {
		const auto arity{ function->arity() };
		throw error(error_code::ee_invalid_arguments_cound,
			std::format("Invalid count of arguments. Expected {}, but got {}",
				(arity.has_value() ? std::to_string(*arity) : "variadic amount"), std::size(call.args)
			),
			call.paren
		);
	}

	std::vector<literal> params(std::size(call.args));
	for (size_t i{}; i < std::size(params); ++i) {
		params[i] = evaluate(call.args[i]);
	}

	m_env.push_scope();
	/// @todo aw shit. But there's no plans on this project anyway lol.
	std::shared_ptr<void> defer{ nullptr, [&env{ m_env }](...){ env.pop_scope(); } };
	return function->call(this, std::move(params));
}

auto interpreter::accept(const expression_incdec &incdec) -> literal {
	const auto id{ incdec.name.lexeme_id };

	if (!m_env.contains(id, execution::search_range::globally)) {
		error(error_code::ee_undefined_identifier, std::format(
			R"(Undefined variable "{}")", lexemes.get().get(id)
		), incdec.name);
	}

	const auto &value{ m_env.look_up(id) };
	if (!is_suitable_for(incdec.op.type, value.type())) {
		throw error_no_suitable(incdec.op, value);
		return null_literal;
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

auto interpreter::accept(const expression_assignment &assign) -> literal {
	const auto literal{ evaluate(assign.value) };
	safe_assign(assign.name, literal);
	return literal;
}

auto interpreter::accept(const expression_binary &expr) -> literal {
	if (std::empty(expr.left) || std::empty(expr.right)) {
		/// @todo Add description
		error(error_code::ee_missing_expression, "", expr.op);
	}

	auto lhv{ evaluate(expr.left) };
	auto rhv{ evaluate(expr.right) };
	if (!is_suitable_for(expr.op.type, lhv.type(), rhv.type())) {
		throw error_no_suitable(expr.op, lhv, rhv);
	}

	auto throw_error{ [this, &op{ expr.op }] (const auto &msg) {
		throw error(error_code::ee_runtime_error, msg, op);
	} };

	return [op{ expr.op.type }, &lhv, &rhv] {
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
		.or_else(std::move(throw_error))
		.value_or(null_literal)
	;
}

auto interpreter::accept(const expression_grouping &group) -> literal {
	if (!std::empty(group.expr)) {
		return evaluate(group.expr);
	} else {
		error(error_code::ee_missing_expression, "");
	}
}

auto interpreter::accept(const expression_literal &value) -> literal {
	return value.value;
}

auto interpreter::accept(const expression_logical &logic) -> literal {
	const auto result{ evaluate(logic.left) };
	const auto truth{ is_truth(result) };
	if (!truth.has_value()) {
		error(lox::error_code::ee_condition_is_not_logical,
			"Non-logical expression couldn't be used", logic.op
		);
		return null_literal;
	}

	switch (logic.op.type) {
		case token_type::kw_or:
			if (truth.value()) return null_literal;
			break;

		case token_type::kw_and:
			if (!truth.value()) return null_literal;
			break;

		default:
			/// @todo error message
			// error(lox::error_code::ee_)
			return null_literal;
	}

	return evaluate(logic.right);
}

auto interpreter::accept(const expression_identifier &id) -> literal {
	if (m_env.contains(id.name.lexeme_id, execution::search_range::globally)) {
		return m_env.look_up(id.name.lexeme_id);
	}

	error(error_code::ee_undefined_identifier, std::format(
		R"(Undefined identifier "{}")", lexemes.get().get(id.name.lexeme_id)
	), id.name);
	return null_literal;
}

#pragma endregion expression::visitor_interface methods

#pragma region statement::visitor_interface methods

void interpreter::accept(const statement_scope &scope) {
	execute_block(scope.statements);
}

void interpreter::accept(const statement_expression &expr) {
	if (!std::empty(expr.expr)) {
		evaluate(expr.expr);
	} else {
		error(error_code::ee_missing_expression, "");
	}
}

void interpreter::accept(const statement_function &func) {
	m_env.register_function(func.name.lexeme_id, function{ [this, func](void *, std::span<const literal> args) {
		for (size_t i{}; i < std::size(args); ++i) {
			m_env.define_variable(func.params[i].lexeme_id, args[i]);
		}

		try {
			execute(func.body);
		} catch (literal output) {
			return output;
		}

		return null_literal;
	}, std::size(func.params) });
}

void interpreter::accept(const statement_branch &branch) {
	if (auto truth{ is_truth(evaluate(branch.condition)) }; truth.has_value()) {
		if (truth.value()) {
			if (!std::empty(branch.then_branch)) execute(branch.then_branch);
		} else {
			if (!std::empty(branch.else_branch)) execute(branch.else_branch);
		}
	} else {
		error(lox::error_code::ee_condition_is_not_logical,
			"The condition of branch couldn't be converted to boolean type!"
		);
	}
}

void interpreter::accept(const statement_variable &var) {
	if (m_env.contains(var.identifier.lexeme_id)) {
		error(error_code::ee_identifier_already_exists, std::format(
			R"(Variable "{}" is already defined)", lexemes.get().get(var.identifier.lexeme_id)
		), var.identifier);
	}
	std::ignore = m_env.define_variable(var.identifier.lexeme_id,
		!std::empty(var.initializer) ? evaluate(var.initializer) : null_literal
	);
}

void interpreter::accept(const statement_constant &con) {
	if (m_env.contains(con.identifier.lexeme_id)) {
		error(error_code::ee_identifier_already_exists, std::format(
			R"(Constant "{}" is already defined)", lexemes.get().get(con.identifier.lexeme_id)
		), con.identifier);
	}
	if (std::empty(con.initializer)) {
		error(error_code::ee_missing_expression, std::format(
			R"(Constant "{}" wasn't initialized)", lexemes.get().get(con.identifier.lexeme_id)
		), con.identifier);
	}

	std::ignore = m_env.define_constant(con.identifier.lexeme_id, evaluate(con.initializer));
}

void interpreter::accept(const statement_loop &loop) {
	const auto make_condition{ [this, &loop] { return is_truth(evaluate(loop.condition)); }};

	auto cond{ make_condition() };
	if (std::empty(loop.body)) {
		while (cond.value_or(false)) {
			cond = make_condition();
		}
	} else {
		for (; cond.value_or(false); cond = make_condition()) {
			execute(loop.body);
		}
	}

	if (!cond.has_value()) {
		error(lox::error_code::ee_condition_is_not_logical,
			"The condition of 'while' loop couldn't be converted to boolean type!"
		);
		return;
	}
}

#pragma endregion statement::visitor_interface methods

void interpreter::execute_block(const program::statement_list &statements) {
	m_env.push_scope();
	try {
		for (const auto &statement : statements) {
			execute(statement);
		}
	} catch (...) {}
	m_env.pop_scope();
}

void interpreter::safe_assign(const token &tok, literal value) {
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
auto interpreter::error_no_suitable(const token &op, const literal &value) const -> execution_error {
	return error(error_code::ee_literal_not_suitable_for_operation,
		std::format("Value '{}' is not suitable for '{}' ('{}') unary operation",
			to_string(value), token_string(op.type), token_name(op.type)
		), op
	);
}

auto interpreter::error_no_suitable(const token &op, const literal &lhv, const literal &rhv) const -> execution_error {
	return error(error_code::ee_literal_not_suitable_for_operation,
		make_no_operator_error(op.type, lhv, rhv), op
	);
}

auto interpreter::error(error_code err_no, std::string_view msg, const token &tok) const -> execution_error {
	errout.get().report(msg, error_record{
		.code = err_no,
		.line = tok.line,
		.from = tok.position,
		.to   = static_cast<uint32_t>(std::size(token_string(tok.type)))
	});
	return execution_error{ std::data(msg) };
}

auto interpreter::error(error_code err_no, std::string_view msg) const -> execution_error {
	errout.get().report(msg, error_record{
		.code = err_no
	});
	return execution_error{ std::data(msg) };
}

auto interpreter::is_suitable_for(token_type op, literal_type type) -> bool {
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

auto interpreter::is_suitable_for(token_type op, literal_type lhv, literal_type rhv) -> bool {
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

auto interpreter::is_truth(const literal &value) -> std::optional<bool> {
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

auto interpreter::negate_number(literal &value) -> bool {
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

auto interpreter::inverse_boolean(literal &value) -> bool {
	if (auto res{ is_truth(value) }; res.has_value()) {
		value = res.value();
		return true;
	}
	return false;
}


} // namespace lox::execution
