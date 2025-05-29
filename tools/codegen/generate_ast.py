import os
import utils
import constants
from configuratoin import configuration
from io import TextIOWrapper
from argparse import ArgumentParser, Namespace

def generate_types_enum(file: TextIOWrapper, base_class: str, classes: dict[str, str]) -> None:
	file.write(f'enum class {base_class}_type : uint8_t {{\n')
	for cls in classes:
		(conditions, class_name) = utils.split_conditions_and_class_name(cls)
		utils.open_conditions(conditions, file)
		file.write(f'\t{class_name},\n')
		utils.close_conditions(conditions, file)

	file.write('};\n')

def generate_base_class(file: TextIOWrapper, base_class: str, classes: dict[str, str]) -> None:
	file.write(f'template<{base_class}_type Type>\n')
	file.write(f'struct LOX_EXPORT {base_class};\n\n')

	file.write('template<class OutputType>\n')
	file.write(f'struct {base_class}_visitor_interface {{\n')
	file.write('\tusing output_type = OutputType;\n\n')
	file.write(f'\tvirtual ~{base_class}_visitor_interface() noexcept = default;\n\n')

	for cls in classes:
		(conditions, class_name) = utils.split_conditions_and_class_name(cls)
		utils.open_conditions(conditions, file) # TODO Use copy for expressions and const ref for statements
		parameter: str = f'const {base_class}<{base_class}_type::{class_name}> &'
		file.write(f'\t[[nodiscard]] virtual auto accept({parameter}) -> output_type = 0;\n')
		utils.close_conditions(conditions, file)

	file.write('};\n\n')

def generate_derived_class_definition(file: TextIOWrapper, base_class: str, cls_name: str, fields: str) -> None:
	(conditions, class_name) = utils.split_conditions_and_class_name(cls_name)
	enum_name: str = f'{base_class}_type'

	utils.open_conditions(conditions, file)
	file.write(f'template<> struct LOX_EXPORT {base_class}<{enum_name}::{class_name}> {{\n')
	file.write(f'\tstatic constexpr auto type{{ {enum_name}::{class_name} }};\n\n')
	# file.write(f'\t{class_name}() = default;\n')

	# file.write('\texplicit ' if fields.count(',') == 0 else '\t')
	# file.write(f'{class_name}({fields}) noexcept;\n\n')
	# file.write(f'\t~{class_name}() override = default;\n\n')

	for field in fields.split(','):
		file.write(f'\t{field.strip()}{{}};\n')

	# file.write(f'\t[[nodiscard]] static auto type() noexcept -> {enum_name} {{ return {enum_name}::{class_name}; }}\n')
	file.write('};\n')
	file.write(f'using {base_class}_{class_name} = {base_class}<{enum_name}::{class_name}>;\n')

	utils.close_conditions(conditions, file)

def define_ast(out_dir: str, includes: list[str], base_class: str, classes: dict[str, str]) -> str:
	header_file_path: str = os.path.join(out_dir, constants.INCLUDE_DIR, base_class + '.hpp').replace('\\', '/')
	include_path: str = utils.make_include_path(base_class)

	with open(header_file_path, 'w+') as header:
		header.write(constants.DISCLAIMER.format(generator_filename = utils.get_filename(__file__)))
		header.write(constants.HEADER_BEGIN)
		for include in includes:
			header.write(f'\n#include {include}')

		header.write(constants.OPEN_NAMESPACE)

		generate_types_enum(header, base_class, classes)
		header.write(f'using {base_class}_id = ID<{base_class}_type>;\n\n')
		generate_base_class(header, base_class, classes)

		for (cls, fields) in classes.items():
			header.write('\n')
			generate_derived_class_definition(header, base_class, cls, fields)

		header.write(constants.CLOSE_NAMESPACE)

	return include_path


def main(args: Namespace) -> int:
	output_path: str = args.output.replace('\\', '/')
	os.makedirs(os.path.join(output_path, constants.INCLUDE_DIR), exist_ok=True)

	config = configuration(args.config)

	include: str = define_ast(output_path, config.expression_includes, config.expression_class, config.expressions)
	define_ast(output_path, config.statement_includes + [include], config.statement_class, config.statements)

	return 0

if __name__ == '__main__':
	parser: ArgumentParser = ArgumentParser(
		prog='generate_ast',
		description='Utility to generate the abstract syntax tree C++ sources'
	)
	parser.add_argument('--output', type=str, help='The path to output directory')
	parser.add_argument('--config', type=str, help='The path to the generation info JSON file')
	exit(main(parser.parse_args()))
