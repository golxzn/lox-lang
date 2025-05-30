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


class LOX_EXPORT interpreter final
	: public expression_visitor_interface<literal>
	, public statement_visitor_interface<void> {
public:
	struct execution_error : public std::runtime_error {
		using std::runtime_error::runtime_error;
		status reason{ status::runtime_error };
	};

	interpreter(
		environment env,
		const program &prog,
		const lexeme_database &lexemes,
		error_handler &handler
	) noexcept;

	~interpreter() override = default;

	[[nodiscard]] auto run() -> status;
	[[nodiscard]] auto runtime_error() const noexcept -> bool { return got_runtime_error; }

	void execute(statement_id stmt);
	[[nodiscard]] auto evaluate(expression_id expr) -> literal;


#pragma region expression::visitor_interface methods

	[[nodiscard]] auto accept(const expression_unary &unary) -> literal override;
	[[nodiscard]] auto accept(const expression_incdec &incdec) -> literal override;
	[[nodiscard]] auto accept(const expression_assignment &assign) -> literal override;
	[[nodiscard]] auto accept(const expression_binary &binary) -> literal override;
	[[nodiscard]] auto accept(const expression_call &call) -> literal override;
	[[nodiscard]] auto accept(const expression_grouping &group) -> literal override;
	[[nodiscard]] auto accept(const expression_literal &value) -> literal override;
	[[nodiscard]] auto accept(const expression_logical &logic) -> literal override;
	[[nodiscard]] auto accept(const expression_identifier &id) -> literal override;

#pragma endregion expression::visitor_interface methods

#pragma region statement::visitor_interface methods

	void accept(const statement_scope &scope) override;
	void accept(const statement_expression &expr) override;
	void accept(const statement_branch &branch) override;
	void accept(const statement_variable &var) override;
	void accept(const statement_constant &con) override;
	void accept(const statement_loop &loop) override;

#if defined(LOX_DEBUG)
	void accept(const statement_print &print) override;
#endif // defined(LOX_DEBUG)

#pragma endregion statement::visitor_interface methods


private:
	environment m_env{};
	std::reference_wrapper<const program> prog;
	std::reference_wrapper<const lexeme_database> lexemes;
	std::reference_wrapper<error_handler> errout;
	bool got_runtime_error{ false };

	void execute_block(const program::statement_list &statements);
	void safe_assign(const token &tok, literal value);

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
