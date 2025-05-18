#pragma once

#include <optional>
#include <stdexcept>
#include "lox/expression.hpp"
#include "lox/error_handler.hpp"

namespace lox::execution {

class syntax_tree_interpreter final : public expression::visitor_interface {
public:
	struct execution_error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	syntax_tree_interpreter(file_id file, error_handler &handler) noexcept;
	~syntax_tree_interpreter() override = default;

	[[nodiscard]] auto evaluate(std::unique_ptr<expression> &expr) -> literal;
	[[nodiscard]] auto runtime_error() const noexcept -> bool { return got_runtime_error; }

	void accept(expression::unary &unary) override;
	void accept(expression::binary &binary) override;
	void accept(expression::grouping &group) override;
	void accept(expression::literal &value) override;

private:
	literal m_output{};
	error_handler &errout;
	file_id m_file_id;
	bool got_runtime_error{ false };

	auto error_no_suitable(token_type op, const literal &value) const -> execution_error;
	auto error_no_suitable(token_type op, const literal &lhv, const literal &rhv) const -> execution_error;
	auto error(error_code err_no, std::string_view msg) const -> execution_error;

	static auto is_suitable_for(token_type op, literal_type type) -> bool;
	static auto is_suitable_for(token_type op, literal_type lhv, literal_type rhv) -> bool;

	static auto is_truth(const literal &value) -> std::optional<bool>;
	static auto negate_number(literal &value) -> bool;
	static auto inverse_boolean(literal &value) -> bool;
};

} // namespace lox::execution
