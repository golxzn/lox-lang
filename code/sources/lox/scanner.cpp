#include <array>
#include <format>
#include <ranges>
#include <algorithm>
#include <unordered_map>

#include "lox/scanner.hpp"

#include "lox/types/token.hpp"
#include "lox/types/literal.hpp"
#include "lox/utils/strhash.hpp"
#include "lox/utils/strutils.hpp"

namespace lox {

scanner::scanner(const std::string_view script, error_handler &err)
	: errout{ err }
	, m_script{ lox::utils::strip(script) }
{}

auto scanner::scan() -> output_type {
	output_type output{};

	if (std::empty(m_script)) {
		errout.report("No source was given!", error_record{
			.code    = error_code::se_no_sources,
			.line    = m_line,
		});
		return output;
	}

	const auto end_pos{ end_position() };
	// Common used shit
	output.literals = std::vector<literal>{
		{ /*null*/ }, true, false,
		std::string{},
		double{},
		int64_t{},
	};

	m_line = 0u;
	uint32_t position{};
	while (position != end_pos) {
		position = next_token(position, output);
	}

	output.tokens.emplace_back(m_line, position, invalid_id, invalid_id, token_type::end_of_file);

	return output;
}

auto scanner::end_position() const noexcept -> uint32_t {
	return static_cast<uint32_t>(std::size(m_script));
}

auto scanner::next_token(uint32_t pos, output_type &output) -> uint32_t {
	constexpr uint32_t symbol_size{ static_cast<uint32_t>(sizeof(char)) };

	pos = skip_whitespaces(pos);

	const auto symbol{ m_script[pos] };
	const auto add_token{ [&tokens{ output.tokens }, line{ m_line }, pos] (auto type, uint32_t offset = 1u) {
		tokens.emplace_back(line, pos, invalid_id, invalid_id, type);
		return pos + offset;
	} };
	const auto next_is{ [&script{ m_script }, pos, end{ end_position() }](const auto expected) {
		if (pos + symbol_size < end && script[pos + 1] == expected) {
			return 1u;
		}
		return 0u;
	} };

	const auto is_next_equal{ next_is('=') };

	switch (symbol) {
		case '(': return add_token(token_type::left_paren);
		case ')': return add_token(token_type::right_paren);
		case '{': return add_token(token_type::left_brace);
		case '}': return add_token(token_type::right_brace);
		case ';': return add_token(token_type::semicolon);
		case ',': return add_token(token_type::comma);
		case '-':
			if (next_is('-')) {
				return add_token(token_type::decrement);
			}
			return add_token(is_next_equal ? token_type::minus_equal : token_type::minus) + is_next_equal;
		case '+':
			if (next_is('+')) {
				return add_token(token_type::increment) + 1;
			}
			return add_token(is_next_equal ? token_type::plus_equal : token_type::plus) + is_next_equal;

		case '*': return add_token(is_next_equal ? token_type::star_equal : token_type::star) + is_next_equal;
		case '/':
			if (next_is('/')) {
				return skip_till('\n', pos + 2);
			}
			if (next_is('*')) {
				return skip_multiline_comment(pos + 2);
			}
			if (is_next_equal) {
				return add_token(token_type::slash_equal) + 1;
			}
			return add_token(token_type::slash);

		case '"': return parse_string_token(pos, output);


		default: break;
	}

	switch (symbol) {
		case '!': return add_token(
			is_next_equal ? token_type::bang_equal : token_type::bang,
			symbol_size + is_next_equal
		);
		case '=': return add_token(
			is_next_equal ? token_type::equal_equal : token_type::equal,
			symbol_size + is_next_equal
		);
		case '<': return add_token(
			is_next_equal ? token_type::less_equal : token_type::less,
			symbol_size + is_next_equal
		);
		case '>': return add_token(
			is_next_equal ? token_type::greater_equal : token_type::greater,
			symbol_size + is_next_equal
		);

		default: break;
	}

	if (std::isdigit(symbol)) {
		return parse_number_token(pos, output);
	}

	if (std::isalpha(symbol) || symbol == '_') {
		if (auto next_pos{ try_parse_null_or_boolean(pos, output) }; next_pos.has_value()) {
			return next_pos.value();
		}
		return parse_identifier_token(pos, output);
	}

	make_error_unexpected_symbol(pos);

	return pos + symbol_size; // keep going
}

auto scanner::parse_string_token(const uint32_t pos, output_type &output) -> uint32_t {
	const auto end{ end_position() };

	auto cur{ pos + 1u };
	uint32_t lines_count{};
	while (cur < end && m_script[cur] != '"') {
		lines_count += static_cast<uint32_t>(m_script[cur] == '\n');
		++cur;
	}

	if (cur == end) {
		++m_line;

		errout.report(R"(Unclosed string literal! No '"' was found)", error_record{
			.code = error_code::se_broken_symmetry,
			.line    = m_line,
			.from    = pos,
			.to      = static_cast<uint16_t>(pos + 1)
		});

		return skip_till(';', pos + 1);
	}

