#include <format>

#include "lox/error_handler.hpp"

namespace lox {

auto error_handler::register_file(const std::string_view path) -> file_id {
	const auto id{ static_cast<file_id>(std::size(m_files)) };
	m_files.emplace_back(std::string{ path });
	return id;
}

void error_handler::report(
	std::string_view message,
	error_record record,
	std::string_view source_code
) noexcept {
	if (const auto hash{ merge_file_and_line(record) }; !(std::empty(source_code) || m_lines.contains(hash))) {
		m_lines.try_emplace(hash, take_line(source_code, record));
	}

	const auto from{ std::exchange(record.from, relative_to_line(source_code, record.from)) };
	record.to = record.from + record.to - from;

	m_errors.emplace_back(std::move(record));
	m_error_messages.emplace_back(message);
}

void error_handler::make_msg(std::pmr::string &buffer, size_t id, const error_record &record) const {
	auto out{ std::back_inserter(buffer) };

	std::format_to(out, "{}:{}:{} > {} #{:04}:",
		get_file_path(record.file_id), record.line, record.from,
		code_type_name(record.code), code_value(record.code)
	);

	if (const auto hash{ merge_file_and_line(record) }; m_lines.contains(hash)) {
		std::format_to(out, "\n\n{:3} | {}\n", record.line, m_lines.at(hash));
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
	// m_files.clear();
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

auto error_handler::get_file_path(const file_id id) const noexcept -> std::string_view {
	if (const size_t index{ static_cast<size_t>(id) }; index < std::size(m_files)) {
		return m_files.at(index).path;
	}
	return "";
}

auto error_handler::relative_to_line(const std::string_view src, uint32_t pos) -> uint16_t {
	const size_t last_eol_pos{ src.find_last_of('\n', pos) };
	const size_t last_eol{ last_eol_pos != std::string_view::npos ? last_eol_pos : 0u };
	return static_cast<uint16_t>(pos - last_eol);
}

auto error_handler::code_value(const record_code code) noexcept -> uint32_t {
	if (auto err{ std::get_if<error_code>(&code) }; err) {
		return static_cast<uint32_t>(*err);
	}
	if (auto err{ std::get_if<warning_code>(&code) }; err) {
		return static_cast<uint32_t>(*err);
	}
	return 0;
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

auto error_handler::merge_file_and_line(const error_record &record) noexcept -> uint64_t {
	return static_cast<uint64_t>(record.file_id) << 32
		| static_cast<uint64_t>(record.line);
}


} // namespace lox
