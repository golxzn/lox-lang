#pragma once

#include "lox/types/function.hpp"

namespace lox {
class LOX_EXPORT lexeme_database;
} // namespace lox
namespace lox::execution {
class LOX_EXPORT environment;
} // namespace lox::execution

namespace lox::native {

/** @return count of registered functions */
auto add_native_functions(lexeme_database &db, execution::environment &env) -> size_t;


[[nodiscard]] auto fn_print_impl(void *, std::span<const literal>) -> literal;
[[nodiscard]] auto fn_println_impl(void *, std::span<const literal>) -> literal;
[[nodiscard]] auto fn_time_now_impl(void *, std::span<const literal>) -> literal;

const function fn_print   { fn_print_impl };
const function fn_println { fn_println_impl };
const function fn_time_now{ fn_time_now_impl, 0ull };

} // namespace lox::native
