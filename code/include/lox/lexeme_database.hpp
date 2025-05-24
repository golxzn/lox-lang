#pragma once

#include <map>
#include <limits>
#include <vector>
#include <cstdint>
#include <string_view>

#include "lox/types.hpp"
#include "lox/export.hpp"

namespace lox {

class LOX_EXPORT lexeme_database {
public:
	static constexpr auto npos{ (std::numeric_limits<lexeme_id>::max)() };

	struct section {
		uint32_t offset{};
		uint32_t length{};
	};

	[[nodiscard]] auto add(std::string_view lexeme) noexcept -> lexeme_id;
	[[nodiscard]] auto find(std::string_view lexeme) const noexcept -> lexeme_id;
	[[nodiscard]] auto get(lexeme_id id) const noexcept -> std::string_view;

private:
	std::vector<char> m_buffer{};
	std::vector<section> m_sections{ section{ 0u, 0u } };
	std::map<hash_type, lexeme_id> m_lookup_table{};

	auto find(hash_type hash) const noexcept -> lexeme_id;
};

} // namespace lox
