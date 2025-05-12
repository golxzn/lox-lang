#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

namespace lox {

struct token;
struct literal;

class scanner {
public:
	struct skip_info {
		uint32_t skipped{};
		uint32_t new_lines{};
	};

	struct error_type {
		std::string message{};
	};
	struct output_type {
		std::vector<token> tokens{};
		std::vector<literal> literals{};
		std::vector<error_type> errors{};
	};

	explicit scanner(const std::string_view script);

	[[nodiscard]] auto scan() -> output_type;

private:
	std::string_view m_script;
	uint32_t m_line{};

	auto end_position() const noexcept -> uint32_t;

	auto next_token(uint32_t position, output_type &output) -> uint32_t;

	auto parse_string_token(uint32_t position, output_type &output) -> uint32_t;
	auto parse_number_token(uint32_t position, output_type &output) -> uint32_t;
	auto parse_identifier_token(uint32_t position, output_type &output) -> uint32_t;

	auto skip_till(const char matches, uint32_t from) const noexcept -> uint32_t;
	auto skip_whitespaces(uint32_t position) noexcept -> uint32_t;
	auto skip_multiline_comment(uint32_t position) noexcept -> uint32_t;

	auto make_error_unexpected_symbol(uint32_t pos) const -> error_type;
	auto make_error_message(const std::string_view text, uint32_t from, uint32_t to) const -> std::string;

};

} // namespace lox