	const auto id{ emplace_literal(std::string{ m_script.substr(pos + 1u, cur - pos - 1u) }, output.literals) };
	output.tokens.emplace_back(m_line, pos, id, invalid_id, token_type::string);

	m_line += lines_count;

	return cur + 1u;
}

auto scanner::parse_number_token(const uint32_t pos, output_type &output) -> uint32_t {
	const auto end{ end_position() };
	static constexpr auto is_digit_or_single_quote{ [](const char c) {
		return std::isdigit(c) || c == '\'';
	} };

	auto cur{ pos + 1 };
	while (cur < end && is_digit_or_single_quote(m_script[cur])) {
		++cur;
	}
	if (m_script[cur] == '.') {
		++cur;
		while (cur < end && is_digit_or_single_quote(m_script[cur])) {
			++cur;
		}
	}

	const auto id{ emplace_literal(to_number_literal(m_script.substr(pos, cur - pos)), output.literals) };
	output.tokens.emplace_back(m_line, pos, id, invalid_id, token_type::number);

	return cur;
}

auto scanner::parse_identifier_token(uint32_t pos, output_type &output) -> uint32_t {
	const auto end{ end_position() };

	auto cur{ pos + 1u };
	while (cur != end && (std::isalnum(m_script[cur]) || m_script[cur] == '_')) {
		++cur;
	}
	const auto lexeme{ m_script.substr(pos, cur - pos) };
	const auto type{ from_keyword(lexeme) };
	uint16_t lexeme_id{ invalid_id };
	if (type == token_type::identifier) {
		lexeme_id = output.lexemes.add(lexeme);
	}

	output.tokens.emplace_back(m_line, pos, invalid_id, lexeme_id, type);

	return cur;
}

auto scanner::try_parse_null_or_boolean(uint32_t pos, output_type &output) -> std::optional<uint32_t> {
	if (constexpr std::string_view check{ "ntf" }; check.find(m_script[pos]) == std::string_view::npos) {
		return std::nullopt;
	}

	const auto end{ end_position() };

	constexpr auto is_punct_or_space_except_underscore{ [](auto symbol) {
		return !((std::ispunct(symbol) && symbol != '_') || std::isspace(symbol));
	} };

	auto cur{ pos + 1u };
	while (cur != end && is_punct_or_space_except_underscore(m_script[cur])) {
		++cur;
	}

	const size_t len{ static_cast<size_t>(cur - pos) };

	switch (const auto identifier{ m_script.substr(pos, len) }; utils::fnv1a(identifier)) {
		using namespace utils::fnv1a_literals;
		case "null"_fnv1a:
			output.tokens.emplace_back(
				m_line, pos, emplace_literal({}, output.literals), invalid_id, token_type::null
			);
			break;

		case "true"_fnv1a:
			output.tokens.emplace_back(
				m_line, pos, emplace_literal(true, output.literals), invalid_id, token_type::boolean
			);
			break;

		case "false"_fnv1a:
			output.tokens.emplace_back(
				m_line, pos, emplace_literal(false, output.literals), invalid_id, token_type::boolean
			);
			break;

		default:
			return std::nullopt;
	}

	return cur;
}

auto scanner::skip_till(const char matches, uint32_t pos) const noexcept -> uint32_t {
	const auto end{ end_position() };
	while (pos < end && m_script[pos] != matches) {
		++pos;
	}
	return pos;
}

auto scanner::skip_whitespaces(uint32_t pos) noexcept -> uint32_t {
	for (const auto end{ end_position() }; pos < end; ++pos) {
		if (const auto ch{ m_script[pos] }; std::isspace(m_script[pos])) {
			if (ch == '\n') ++m_line;
			continue;
		}
		break;
	}
	return pos;
}

auto scanner::skip_multiline_comment(uint32_t pos) noexcept -> uint32_t {
	const auto end{ end_position() };

	++pos;
	while (pos < end && m_script[pos] != '/' && m_script[pos - 1] != '*') {
		m_line += static_cast<uint32_t>(m_script[pos] == '\n');
		++pos;
	}

	return pos;
}

auto scanner::emplace_literal(literal lit, std::vector<literal> &out) -> uint16_t {
	if (auto found{ std::ranges::find(out, lit) }; found != std::end(out)) {
		return static_cast<uint16_t>(std::distance(std::begin(out), found));
	}

	const auto id{ static_cast<uint16_t>(std::size(out)) };
	out.emplace_back(std::move(lit));
	return id;
}

void scanner::make_error_unexpected_symbol(const uint32_t pos) const {
	constexpr size_t message_len{ sizeof("Unexpected symbol ' '" ) };
	constexpr size_t symbol_pos{ sizeof("Unexpected symbol '") - 1u };

	std::array<char, message_len> stack_string{  "Unexpected symbol ' '" };
	stack_string[symbol_pos] = m_script[pos];

	const std::string_view message{ std::data(stack_string), std::size(stack_string) - 1ull };
	errout.report(message, error_record{
		.code    = error_code::se_no_sources,
		.line    = m_line,
		.from    = pos,
		.to      = pos + 1u
	});
}

} // namespace lox
