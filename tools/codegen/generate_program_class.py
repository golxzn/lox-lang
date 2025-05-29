import os
import utils
import constants
from configuratoin import configuration
from io import TextIOWrapper
from argparse import ArgumentParser, Namespace

CLASS_NAME: str = 'program_base'

DEFAULT_RULE_OF_FIVE: str = '''
	{class_name}() = default;

	{class_name}(const {class_name} &) = default;
	{class_name}({class_name} &&) noexcept = default;

	auto operator=(const {class_name} &) -> {class_name} & = default;
	auto operator=({class_name} &&) noexcept -> {class_name} & = default;

'''

DEFAULT_GETTERS_METHODS: str = '''
	[[nodiscard]] decltype(auto) size() const noexcept {{ return std::size({stmts_name}); }}
	[[nodiscard]] decltype(auto) empty() const noexcept {{ return std::empty({stmts_name}); }}

	[[nodiscard]] decltype(auto) begin() const noexcept {{ return std::begin({stmts_name}); }}
	[[nodiscard]] decltype(auto) end() const noexcept {{ return std::end({stmts_name}); }}
	[[nodiscard]] decltype(auto) begin() noexcept {{ return std::begin({stmts_name}); }}
	[[nodiscard]] decltype(auto) end() noexcept {{ return std::end({stmts_name}); }}
'''

def define_records_struct(file: TextIOWrapper, base_class: str, types: dict[str, str]) -> str:
	name: str = f'{base_class}s_records'
	file.write(f'struct {name} {{\n')

	for type in types:
		(conditions, class_name) = utils.split_conditions_and_class_name(type)
		utils.open_conditions(conditions, file)
		type: str = f'{base_class}_{class_name}'
		file.write(f'\tstd::vector<{type}> {class_name}s{{}};\n')
		utils.close_conditions(conditions, file)

	file.write('};\n\n')
	return name

def make_get_impl(file: TextIOWrapper, base_class: str, type_name: str, class_name: str):
	file.write(f'template<> inline decltype(auto) {base_class}::get_{type_name}s<{type_name}_type::{class_name}>() const noexcept {{\n')
	file.write(f'\treturn std::cref({type_name}s_db.{class_name}s).get();\n')
	file.write('}\n\n')

	file.write(f'template<> inline decltype(auto) {base_class}::get_{type_name}s<{type_name}_type::{class_name}>() noexcept {{\n')
	file.write(f'\treturn std::ref({type_name}s_db.{class_name}s).get();\n')
	file.write('}\n')



def define_get_instantiations(file: TextIOWrapper, base_class: str, type_name: str, types: dict[str, str]):
	for type in types:
		(conditions, class_name) = utils.split_conditions_and_class_name(type)
		utils.open_conditions(conditions, file)
		make_get_impl(file, base_class, type_name, class_name)
		utils.close_conditions(conditions, file)
		file.write('\n')

def define_get_with_id(file: TextIOWrapper, type_name: str):
	file.write(f'\ttemplate<{type_name}_type Type>\n')
	file.write(f'\t[[nodiscard]] decltype(auto) get_{type_name}s(ID<decltype(Type)> id) const {{\n')
	file.write('\t\tif (id.type == Type) {\n')
	file.write(f'\t\t\treturn get_{type_name}s<Type>().at(id.index);\n')
	file.write('\t\t}\n\n')
	file.write('\t\tthrow std::invalid_argument{ "Invalid type id" };\n')
	file.write('\t}\n\n')

	file.write(f'\ttemplate<{type_name}_type Type>\n')
	file.write(f'\t[[nodiscard]] decltype(auto) get_{type_name}s(ID<decltype(Type)> id) {{\n')
	file.write('\t\tif (id.type == Type) {\n')
	file.write(f'\t\t\treturn get_{type_name}s<Type>().at(id.index);\n')
	file.write('\t\t}\n\n')
	file.write('\t\tthrow std::invalid_argument{ "Invalid type id" };\n')
	file.write('\t}\n\n')


def define_accept_instantiations(file: TextIOWrapper, base_class: str, type_name: str, types: dict[str, str]):
	file.write('template<class T>\n')
	file.write(f'auto {base_class}::accept({type_name}_visitor_interface<T> &visitor, ID<{type_name}_type> id) const -> T {{\n')
	file.write(f'\tswitch (id.type) {{\n\t\tusing enum {type_name}_type;\n\n')

	space: int = max(len(utils.split_conditions_and_class_name(key)[1]) for key in types)

	for type in types:
		(conditions, class_name) = utils.split_conditions_and_class_name(type)
		utils.open_conditions(conditions, file)
		file.write(f'\t\tcase {class_name:<{space}}: return visitor.accept(get_{type_name}s<{class_name:<{space}}>(id));\n')
		utils.close_conditions(conditions, file)

	file.write('\n\t\tdefault: break;\n')

	file.write('\t}\n')

	file.write('\tif constexpr (!std::is_void_v<T>) { return {}; }\n')
	# file.write('\treturn {{}};\n')
	file.write('}\n\n')

