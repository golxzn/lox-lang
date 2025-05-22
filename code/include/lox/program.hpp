#pragma once

#include <vector>

#include "lox/gen/statement.hpp"

namespace lox {

using program = std::vector<std::unique_ptr<statement>>;

} // namespace lox
