#pragma once

#include <optional>
#include <stdexcept>

#include "lox/program.hpp"
#include "lox/error_handler.hpp"
#include "lox/lexeme_database.hpp"
#include "lox/execution/environment.hpp"

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

	syntax_tree_interpreter(const lexeme_database &lexemes, error_handler &handler) noexcept;
	~syntax_tree_interpreter() override = default;

	[[nodiscard]] auto run(program &prog) -> status;
	[[nodiscard]] auto evaluate(expression &expr) -> literal;
	[[nodiscard]] auto runtime_error() const noexcept -> bool { return got_runtime_error; }

	void execute(statement &stmt);

#pragma region expression::visitor_interface methods

	void accept(const expression::unary &unary) override;
	void accept(const expression::assignment &assign) override;
	void accept(const expression::binary &binary) override;
	void accept(const expression::grouping &group) override;
	void accept(const expression::literal &value) override;
	void accept(const expression::logical &logic) override;
	void accept(const expression::identifier &id) override;

#pragma endregion expression::visitor_interface methods

#pragma region statement::visitor_interface methods

	void accept(const statement::scope &scope) override;
	void accept(const statement::expression &expr) override;
	void accept(const statement::branch &branch) override;
	void accept(const statement::variable &var) override;
	void accept(const statement::constant &con) override;
	void accept(const statement::loop &loop) override;

#if defined(LOX_DEBUG)
	void accept(const statement::print &print) override;
#endif // defined(LOX_DEBUG)

#pragma endregion statement::visitor_interface methods


private:
	environment m_env{};
	literal m_output{};
	std::reference_wrapper<const lexeme_database> lexemes;
	std::reference_wrapper<error_handler> errout;
	bool got_runtime_error{ false };

	void execute_block(const std::vector<std::unique_ptr<statement>> &statements);

	auto error_no_suitable(const token &op, const literal &value) const -> execution_error;
	auto error_no_suitable(const token &op, const literal &lhv, const literal &rhv) const -> execution_error;
	auto error(error_code err_no, std::string_view msg, const token &tok) const -> execution_error;
	auto error(error_code err_no, std::string_view msg) const -> execution_error;

	static auto is_suitable_for(token_type op, literal_type type) -> bool;
	static auto is_suitable_for(token_type op, literal_type lhv, literal_type rhv) -> bool;

	static auto is_truth(const literal &value) -> std::optional<bool>;
	static auto negate_number(literal &value) -> bool;
	static auto inverse_boolean(literal &value) -> bool;
};

} // namespace lox::execution
