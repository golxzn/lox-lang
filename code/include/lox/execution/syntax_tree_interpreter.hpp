#pragma once

#include <optional>
#include <stdexcept>

#include "lox/program.hpp"
#include "lox/error_handler.hpp"

namespace lox::execution {

enum class status : uint8_t {
	ok,
	invalid_program,
	runtime_error,
};


class LOX_EXPORT syntax_tree_interpreter final
	: public expression::visitor_interface
	, public statement::visitor_interface {
public:
	struct execution_error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	explicit syntax_tree_interpreter(error_handler &handler) noexcept;
	~syntax_tree_interpreter() override = default;

	[[nodiscard]] auto run(program &prog) -> status;
	[[nodiscard]] auto evaluate(expression &expr) -> literal;
	[[nodiscard]] auto runtime_error() const noexcept -> bool { return got_runtime_error; }

	void execute(statement &stmt);

#pragma region expression::visitor_interface methods

	void accept(const expression::unary &unary) override;
	void accept(const expression::binary &binary) override;
	void accept(const expression::grouping &group) override;
	void accept(const expression::literal &value) override;

#pragma endregion expression::visitor_interface methods

#pragma region statement::visitor_interface methods

	void accept(const statement::expression &expr) override;

#if defined(LOX_DEBUG)
	void accept(const statement::print &print) override;
#endif // defined(LOX_DEBUG)

#pragma endregion statement::visitor_interface methods


private:
	literal m_output{};
	error_handler &errout;
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
