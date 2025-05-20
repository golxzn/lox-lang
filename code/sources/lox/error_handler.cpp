#include <format>

#include "lox/error_handler.hpp"

namespace lox {

error_handler::error_handler(std::string path, std::string_view source_code) noexcept
	: m_file{ .path = std::move(path), .source_code = source_code } {}

void error_handler::report(
	std::string_view message,
	error_record record
) noexcept {
	if (!(std::empty(m_file.source_code) || m_lines.contains(record.line))) {
		m_lines.try_emplace(record.line, take_line(m_file.source_code, record));
	}

	const auto from{ std::exchange(record.from, relative_to_line(m_file.source_code, record.from)) };
	record.to = record.from + record.to - from;

	m_errors.emplace_back(std::move(record));
	m_error_messages.emplace_back(message);
}

void error_handler::make_msg(std::pmr::string &buffer, size_t id, const error_record &record) const {
	auto out{ std::back_inserter(buffer) };

	std::format_to(out, "{}:{}:{} > {} #{:04}:",
		m_file.path, record.line, record.from,
		code_type_name(record.code), code_value(record.code)
	);

	if (m_lines.contains(record.line)) {
		std::format_to(out, "\n\n{:3} | {}\n", record.line, m_lines.at(record.line));
		if (record.from != 0u && record.to != 0u) {
			std::format_to(out, "    |{:>{}}{:^>{}}\n", "", record.from, "", record.to - record.from);
			std::format_to(out, "    |{:>{}}", "", record.from);
		}
	}

	std::format_to(out, " {}\n", m_error_messages.at(id));
}

void error_handler::clear() {
	m_lines.clear();
	m_error_messages.clear();
	m_errors.clear();
}

auto error_handler::take_line(const std::string_view source, const error_record &record) const noexcept -> std::pmr::string {
	const auto line_start_pos{ source.find_last_of(eol, record.from) };
	const auto line_start{ line_start_pos != std::string_view::npos ? line_start_pos + 1u : 0u };
	const auto line_end{ source.find_first_of(eol, record.to) };

	const auto line{ source.substr(line_start,
		line_end != std::string_view::npos ? line_end - line_start : line_end
	) };

	return std::pmr::string{ line, m_allocator };
}

auto error_handler::relative_to_line(const std::string_view src, uint32_t pos) -> uint16_t {
	const size_t last_eol_pos{ src.find_last_of('\n', pos) };
	const size_t last_eol{ last_eol_pos != std::string_view::npos ? last_eol_pos : 0u };
	return static_cast<uint16_t>(pos - last_eol);
}

auto error_handler::code_value(const record_code code) noexcept -> uint32_t {
	return std::visit([] (const auto value) noexcept { return static_cast<uint32_t>(value); }, code);
}

auto error_handler::code_type_name(const record_code code) noexcept -> std::string_view {
	using namespace std::string_view_literals;
	switch (code.index()) {
		case 0: return "error"sv;
		case 1: return "warning"sv;
		default: break;
	}
	return "info"sv;
}

} // namespace lox