def define_program_class(out_dir: str, class_name: str, config: configuration) -> None:
	header_file_path: str = os.path.join(out_dir, constants.INCLUDE_DIR, class_name + '.hpp').replace('\\', '/')
	statements_include: str = utils.make_include_path(config.statement_class)

	with open(header_file_path, 'w+') as header:
		header.write(constants.DISCLAIMER.format(generator_filename = utils.get_filename(__file__)))
		header.write(constants.HEADER_BEGIN)
		header.write('\n#include <vector>\n')
		header.write('#include <stdexcept>\n')
		header.write('#include <type_traits>\n')
		header.write(f'#include {statements_include}\n')
		header.write(constants.OPEN_NAMESPACE)

		exprs_records: str = define_records_struct(header, config.expression_class, config.expressions)
		stmts_records: str = define_records_struct(header, config.statement_class, config.statements)

		header.write(f'class {class_name} {{\npublic:\n')
		header.write(f'\tusing {config.statement_class}_list = std::vector<{config.statement_class}_id>;\n\n')

		header.write(f'\t{class_name}(\n')
		header.write(f'\t\t{exprs_records} {config.expression_class}s_db,\n')
		header.write(f'\t\t{stmts_records} {config.statement_class}s_db,\n')
		header.write(f'\t\t{config.statement_class}_list {config.statement_class}s\n')
		header.write('\t) noexcept\n')
		header.write(f'\t\t: {config.expression_class}s_db{{ std::move({config.expression_class}s_db) }}\n')
		header.write(f'\t\t, {config.statement_class}s_db{{ std::move({config.statement_class}s_db) }}\n')
		header.write(f'\t\t, {config.statement_class}s{{ std::move({config.statement_class}s) }}\n')
		header.write('\t{}\n')

		header.write(DEFAULT_RULE_OF_FIVE.format(class_name = class_name))

		header.write(f'\ttemplate<{config.statement_class}_type>\n')
		header.write(f'\t[[nodiscard]] inline decltype(auto) get_{config.statement_class}s() const noexcept;\n\n')

		header.write(f'\ttemplate<{config.statement_class}_type>\n')
		header.write(f'\t[[nodiscard]] inline decltype(auto) get_{config.statement_class}s() noexcept;\n\n')

		header.write(f'\ttemplate<{config.expression_class}_type>\n')
		header.write(f'\t[[nodiscard]] inline decltype(auto) get_{config.expression_class}s() const noexcept;\n\n')

		header.write(f'\ttemplate<{config.expression_class}_type>\n')
		header.write(f'\t[[nodiscard]] inline decltype(auto) get_{config.expression_class}s() noexcept;\n\n')

		define_get_with_id(header, config.expression_class)
		define_get_with_id(header, config.statement_class)

		header.write('\ttemplate<class T>\n')
		header.write(f'\t[[nodiscard]] auto accept({config.expression_class}_visitor_interface<T> &visitor, ID<{config.expression_class}_type> id) const -> T;\n\n')
		header.write('\ttemplate<class T>\n')
		header.write(f'\t[[nodiscard]] auto accept({config.statement_class}_visitor_interface<T> &visitor, ID<{config.statement_class}_type> id) const -> T;\n')

		header.write(DEFAULT_GETTERS_METHODS.format(stmts_name = f'{config.statement_class}s'))

		header.write('protected:\n')
		header.write(f'\t{exprs_records} {config.expression_class}s_db{{}};\n')
		header.write(f'\t{stmts_records} {config.statement_class}s_db{{}};\n')
		header.write(f'\t{config.statement_class}_list {config.statement_class}s{{}};\n')

		header.write('};\n\n')

		define_get_instantiations(header, class_name, config.statement_class, config.statements)
		define_get_instantiations(header, class_name, config.expression_class, config.expressions)

		define_accept_instantiations(header, class_name, config.statement_class, config.statements)
		define_accept_instantiations(header, class_name, config.expression_class, config.expressions)

		header.write(constants.CLOSE_NAMESPACE)

	# include_path: str = '"' + os.path.join(INCLUDE_DIR.removeprefix('include/'), class_name + '.hpp"').replace('\\', '/')

def main(args: Namespace) -> int:
	output_path: str = args.output.replace('\\', '/')
	os.makedirs(os.path.join(output_path, constants.INCLUDE_DIR), exist_ok=True)
	os.makedirs(os.path.join(output_path, constants.SOURCES_DIR), exist_ok=True)

	config = configuration(args.config)

	define_program_class(output_path, CLASS_NAME, config)

	return 0

if __name__ == '__main__':
	parser: ArgumentParser = ArgumentParser(
		prog='generate_ast',
		description='Utility to generate the abstract syntax tree C++ sources'
	)
	parser.add_argument('--output', type=str, help='The path to output directory')
	parser.add_argument('--config', type=str, help='The path to the generation info JSON file')
	exit(main(parser.parse_args()))

