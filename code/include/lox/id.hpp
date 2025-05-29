#pragma once

#include <limits>
#include "lox/utils/compile_time.hpp"

namespace lox {

template<utils::ct::enumeration Type, std::integral IndexType = uint32_t>
struct ID {
	static constexpr auto invalid_index{ (std::numeric_limits<IndexType>::max)() };

	using enum_type  = Type;
	using index_type = IndexType;

	IndexType index{ invalid_index };
	Type      type{};

	[[nodiscard]] constexpr auto empty() const noexcept -> bool { return index == invalid_index; }
};

} // namespace lox
