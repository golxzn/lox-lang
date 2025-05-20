#pragma once

#include <string>
#include "lox/gen/expression.hpp"

namespace lox::utils {

class LOX_EXPORT ast_printer final : public expression::visitor_interface {
public:
	~ast_printer() override = default;

	[[nodiscard]] auto print(const std::unique_ptr<expression> &expr) -> std::string;

	void accept(const expression::unary &expr) override;
	void accept(const expression::binary &expr) override;
	void accept(const expression::grouping &expr) override;
	void accept(const expression::literal &expr) override;

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
