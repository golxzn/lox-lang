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
#include "lox/utils/ast_printer.hpp"
#include "lox/utils/rpn_printer.hpp"

enum exit_codes : int32_t {
	ok,
	usage = 64,  /* command line usage error */
	dataerr,     /* data format error */
	noinput,     /* cannot open input */
	nouser,      /* addressee unknown */
	nohost,      /* host name unknown */
	unavailable, /* service unavailable */
	software,    /* internal software error */
	oserr,       /* system error (e.g., can't fork) */
	osfile,      /* critical OS file missing */
	cantcreat,   /* can't create (user) output file */
	ioerr,       /* input/output error */
	tempfail,    /* temp failure; user is invited to retry */
	protocol,    /* remote error in protocol */
	noperm,      /* permission denied */
	config,      /* configuration error */
};

constexpr auto exit_codes_name(exit_codes code) -> std::string_view {
	using namespace std::string_view_literals;
	switch (code) {
		case ok:          return "ok"sv;
		case usage:       return "command line usage error"sv;
		case dataerr:     return "data format error"sv;
		case noinput:     return "cannot open input"sv;
		case nouser:      return "addressee unknown"sv;
		case nohost:      return "host name unknown"sv;
		case unavailable: return "service unavailable"sv;
		case software:    return "internal software error"sv;
		case oserr:       return "system error (e.g., can't fork)"sv;
		case osfile:      return "critical OS file missing"sv;
		case cantcreat:   return "can't create (user) output file"sv;
		case ioerr:       return "input/output error"sv;
		case tempfail:    return "temp failure; user is invited to retry"sv;
		case protocol:    return "remote error in protocol"sv;
		case noperm:      return "permission denied"sv;
		case config:      return "configuration error"sv;
		default: break;
	}
	return "unknown error"sv;
}

void print_literal(const lox::literal &lit) {
	switch (lit.type()) {
		case lox::literal_type::null:
			std::printf(" null");
			break;
		case lox::literal_type::boolean:
			std::printf(" %s", (*lit.as<bool>() ? "true" : "false"));
			break;
		case lox::literal_type::number:
			std::printf(" %f", *lit.as<double>());
			break;
		case lox::literal_type::integral:
			std::printf(" %lld", *lit.as<int64_t>());
			break;
		case lox::literal_type::string: {
			const auto str{ *lit.as<std::string>() };
			std::printf(R"( "%.*s")", static_cast<int32_t>(std::size(str)), std::data(str));
		} break;
		default: break;
	}
}

auto evaluate(const std::string_view file_path, const std::string_view script) -> exit_codes {
	lox::error_handler errout{};

	const auto file_id{ errout.register_file(file_path) };
	lox::scanner scanner{ script, file_id, errout };

	std::printf("\n------------------------ SCANNING -----------------------\n\n");
	auto output{ scanner.scan() };

	const auto [tokens, literals]{ std::move(output) };

	std::printf("Scan Errors:\n");
	errout.export_records([](std::string_view err) {
		std::printf("%.*s\n", static_cast<int32_t>(std::size(err)), std::data(err));
	});
	errout.clear();

	std::printf("\nLiterals:\n");
	for (size_t i{}; i < std::size(literals); ++i) {
		std::printf("%.3zu | ", i);
		print_literal(literals[i]);
		std::printf("\n");
	}

	std::printf("\nTokens:\n");

	for (const auto &token : tokens) {
		std::printf("at %.3u is %s", token.position, std::data(lox::token_name(token.type)));
		if (lox::invalid_literal_id != token.literal_id) {
			print_literal(literals.at(token.literal_id));
		}
		std::printf("\n");
	}


	std::printf("\n------------------------ PARSING ------------------------\n\n");

	lox::parser parser{ tokens, literals, file_id, errout };
	auto expr{ parser.parse() };

	std::printf("Parse Errors:\n");
	errout.export_records([](std::string_view err) {
		std::printf("%.*s\n", static_cast<int32_t>(std::size(err)), std::data(err));
	});
	errout.clear();


	std::printf("AST:\n");
	std::printf("\t%s\n", std::data(lox::utils::ast_printer{}.print(expr)));

	std::printf("\n----------------------- EXECUTION -----------------------\n\n");

	lox::execution::syntax_tree_interpreter interpreter{ file_id, errout };

	const auto value{ interpreter.evaluate(expr) };
	if (interpreter.runtime_error()) {
		std::printf("Runtime Errors:\n");
		errout.export_records([](std::string_view err) {
			std::printf("%.*s\n", static_cast<int32_t>(std::size(err)), std::data(err));
		});
		errout.clear();

		return exit_codes::software;
	}

	std::printf("Output: %s\n", std::data(lox::to_string(value)));

	return exit_codes::ok;
}

auto run_file(const std::string_view path) -> exit_codes;
auto run_prompt() -> exit_codes;

int main(const int argc, const char *argv[]) {
	if (argc > 2) {
		const std::string_view executable{ argv[0] };
		const auto file_name{ executable.substr(executable.find_last_of("/\\") + 1) };
		std::printf("Usage: %.*s [script]",
			static_cast<int32_t>(std::size(file_name)),
			std::data(file_name)
		);
		return exit_codes::usage;
	}

	if (argc == 2) {
		return run_file(argv[1]);
	}
	return run_prompt();
}

auto run_file(const std::string_view path) -> exit_codes {
	auto file{ std::fopen(std::data(path), "r") };
	if (file == nullptr) {
		std::printf(R"(Failed to open "%s" file: %s)", std::data(path), std::strerror(errno));
		return exit_codes::ioerr;
	}

	if (std::fseek(file, 0, SEEK_END)) {
		std::printf(R"(Failed to seek file "%s": %s)", std::data(path), std::strerror(errno));
		std::fclose(file);
		return exit_codes::ioerr;
	}
	const auto length{ std::ftell(file) };
	if (length == -1) {
		std::printf(R"(Failed to read file "%s": %s)", std::data(path), std::strerror(errno));
		std::fclose(file);
		return exit_codes::ioerr;
	}

	std::fseek(file, 0, SEEK_SET);

	if (constexpr size_t stack_buffer_size{ 256 }; length < stack_buffer_size) {
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

auto run_prompt() -> exit_codes {
	std::printf("Lox 1.0.0\n> ");

	for (std::string line{}; std::getline(std::cin, line); std::printf("> ")) {
		const auto exit_code{ evaluate("console", line) };
		std::printf("Result %X: %s\n", exit_code, std::data(exit_codes_name(exit_code)));
	}

	return exit_codes::ok;
}

