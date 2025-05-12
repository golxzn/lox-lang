#pragma once

#include <string>
#include "lox/expression.hpp"

namespace lox::utils {

class ast_printer final : public expression::visitor_interface {
public:
	~ast_printer() override = default;

	[[nodiscard]] auto print(std::unique_ptr<expression> &expr) -> std::string;

	void accept(expression::unary &expr) override;
	void accept(expression::binary &expr) override;
	void accept(expression::grouping &expr) override;
	void accept(expression::literal &expr) override;

private:
	std::string value;

	template<class ...Exprs>
	void parenthesize(std::string_view name, Exprs &...expressions) {
		value.append("(");
		value.append(name);

		([this] (Exprs &expr) {
			if (expr) {
				value.append(" ");
				expr->accept(*this);
			}
		} (expressions), ...);

		value.append(")");
	}
};

} // namespace lox::utils
