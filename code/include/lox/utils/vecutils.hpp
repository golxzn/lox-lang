#pragma once

#include <vector>
#include <memory>
#include <concepts>

namespace lox::utils {

template<class T, std::derived_from<T> ...Values>
constexpr decltype(auto) make_pointers_vector(std::unique_ptr<Values> ...elements) {
	std::vector<std::unique_ptr<T>> vector{};
	vector.reserve(sizeof...(elements));
	( vector.emplace_back(std::move(elements)), ... );
	return vector;
}

} // namespace lox::utils
