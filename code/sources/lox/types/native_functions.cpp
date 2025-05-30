#include <string>
#include <cstdio>
#include <chrono>

#include "lox/types/native_functions.hpp"
#include "lox/execution/environment.hpp"
#include "lox/lexeme_database.hpp"

namespace lox::native {

auto add_native_functions(lexeme_database &db, execution::environment &env) -> size_t {
	constexpr auto to_sz{ [](bool val) { return static_cast<size_t>(val); } };

	return 0ull
		+ to_sz(env.register_function(db.add("print"), fn_print))
		+ to_sz(env.register_function(db.add("println"), fn_println))
		+ to_sz(env.register_function(db.add("time"), fn_time_now))
	;
}

auto fn_print_impl(void *, std::span<const literal> args) -> literal {
	for (const auto &arg : args) {
		switch (arg.type()) {
			using enum literal_type;

			case null:     std::fprintf(stdout, "null");
			case boolean:  std::fprintf(stdout, (std::get<bool>(arg) ? "true" : "false")); break;
			case number:   std::fprintf(stdout, "%f", std::get<double>(arg)); break;
			case integral: std::fprintf(stdout, "%d", std::get<int64_t>(arg)); break;
			case string:   std::fprintf(stdout, "%s", std::data(std::get<std::string>(arg))); break;

			default: break;
		}
	}
	return null_literal;
}

auto fn_println_impl(void *_, std::span<const literal> args) -> literal {
	fn_print_impl(_, args);
	std::fprintf(stdout, "\n");
	return null_literal;
}

auto fn_time_now_impl(void *, std::span<const literal>) -> literal {
	using i64_milli = std::chrono::duration<int64_t, std::milli>;
	return std::chrono::duration_cast<i64_milli>(
		std::chrono::high_resolution_clock::now().time_since_epoch()
	).count();
}

} // namespace lox::native
