#pragma once

#include <concepts>

#include "lox/token.hpp"
#include "lox/literal.hpp"

namespace lox {

enum class expression_tag {
	unary,
	binary,
	grouping,
	literal,
};

struct expression {
	virtual ~expression() = default;

	class unary;
	class binary;
	class grouping;
	class literal;

	struct visitor_interface {
		virtual ~visitor_interface() = default;

		virtual void accept(expression::unary &expr);
		virtual void accept(expression::binary &expr);
		virtual void accept(expression::grouping &expr);
		virtual void accept(expression::literal &expr);
	};

	virtual void accept(visitor_interface &visitor) = 0;
};

struct expression::unary final : public expression {
	unary() = default;
	unary(token op, std::unique_ptr<expression> expr) noexcept
		: op{ std::move(op) }, expr{ std::move(expr) } {}

	~unary() override = default;

	void accept(visitor_interface &visitor) override;

	token op{};
	std::unique_ptr<expression> expr{};
};

struct expression::binary final : public expression {
	~binary() override = default;
	binary() = default;
	explicit binary(token op,
		std::unique_ptr<expression> le = nullptr,
		std::unique_ptr<expression> re = nullptr
	) noexcept
		: op{ std::move(op) }, left{ std::move(le) }, right{ std::move(re) } {}

	void accept(visitor_interface &visitor) override;

	token op{};
	std::unique_ptr<expression> left{};
	std::unique_ptr<expression> right{};
};

struct expression::grouping final : public expression {
	~grouping() override = default;
	grouping() = default;
	explicit grouping(std::unique_ptr<expression> v) noexcept : expr{ std::move(v) } {}

	void accept(visitor_interface &visitor) override;

	std::unique_ptr<expression> expr{};
};

struct expression::literal final : public expression {
	~literal() override = default;

	literal() = default;
	explicit literal(lox::literal v) noexcept : value{ std::move(v) } {}

	void accept(visitor_interface &visitor) override;

	lox::literal value{};
};

} // namespace lox
