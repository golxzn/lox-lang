import os
from io import TextIOWrapper
from argparse import ArgumentParser, Namespace

DISCLAIMER: str = '''/* ~~~ DO NOT MODIFY THIS FILE ~~~
 * This file is auto generated using tools/codegen/generate_ast.py
 */
'''

HEADER_BEGIN: str = '''#pragma once

#include "lox/token.hpp"
#include "lox/literal.hpp"

namespace lox {

'''

CLOSE_NAMESPACE: str = '''

} // namespace lox

'''

SOURCE_BEGIN: str = '''
#include "{header_file_path}"

namespace lox {{

'''

def generate_base_class(file: TextIOWrapper, base_class: str, classes: dict[str, str]) -> None:
	file.write(f'struct {base_class} {{\n')
	file.write(f'\tvirtual ~{base_class}() noexcept = default;\n\n')

	for cls in classes:
		file.write(f'\tclass {cls};\n')

	file.write('\n\tstruct visitor_interface {\n')
	file.write('\n\t\tvirtual ~visitor_interface() noexcept = default;\n\n')

	for cls in classes:
		file.write(f'\t\tvirtual void accept(const {cls} &) {{ /* no impl required */ }}\n')

	file.write('\t};\n\n')
	file.write('\tvirtual void accept(visitor_interface &visitor) = 0;\n')
	file.write('\n};\n')

def generate_derived_class(file: TextIOWrapper, base_class: str, class_name: str, fields: str) -> None:
	file.write(f'struct {base_class}::{class_name} final : public {base_class} {{\n')
	file.write(f'\t{class_name}() = default;\n')

	file.write('\texplicit ' if fields.count(',') == 0 else '\t')
	file.write(f'{class_name}({fields}) noexcept')

	INIT_LIST_TEMPLATE: str = '\n\t\t{sep} {name}{{ std::move({name}) }}'
	SEP_LIST: list[str] = [',', ':']
	fields_list: list[str] = fields.split(',')
	first: bool = True
	for field in fields_list:
		name: str = field.split(' ')[-1]
		file.write(INIT_LIST_TEMPLATE.format(
			sep  = SEP_LIST[first],
			name = name
		))
		first = False
	file.write(f' {{}}\n\n\t~{class_name}() override = default;\n\n')

	file.write('\tvoid accept(visitor_interface &visitor) override;\n\n')

	for field in fields_list:
		file.write('\t')
		file.write(field.strip())
		file.write('{};\n')

	file.write('};\n')

def define_ast(out_dir: str, code_dir: str, base_class: str, classes: dict[str, str]) -> None:
	if not code_dir.endswith('/'):
		code_dir += '/'

	header_file_path: str = os.path.join(out_dir, base_class + '.hpp').replace('\\', '/')
	with open(header_file_path, 'w+') as header:
		header.write(DISCLAIMER)
		header.write(HEADER_BEGIN)
		generate_base_class(header, base_class, classes)
		for (cls, fields) in classes.items():
			header.write('\n')
			generate_derived_class(header, base_class, cls, fields)

		header.write(CLOSE_NAMESPACE)

	with open(os.path.join(out_dir, base_class + '.cpp'), 'w+') as source:
		source.write(DISCLAIMER)
		source.write(SOURCE_BEGIN.format(
			header_file_path = header_file_path.replace(code_dir, '')
		))

		for cls in classes:
			source.write(f"void {base_class}::{cls}::accept(visitor_interface &visitor) {{\n")
			source.write('\tvisitor.accept(*this);\n')
			source.write('}\n\n')

		source.write(CLOSE_NAMESPACE)

def main(args: Namespace) -> int:
	output_path: str = args.output
	os.makedirs(output_path, exist_ok=True)

	define_ast(output_path.replace('\\', '/'), args.code.replace('\\', '/'), 'expression', {
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
	parser.add_argument('-o', '--output', help='The path to output directory')
	parser.add_argument('--code', help='The part of output path which should be deleted in case of #include')
	exit(main(parser.parse_args()))