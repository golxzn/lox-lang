#include "lox/utils/rpn_printer.hpp"

namespace lox::utils {

auto rpn_printer::print(const std::unique_ptr<expression> &expr) -> std::string {
	value.clear();
	expr->accept(*this);
	return std::move(value);
}

void rpn_printer::accept(const expression::unary &expr) {
	expr.expr->accept(*this);
	value += token_name(expr.op.type);
	value += " ";
}

void rpn_printer::accept(const expression::binary &expr) {
	expr.left->accept(*this);
	expr.right->accept(*this);
	value += token_name(expr.op.type);
	value += " ";
}

void rpn_printer::accept(const expression::grouping &expr) {
	expr.expr->accept(*this);
}

void rpn_printer::accept(const expression::literal &expr) {
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
	value += " ";
}




} // namespace lox::utils
