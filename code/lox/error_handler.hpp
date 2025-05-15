#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory_resource>

namespace lox {

enum class error_code : uint32_t {
	no_error,

	scanner_error_begin,
	se_no_sources,           /// No sources was given to the scanner
	se_unexpected_symbol,    /// Found unexpected symbol during scan
	se_broken_symmetry,      /// The symmetry character like () or "" was not closed
	scanner_error_end = 99,

	parser_error_begin,
	parser_error_end = 199,

};

enum class warning_code : uint32_t {
	no_warning,
};

using record_code = std::variant<error_code, warning_code>;

enum class file_id : uint32_t {};

struct error_record {
	record_code code{};
	file_id file_id{};
	uint32_t line{};
	uint32_t from{}; /// underscore from position inside the line
	uint32_t to{};   /// underscore to position inside the line
};

template<class T>
concept error_exporter = requires(const T &pr) {
	{ pr(std::string_view{}) };
};

class error_handler {
public:
	static constexpr char eol{ '\n' };
	static constexpr std::pmr::pool_options buffer_options{
		.max_blocks_per_chunk = 4,
		.largest_required_pool_block = 256
	};

	struct file_info {
		std::string path{};
	};

	[[nodiscard]] auto register_file(const std::string_view path) -> file_id;

	void report(std::string_view message, error_record record, std::string_view source_code) noexcept;

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

	std::vector<std::string> m_error_messages{};
	std::vector<error_record> m_errors{};
	std::vector<file_info> m_files{};

	void make_msg(std::pmr::string &buffer, size_t id, const error_record &record) const;
	auto take_line(const std::string_view source, const error_record &record) const noexcept -> std::pmr::string;
	auto get_file_path(const file_id id) const noexcept -> std::string_view;

	static auto relative_to_line(const std::string_view src, uint32_t pos) -> uint16_t;
	inline static auto code_value(const record_code record) noexcept -> uint32_t;
	inline static auto code_type_name(const record_code record) noexcept -> std::string_view;
	inline static auto merge_file_and_line(const error_record &record) noexcept -> uint64_t;
};

} // namespace lox
