#include <format>
#include <unordered_map>

#include "lox/scanner.hpp"

#include "lox/utils.hpp"
#include "lox/token.hpp"
#include "lox/literal.hpp"

namespace lox {

scanner::scanner(const std::string_view script)
	: m_script{ lox::utils::strip(script) } {}

auto scanner::scan() -> output_type {
	output_type output{};

	if (std::empty(m_script)) {
		output.errors.emplace_back("No source was given!");
		return std::move(output);
	}

	const auto end_pos{ end_position() };

	m_line = 1u;
	uint32_t position{};
	while (position != end_pos) {
		position = next_token(position, output);
	}

	output.tokens.emplace_back(position, invalid_literal_id, token_type::end_of_file);

	return std::move(output);
}

auto scanner::end_position() const noexcept -> uint32_t {
	return static_cast<uint32_t>(std::size(m_script));
}

auto scanner::next_token(uint32_t pos, output_type &output) -> uint32_t {
	constexpr uint32_t symbol_size{ static_cast<uint32_t>(sizeof(char)) };

	pos = skip_whitespaces(pos);

	const auto symbol{ m_script[pos] };
	const auto add_token{ [&tokens{ output.tokens }, pos] (token_type type, uint32_t offset = 1u) {
		tokens.emplace_back(pos, invalid_literal_id, type);
		return pos + offset;
	} };
	const auto next_is{ [&script{ m_script }, pos, end{ end_position() }](const auto expected) {
		if (pos + symbol_size < end && script[pos + 1] == expected) {
			return 1u;
		}
		return 0u;
	} };


	switch (symbol) {
		case '(': return add_token(token_type::left_paren);
		case ')': return add_token(token_type::right_paren);
		case '{': return add_token(token_type::left_brace);
		case '}': return add_token(token_type::right_brace);
		case ';': return add_token(token_type::semicolon);
		case ',': return add_token(token_type::comma);
		case '-': return add_token(token_type::minus);
		case '+': return add_token(token_type::plus);
		case '*': return add_token(token_type::star);
		case '/':
			if (next_is('/')) {
				return skip_till('\n', pos + 2);
			} else if (next_is('*')) {
				return skip_multiline_comment(pos + 2);
			}
			return add_token(token_type::slash);

		case '"': return parse_string_token(pos, output);


		default: break;
	}

	const auto is_next_equal{ next_is('=') };
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
		return parse_identifier_token(pos, output);
	}

	output.errors.emplace_back(make_error_unexpected_symbol(pos));

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
		output.errors.emplace_back(
			make_error_message(R"(Unclosed string literal! No '"' was found)", pos, pos + 1u)
		);
		return skip_till(';', pos + 1);
	}

	const auto id{ static_cast<uint16_t>(std::size(output.literals)) };

	output.tokens.emplace_back(pos, id, token_type::string);
	output.literals.emplace_back(m_script.substr(pos + 1u, cur - pos - 1u));
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
	const auto id{ static_cast<uint16_t>(std::size(output.literals)) };

	output.tokens.emplace_back(pos, id, token_type::number);
	output.literals.emplace_back(to_number_literal(m_script.substr(pos, cur - pos)));

	return cur;
}

auto scanner::parse_identifier_token(uint32_t pos, output_type &output) -> uint32_t {
	const auto end{ end_position() };

	auto cur{ pos + 1u };
	while (cur != end && (std::isalnum(m_script[cur]) || m_script[cur] == '_')) {
		++cur;
	}
	const auto identifier{ m_script.substr(pos, cur - pos) };

	output.tokens.emplace_back(pos, invalid_literal_id, from_keyword(identifier));

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

auto scanner::make_error_unexpected_symbol(const uint32_t pos) const -> error_type {
	constexpr size_t msg_length{ sizeof("Unexpected symbol ' '") };
	constexpr size_t symbol_pos{ sizeof("Unexpected symbol '") - 1u };

	std::array<char, msg_length> stack_string{ "Unexpected symbol ' '" };
	stack_string[symbol_pos] = m_script[pos];

	// auto [ptr, len]{ std::format_to_n(std::data(stack_string), std::size(stack_string), "Unexpected symbol '{}'", m_script[pos]) };

	return error_type{
		.message = make_error_message(
			std::string_view{ std::data(stack_string), std::size(stack_string) - 1 },
			pos, pos + 1
		)
	};
}

auto scanner::make_error_message(const std::string_view text, const uint32_t from, const uint32_t to) const -> std::string {
	const auto line_start_pos{ m_script.find_last_of('\n', from) };
	const auto line_start{ line_start_pos != std::string_view::npos ? line_start_pos + 1u : 0u };
	const auto line_end{ m_script.find_first_of('\n', to) };

	const auto line{ m_script.substr(line_start,
		line_end != std::string_view::npos ? line_end - line_start : line_end
	) };

	return std::format(
R"({}:

{:3} | {}
     {:>{}}{:^>{}}
)",
		text,
		m_line, line,
		' ', from - line_start + 1,
		'^', to - from
	);
}

} // namespace lox
