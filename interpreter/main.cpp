#include <cstdio>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "lox/token.hpp"
#include "lox/literal.hpp"
#include "lox/scanner.hpp"
#include "lox/parser.hpp"
#include "lox/execution/syntax_tree_interpreter.hpp"
#include "lox/utils/exit_codes.hpp"
#include "lox/utils/ast_printer.hpp"
#include "lox/utils/rpn_printer.hpp"

void print_literal(const lox::literal &lit) {
	std::printf(" %s", std::data(lox::to_string(lit)));
}

auto evaluate(const std::string_view file_path, const std::string_view script) -> lox::utils::exit_codes {
	lox::error_handler errout{ std::string{ file_path }, script };

	lox::scanner scanner{ script, errout };

	// std::printf("\n------------------------ SCANNING -----------------------\n\n");
	const auto ctx{ scanner.scan() };

	if (!std::empty(errout)) {
		std::printf("Scan Errors:\n");
		errout.export_records([](std::string_view err) {
			std::printf("%.*s\n", static_cast<int32_t>(std::size(err)), std::data(err));
		});
		errout.clear();
	}

	// std::printf("\nLiterals:\n");
	// for (size_t i{}; i < std::size(ctx.literals); ++i) {
	// 	std::printf("%.3zu | ", i);
	// 	print_literal(ctx.literals[i]);
	// 	std::printf("\n");
	// }

	// std::printf("\n------------------------ PARSING ------------------------\n\n");

	lox::parser parser{ ctx, errout };
	auto program{ parser.parse() };

	if (!std::empty(errout)) {
		std::printf("Parse Errors:\n");
		errout.export_records([](std::string_view err) {
			std::printf("%.*s\n", static_cast<int32_t>(std::size(err)), std::data(err));
		});
		errout.clear();
	}

	// std::printf("\n----------------------- EXECUTION -----------------------\n\n");

	lox::execution::syntax_tree_interpreter interpreter{ ctx.lexemes, errout };

	if (const auto status{ interpreter.run(program) }; status != lox::execution::status::ok || !std::empty(errout)) {
		std::printf("Runtime Errors:\n");
		errout.export_records([](std::string_view err) {
			std::printf("%.*s\n", static_cast<int32_t>(std::size(err)), std::data(err));
		});
		errout.clear();

		return lox::utils::exit_codes::software;
	}

	return lox::utils::exit_codes::ok;
}

auto run_file(const std::string_view path) -> lox::utils::exit_codes;
auto run_prompt() -> lox::utils::exit_codes;

int main(const int argc, const char *argv[]) {
	using lox::utils::as_int;

	if (argc > 2) {
		const std::string_view executable{ argv[0] };
		const auto file_name{ executable.substr(executable.find_last_of("/\\") + 1) };
		std::printf("Usage: %.*s [script]",
			static_cast<int32_t>(std::size(file_name)),
			std::data(file_name)
		);
		return as_int(lox::utils::exit_codes::usage);
	}

	if (argc == 2) {
		return as_int(run_file(argv[1]));
	}
	return as_int(run_prompt());
}

auto run_file(const std::string_view path) -> lox::utils::exit_codes {
	auto file{ std::fopen(std::data(path), "r") };
	if (file == nullptr) {
		std::printf(R"(Failed to open "%s" file: %s)", std::data(path), std::strerror(errno));
		return lox::utils::exit_codes::ioerr;
	}

	if (std::fseek(file, 0, SEEK_END)) {
		std::printf(R"(Failed to seek file "%s": %s)", std::data(path), std::strerror(errno));
		std::fclose(file);
		return lox::utils::exit_codes::ioerr;
	}
	const auto length{ std::ftell(file) };
	if (length == -1) {
		std::printf(R"(Failed to read file "%s": %s)", std::data(path), std::strerror(errno));
		std::fclose(file);
		return lox::utils::exit_codes::ioerr;
	}

	std::fseek(file, 0, SEEK_SET);

	if (constexpr long stack_buffer_size{ 256 }; length < stack_buffer_size) {
		std::array<char, stack_buffer_size> buffer{};
		const auto read_bytes{ std::fread(std::data(buffer), sizeof(char), length, file) };
		std::fclose(file);
		return evaluate(path, std::string_view{ std::data(buffer), read_bytes });
	}

	std::vector<char> buffer(static_cast<size_t>(length));
	const auto read_bytes{ std::fread(std::data(buffer), sizeof(char), length, file) };
	std::fclose(file);
	return evaluate(path, std::string_view{ std::data(buffer), read_bytes });
}

auto run_prompt() -> lox::utils::exit_codes {
	std::printf("Lox 1.0.0\n> ");

	for (std::string line{}; std::getline(std::cin, line); std::printf("> ")) {
		const auto exit_code{ evaluate("console", line) };
		std::printf("Result %X: %s\n", static_cast<uint32_t>(exit_code), std::data(exit_codes_name(exit_code)));
	}

	return lox::utils::exit_codes::ok;
}

