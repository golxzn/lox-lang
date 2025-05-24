#pragma once

#include <string>
#include <string_view>

namespace lox::utils {

constexpr std::string_view whitespaces{ " \t\r\n" };

struct string_hash {
	using is_transparent = void;

	[[nodiscard]] constexpr auto operator()(const char *txt) const noexcept -> size_t {
		return std::hash<std::string_view>{}(txt);
	}
	[[nodiscard]] constexpr auto operator()(std::string_view txt) const noexcept -> size_t {
		return std::hash<std::string_view>{}(txt);
	}
	[[nodiscard]] constexpr auto operator()(const std::string &txt) const noexcept -> size_t {
		return std::hash<std::string>{}(txt);
	}
};


[[nodiscard]] constexpr auto strip(const std::string_view line) -> std::string_view {
	if (const auto from{ line.find_first_not_of(whitespaces) }; from != std::string_view::npos) {
		return line.substr(from, line.find_last_not_of(whitespaces) - from + 1);
	}
	return std::string_view{};
}

[[nodiscard]] auto quoted(const std::string_view line, const std::string_view quote = "\"") -> std::string;

} // namespace lox::utils
