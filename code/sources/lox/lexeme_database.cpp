#include "lox/lexeme_database.hpp"

#include "lox/utils/strhash.hpp"

namespace lox {

auto lexeme_database::add(std::string_view lexeme) noexcept -> lexeme_id {
	if (std::empty(lexeme)) {
		return npos;
	}

	const auto hash{ utils::fnv1a(lexeme) };
	if (const auto found{ find(hash) }; found != npos) {
		return found;
	}

	const auto [offset, length]{ m_sections.back() };
	if (const auto required_size{ std::size(m_buffer) + std::size(lexeme) };
		std::size(m_buffer) < required_size
	) {
		m_buffer.resize(required_size);
	}

	const auto new_offset{ offset + length };
	std::ranges::copy(lexeme, std::next(std::begin(m_buffer), new_offset));

	const lexeme_id id{ std::size(m_sections) };
	m_sections.emplace_back(new_offset, std::size(lexeme));

	m_lookup_table.emplace(hash, id);

	return id;
}

auto lexeme_database::find(std::string_view lexeme) const noexcept -> lexeme_id {
	if (!std::empty(lexeme)) {
		return find(utils::fnv1a(lexeme));
	}
	return npos;
}

auto lexeme_database::get(lexeme_id id) const noexcept -> std::string_view {
	if (id < std::size(m_sections)) {
		const auto [offset, length]{ m_sections.at(id) };
		return std::string_view{ std::next(std::data(m_buffer), offset), length };
	}

	return {};
}

auto lexeme_database::find(hash_type hash) const noexcept -> lexeme_id {
	if (const auto found{ m_lookup_table.find(hash) }; found != std::end(m_lookup_table)) {
		return found->second;
	}
	return npos;
}

} // namespace lox
