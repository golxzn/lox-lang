#include "lox/expression.hpp"

namespace lox {

void expression::visitor_interface::accept(expression::unary &) { /* no impl required */ }
void expression::visitor_interface::accept(expression::binary &) { /* no impl required */ }
void expression::visitor_interface::accept(expression::grouping &) { /* no impl required */ }
void expression::visitor_interface::accept(expression::literal &) { /* no impl required */ }

void expression::unary::accept(visitor_interface &visitor) {
	visitor.accept(*this);
}

void expression::binary::accept(visitor_interface &visitor) {
	visitor.accept(*this);
}

void expression::grouping::accept(visitor_interface &visitor) {
	visitor.accept(*this);
}

void expression::literal::accept(visitor_interface &visitor) {
	visitor.accept(*this);
}

} // namespace lox
