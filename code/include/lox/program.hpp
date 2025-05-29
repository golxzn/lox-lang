#pragma once

#include "lox/gen/program_base.hpp"

namespace lox {

class program : public program_base {
public:
	using program_base::program_base;

	auto add_statement(statement_id stmt) -> statement_id {
		return statements.emplace_back(stmt);
	}

	template<statement_type Type, class ...Args>
	auto emplace_and_record(Args ...args) -> statement_id {
		return add_statement(emplace<Type>(std::forward<Args>(args)...));
	}

	template<statement_type Type, class ...Args>
	auto emplace(Args ...args) -> statement_id {
		auto &record{ get_statements<Type>() };
		const auto index{ static_cast<statement_id::index_type>(std::size(record)) };
		record.emplace_back(std::forward<Args>(args)...);
		return statement_id{ index, Type };
	}

	template<expression_type Type, class ...Args>
	auto emplace(Args ...args) -> expression_id {
		auto &record{ get_expressions<Type>() };
		const auto index{ static_cast<expression_id::index_type>(std::size(record)) };
		record.emplace_back(std::forward<Args>(args)...);
		return expression_id{ index, Type };
	}
};

} // namespace lox
