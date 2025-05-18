#include "lox/utils/ast_printer.hpp"

namespace lox::utils {

auto ast_printer::print(std::unique_ptr<expression> &expr) -> std::string {
	value.clear();
	if (expr) expr->accept(*this);
	return std::move(value);
}

void ast_printer::accept(expression::unary &expr) {
	parenthesize(token_name(expr.op.type), expr.expr);
}

void ast_printer::accept(expression::binary &expr) {
	parenthesize(token_name(expr.op.type), expr.left, expr.right);
}

void ast_printer::accept(expression::grouping &expr) {
	parenthesize("group", expr.expr);

}

void ast_printer::accept(expression::literal &expr) {
	if (auto str{ expr.value.as<std::string>() }; str != nullptr) {
		value.append(*str);
	} else if (auto b{ expr.value.as<bool>() }; b != nullptr) {
		value.append(*b ? "true" : "false");
	} else if (auto i{ expr.value.as<int64_t>() }; i != nullptr) {
		value.append(std::to_string(*i));
	} else if (auto d{ expr.value.as<double>() }; d != nullptr) {
		value.append(std::to_string(*d));
	} else {
		value.append("null");
	}
}



} // namespace lox::utils
