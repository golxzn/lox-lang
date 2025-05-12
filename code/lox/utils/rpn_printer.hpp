#pragma once

#include <string>
#include "lox/expression.hpp"

namespace lox::utils {

class rpn_printer final : public expression::visitor_interface {
public:
	~rpn_printer() override = default;

	[[nodiscard]] auto print(std::unique_ptr<expression> &expr) -> std::string;

	void accept(expression::unary &expr) override;
	void accept(expression::binary &expr) override;
	void accept(expression::grouping &expr) override;
	void accept(expression::literal &expr) override;

private:
	std::string value;
};

} // namespace lox::utils

