#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

#include "lox/error_handler.hpp"

namespace lox {

struct token;
struct literal;

class scanner {
public:
	struct skip_info {
		uint32_t skipped{};
		uint32_t new_lines{};
	};

	struct output_type {
		std::vector<token> tokens{};
		std::vector<literal> literals{};
	};

	explicit scanner(const std::string_view script, file_id file_id, error_handler &errs);

	[[nodiscard]] auto scan() -> output_type;

private:
	error_handler &errout;
	std::string_view m_script;
	file_id m_file_id;
	uint32_t m_line{};
	bool m_failed{};

	auto end_position() const noexcept -> uint32_t;

	auto next_token(uint32_t position, output_type &output) -> uint32_t;

	auto parse_string_token(uint32_t position, output_type &output) -> uint32_t;
	auto parse_number_token(uint32_t position, output_type &output) -> uint32_t;
	auto parse_identifier_token(uint32_t position, output_type &output) -> uint32_t;
	auto try_parse_null_or_boolean(uint32_t position, output_type &output) -> std::optional<uint32_t>;

	auto skip_till(const char matches, uint32_t from) const noexcept -> uint32_t;
	auto skip_whitespaces(uint32_t position) noexcept -> uint32_t;
	auto skip_multiline_comment(uint32_t position) noexcept -> uint32_t;

	auto emplace_literal(literal lit, std::vector<literal> &out) -> uint16_t;
	void make_error_unexpected_symbol(uint32_t pos) const;
};

} // namespace lox
