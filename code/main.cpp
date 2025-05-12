#include <cstdio>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "lox/scanner.hpp"
#include "lox/token.hpp"
#include "lox/literal.hpp"
#include "lox/utils/ast_printer.hpp"
#include "lox/utils/rpn_printer.hpp"

void print_literal(const lox::literal &lit) {
	switch (lit.type()) {
		case lox::literal_type::null:
			std::printf("null");
			break;
		case lox::literal_type::boolean:
			std::printf("%s", (*lit.as<bool>() ? "true" : "false"));
			break;
		case lox::literal_type::number:
			std::printf("%f", *lit.as<double>());
			break;
		case lox::literal_type::integral:
			std::printf("%lld", *lit.as<int64_t>());
			break;
		case lox::literal_type::string: {
			const auto str{ *lit.as<std::string_view>() };
			std::printf("%.*s", static_cast<int32_t>(std::size(str)), std::data(str));
		} break;
		default: break;
	}
}

int evaluate(const std::string_view script) {
	lox::scanner scanner{ script };

	auto output{ scanner.scan() };

	const auto [tokens, literals, errors]{ std::move(output) };

	std::printf("Errors:\n");
	for (const auto &error : errors) {
		std::printf("error E0000: %s\n", std::data(error.message));
	}
	std::printf("\nTokens:\n");

	for (const auto &token : tokens) {
		std::printf("at %.3u is %s", token.position, std::data(lox::to_string(token.type)));
		if (lox::invalid_literal_id != token.literal_id) {
			print_literal(literals.at(token.literal_id));
		}
		std::printf("\n");
	}

	return EXIT_SUCCESS;
}

int run_file(const std::string_view path);
int run_prompt();

void test_visitors() {
	std::unique_ptr<lox::expression> expr{ std::make_unique<lox::expression::binary>(
		lox::token{ .type = lox::token_type::star },
		std::make_unique<lox::expression::grouping>(
			std::make_unique<lox::expression::binary>(
				lox::token{ .type = lox::token_type::plus },
				std::make_unique<lox::expression::literal>(lox::literal{ 1 }),
				std::make_unique<lox::expression::literal>(lox::literal{ 2 })
			)
		),
		std::make_unique<lox::expression::grouping>(
			std::make_unique<lox::expression::binary>(
				lox::token{ .type = lox::token_type::minus },
				std::make_unique<lox::expression::literal>(lox::literal{ 3 }),
				std::make_unique<lox::expression::literal>(lox::literal{ 4 })
			)
		)
	)};

	lox::utils::ast_printer ast_print{};
	std::printf("AST: %s\n", std::data(ast_print.print(expr)));

	lox::utils::rpn_printer rpn_print{};
	std::printf("RPN: %s\n", std::data(rpn_print.print(expr)));
}

int main(const int argc, const char *argv[]) {
	test_visitors();

	if (argc > 2) {
		const std::string_view executable{ argv[0] };
		const auto file_name{ executable.substr(executable.find_last_of("/\\") + 1) };
		std::printf("Usage: %.*s [script]",
			static_cast<int32_t>(std::size(file_name)),
			std::data(file_name)
		);
		return 64;
	}

	if (argc == 2) {
		return run_file(argv[1]);
	}
	return run_prompt();
}

int run_file(const std::string_view path) {
	auto file{ std::fopen(std::data(path), "r") };
	if (file == nullptr) {
		std::printf(R"(Failed to open "%s" file: %s)", std::data(path), std::strerror(errno));
		return EXIT_FAILURE;
	}

	if (std::fseek(file, 0, SEEK_END)) {
		std::printf(R"(Failed to seek file "%s": %s)", std::data(path), std::strerror(errno));
		std::fclose(file);
		return EXIT_FAILURE;
	}
	const auto length{ std::ftell(file) };
	if (length == -1) {
		std::printf(R"(Failed to read file "%s": %s)", std::data(path), std::strerror(errno));
		std::fclose(file);
		return EXIT_FAILURE;
	}

	std::fseek(file, 0, SEEK_SET);

	if (constexpr size_t stack_buffer_size{ 256 }; length < stack_buffer_size) {
		std::array<char, stack_buffer_size> buffer{};
		const auto read_bytes{ std::fread(std::data(buffer), sizeof(char), length, file) };
		std::fclose(file);
		return evaluate(std::string_view{ std::data(buffer), read_bytes });
	}

	std::vector<char> buffer(static_cast<size_t>(length));
	const auto read_bytes{ std::fread(std::data(buffer), sizeof(char), length, file) };
	std::fclose(file);
	return evaluate(std::string_view{ std::data(buffer), read_bytes });
}

int run_prompt() {
	std::printf("Lox 1.0.0\n> ");

	for (std::string line{}; std::getline(std::cin, line); std::printf("> ")) {
		std::printf("%d\n", evaluate(line));
	}

	return EXIT_SUCCESS;
}

