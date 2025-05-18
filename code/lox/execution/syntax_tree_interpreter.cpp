#include <cmath>
#include <limits>
#include <format>
#include <functional>

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


syntax_tree_interpreter::syntax_tree_interpreter(file_id file, error_handler &handler) noexcept
	: errout{ handler }, m_file_id{ file } {}


auto syntax_tree_interpreter::evaluate(const std::unique_ptr<expression> &expr) -> literal {
	m_output = {};
	if (expr) try {
		expr->accept(*this);
	} catch (const execution_error &err) {
		got_runtime_error = true;
	}
	return m_output;
}

void syntax_tree_interpreter::accept(const expression::unary &unary) {
	auto value{ evaluate(unary.expr) };
	if (!is_suitable_for(unary.op.type, value.type())) {
		throw error_no_suitable(unary.op.type, value);
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

void syntax_tree_interpreter::accept(const expression::binary &expr) {
	auto lhv{ evaluate(expr.left) };
	auto rhv{ evaluate(expr.right) };
	if (!is_suitable_for(expr.op.type, lhv.type(), rhv.type())) {
		throw error_no_suitable(expr.op.type, lhv, rhv);
	}

	auto assign_output{ [&out{ m_output }] (auto value) {
		out = std::move(value);
		return evaluation_result{};
	} };
	auto throw_error{ [this] (const auto &msg) {
		throw error(error_code::ee_runtime_error, msg);
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
	m_output = evaluate(group.expr);
}

void syntax_tree_interpreter::accept(const expression::literal &value) {
	m_output = value.value;
}

auto syntax_tree_interpreter::error_no_suitable(token_type op, const literal &value) const -> execution_error {
	return error(error_code::ee_literal_not_suitable_for_operation,
		std::format("Value '{}' is not suitable for '{}' ('{}') unary operation",
			to_string(value), token_string(op), token_name(op)
		)
	);
}

auto syntax_tree_interpreter::error_no_suitable(token_type op, const literal &lhv, const literal &rhv) const -> execution_error {
	return error(error_code::ee_literal_not_suitable_for_operation,
		make_no_operator_error(op, lhv, rhv)
	);
}

auto syntax_tree_interpreter::error(error_code err_no, std::string_view msg) const -> execution_error {
	errout.report(msg, error_record{
		.code = err_no,
		.file_id = m_file_id
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
		case token_type::plus:
		case token_type::minus: [[fallthrough]];
		case token_type::slash: [[fallthrough]];
		case token_type::star:
			return both_numbers;

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
