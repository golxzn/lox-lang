#pragma once

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <memory_resource>

#include "lox/export.hpp"

namespace lox {

enum class error_code : uint32_t {
	no_error,

	scanner_error_begin,
	se_no_sources,           /// No sources was given to the scanner
	se_unexpected_symbol,    /// Found unexpected symbol during scan
	se_broken_symmetry,      /// The symmetry character like () or "" was not closed
	scanner_error_end = 99,

	parser_error_begin,
	pe_missing_end_of_statement,
	pe_unexpected_token,
	pe_broken_symmetry,       /// The symmetry character like () or {} was not closed
	pe_missing_literal,
	pe_missing_const_initialization,
	pe_lvalue_assignment,
	parser_error_end = 199,

	evaluation_error_begin,
	ee_literal_not_suitable_for_operation,
	ee_runtime_error,
	ee_missing_expression,
	ee_undefined_identifier,
	ee_identifier_already_exists,
	ee_constant_assignment,
	evaluation_error_end = 299,
};

enum class warning_code : uint32_t {
	no_warning,
};

using record_code = std::variant<error_code, warning_code>;

struct LOX_EXPORT error_record {
	record_code code{};
	uint32_t line{};
	uint32_t from{}; /// underscore from position
	uint32_t to{};   /// underscore to position
};

template<class T>
concept error_exporter = requires(const T &pr) {
	{ pr(std::string_view{}) };
};

class LOX_EXPORT error_handler {
public:
	static constexpr char eol{ '\n' };
	static constexpr std::pmr::pool_options buffer_options{
		.max_blocks_per_chunk = 4,
		.largest_required_pool_block = 256
	};

	struct file_info {
		std::string path{};
		std::string_view source_code{};
	};

	explicit error_handler(std::string path, std::string_view source_code = {}) noexcept;

	[[nodiscard]] auto empty() const noexcept -> bool { return std::empty(m_errors); }
	void report(std::string_view message, error_record record) noexcept;
	void clear();

	void export_records(const error_exporter auto & ...exporter) const {
		std::pmr::monotonic_buffer_resource resource{};
		std::pmr::polymorphic_allocator<char> allocator{ &resource };
		std::pmr::string buffer{ allocator };

		for (size_t i{}; i < std::size(m_errors); ++i) {
			buffer.clear();
			make_msg(buffer, i, m_errors[i]);
			const std::string_view message{ buffer };
			(exporter(message), ...);
		}
	}

private:
	std::pmr::synchronized_pool_resource m_memory_resource{ buffer_options };
	std::pmr::polymorphic_allocator<char> m_allocator{ &m_memory_resource };
	std::pmr::unordered_map<uint64_t, std::pmr::string> m_lines{ m_allocator };

	file_info m_file;
	std::vector<std::string> m_error_messages{};
	std::vector<error_record> m_errors{};

	void make_msg(std::pmr::string &buffer, size_t id, const error_record &record) const;
	auto take_line(const std::string_view source, const error_record &record) const noexcept -> std::pmr::string;

	static auto relative_to_line(const std::string_view src, uint32_t pos) -> uint16_t;
	inline static auto code_value(const record_code record) noexcept -> uint32_t;
	inline static auto code_type_name(const record_code record) noexcept -> std::string_view;
};

} // namespace lox
