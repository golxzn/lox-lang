#pragma once

#include <string>
#include "lox/gen/expression.hpp"

namespace lox::utils {

class rpn_printer final : public expression::visitor_interface {
public:
	~rpn_printer() override = default;

	[[nodiscard]] auto print(const std::unique_ptr<expression> &expr) -> std::string;

	void accept(const expression::unary &expr) override;
	void accept(const expression::binary &expr) override;
	void accept(const expression::grouping &expr) override;
	void accept(const expression::literal &expr) override;

private:
	std::string value;
};

} // namespace lox::utils

