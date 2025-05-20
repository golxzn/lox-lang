#pragma once

#include <string>
#include <string_view>

namespace lox::utils {

constexpr std::string_view whitespaces{ " \t\r\n" };

[[nodiscard]] constexpr auto strip(const std::string_view line) -> std::string_view {
	if (const auto from{ line.find_first_not_of(whitespaces) }; from != std::string_view::npos) {
		return line.substr(from, line.find_last_not_of(whitespaces) - from + 1);
	}
	return std::string_view{};
}

[[nodiscard]] auto quoted(const std::string_view line, const std::string_view quote = "\"") -> std::string;

} // namespace lox::utils
