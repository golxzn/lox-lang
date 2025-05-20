import os
from io import TextIOWrapper
from argparse import ArgumentParser, Namespace

INCLUDE_DIR: str = 'include/lox/gen/'
SOURCES_DIR: str = 'sources/lox/gen/'

DISCLAIMER: str = '''/* ~~~ DO NOT MODIFY THIS FILE ~~~
 * This file is auto generated using tools/codegen/generate_ast.py
 */
'''

HEADER_BEGIN: str = '''#pragma once

#include "lox/export.hpp"
#include "lox/token.hpp"
#include "lox/literal.hpp"'''

OPEN_NAMESPACE: str = '''

namespace lox {

'''

CLOSE_NAMESPACE: str = '''
} // namespace lox

'''

SOURCE_BEGIN: str = '''#include "{header_file_path}"'''

def generate_base_class(file: TextIOWrapper, base_class: str, classes: dict[str, str]) -> None:
	file.write(f'struct LOX_EXPORT {base_class} {{\n')
	file.write(f'\tvirtual ~{base_class}() noexcept = default;\n\n')

	for cls in classes:
		file.write(f'\tstruct LOX_EXPORT {cls};\n')

	file.write('\n\tstruct visitor_interface {\n')
	file.write('\t\tvirtual ~visitor_interface() noexcept = default;\n\n')

	for cls in classes:
		file.write(f'\t\tvirtual void accept(const {cls} &) {{ /* no impl required */ }}\n')

	file.write('\t};\n\n')
	file.write('\tvirtual void accept(visitor_interface &visitor) = 0;\n')
	file.write('\n};\n')

def generate_derived_class(file: TextIOWrapper, base_class: str, class_name: str, fields: str) -> None:
	file.write(f'struct LOX_EXPORT {base_class}::{class_name} final : public {base_class} {{\n')
	file.write(f'\t{class_name}() = default;\n')

	file.write('\texplicit ' if fields.count(',') == 0 else '\t')
	file.write(f'{class_name}({fields}) noexcept;\n\n')
	file.write(f'\t~{class_name}() override = default;\n\n')

	file.write('\tvoid accept(visitor_interface &visitor) override;\n\n')

	for field in fields.split(','):
		file.write('\t')
		file.write(field.strip())
		file.write('{};\n')

	file.write('};\n')

def define_ast(out_dir: str, includes: list[str], base_class: str, classes: dict[str, str]) -> str:
	header_file_path: str = os.path.join(out_dir, INCLUDE_DIR, base_class + '.hpp').replace('\\', '/')
	include_path: str = os.path.join(INCLUDE_DIR, base_class + '.hpp').replace('\\', '/')
	with open(header_file_path, 'w+') as header:
		header.write(DISCLAIMER)
		header.write(HEADER_BEGIN)
		for include in includes:
			header.write(f'\n#include "{include}"')

		header.write(OPEN_NAMESPACE)
		generate_base_class(header, base_class, classes)
		for (cls, fields) in classes.items():
			header.write('\n')
			generate_derived_class(header, base_class, cls, fields)

		header.write(CLOSE_NAMESPACE)

	source_file_path: str = os.path.join(out_dir, SOURCES_DIR, base_class + '.cpp')
	with open(source_file_path, 'w+') as source:
		source.write(DISCLAIMER)
		source.write(SOURCE_BEGIN.format(
			header_file_path = include_path
		))
		source.write(OPEN_NAMESPACE)

		for (class_name, fields) in classes.items():
			source.write(f'\n{base_class}::{class_name}::{class_name}({fields}) noexcept')

			INIT_LIST_TEMPLATE: str = '\n\t{sep} {name}{{ std::move({name}) }}'
			SEP_LIST: list[str] = [',', ':']
			fields_list: list[str] = fields.split(',')
			first: bool = True
			for field in fields_list:
				name: str = field.split(' ')[-1].strip()
				source.write(INIT_LIST_TEMPLATE.format(
					sep  = SEP_LIST[first],
					name = name
				))
				first = False

			source.write(' {}\n\n')

			source.write(f"void {base_class}::{class_name}::accept(visitor_interface &visitor) {{\n")
			source.write('\tvisitor.accept(*this);\n')
			source.write('}\n\n')

		source.write(CLOSE_NAMESPACE)

	return include_path

def main(args: Namespace) -> int:
	output_path: str = args.output.replace('\\', '/')
	os.makedirs(os.path.join(output_path, INCLUDE_DIR), exist_ok=True)
	os.makedirs(os.path.join(output_path, SOURCES_DIR), exist_ok=True)

	define_ast(output_path, [], 'expression', {
		'unary'   : 'token op, std::unique_ptr<expression> expr',
		'binary'  : 'token op, std::unique_ptr<expression> left, std::unique_ptr<expression> right',
		'grouping': 'std::unique_ptr<expression> expr',
		'literal' : 'lox::literal value'
	})

	return 0

if __name__ == '__main__':
	parser: ArgumentParser = ArgumentParser(
		prog='generate_ast',
		description='Utility to generate the abstract syntax tree C++ sources'
	)
	parser.add_argument('--output', help='The path to output directory')
	exit(main(parser.parse_args()))