#include <ranges>
#include <algorithm>
#include "lox/utils/strutils.hpp"

namespace lox::utils {

auto quoted(const std::string_view line, const std::string_view quote) -> std::string {
	std::string quoted_line(std::size(line) + std::size(quote) * 2ull, '\0');
	auto cur{ std::begin(quoted_line) };
	cur = std::ranges::copy(quote, cur).out;
	cur = std::ranges::copy(line, cur).out;
	std::ranges::copy(quote, cur);
	return quoted_line;
}

} // namespace lox::utils
