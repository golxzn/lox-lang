import os
from io import TextIOWrapper
from argparse import ArgumentParser, Namespace

EXPRESSION_CLASS: str = 'expression'
EXPRESSION_PTR: str = f'std::unique_ptr<lox::{EXPRESSION_CLASS}>'
EXPRESSIONS: dict[str, str] = {
	'unary'     : f'token op, {EXPRESSION_PTR} expr',
	'assignment': f'token name, {EXPRESSION_PTR} value',
	'binary'    : f'token op, {EXPRESSION_PTR} left, {EXPRESSION_PTR} right',
	'grouping'  : f'{EXPRESSION_PTR} expr',
	'literal'   : 'lox::literal value',
	'logical'   : f'token op, {EXPRESSION_PTR} left, {EXPRESSION_PTR} right',
	'identifier': 'token name'
}
EXPRESSIONS_INCLUDES: list[str] = [ '<memory>' ]

STATEMENT_CLASS: str = 'statement'
STATEMENT_PTR: str = f'std::unique_ptr<{STATEMENT_CLASS}>'
STATEMENTS: dict[str, str] = {
	'scope'          : f'std::vector<{STATEMENT_PTR}> statements',
	'expression'     : f'{EXPRESSION_PTR} expr',
	'branch'         : f'{EXPRESSION_PTR} condition, {STATEMENT_PTR} then_branch, {STATEMENT_PTR} else_branch',
	'variable'       : f'token identifier, {EXPRESSION_PTR} initializer',
	'constant'       : f'token identifier, {EXPRESSION_PTR} initializer',
	'loop'           : f'{EXPRESSION_PTR} condition, {STATEMENT_PTR} body',
	'LOX_DEBUG!print': f'{EXPRESSION_PTR} expr'
}
STATEMENT_INCLUDES: list[str] = [ '<vector>' ]

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

def split_conditions_and_class_name(cls_name: str) -> tuple[list[str], str]:
	conditions: list[str] = cls_name.split('!')
	class_name: str = conditions[-1]
	conditions.remove(class_name)
	return (conditions, class_name)

def open_conditions(conditions: list[str], file: TextIOWrapper) -> None:
	for condition in conditions:
		file.write(f'#if defined({condition})\n')

def close_conditions(conditions: list[str], file: TextIOWrapper) -> None:
	for condition in conditions:
		file.write(f'#endif // defined({condition})\n')

def generate_tags(file: TextIOWrapper, classes: dict[str, str]) -> None:
	file.write("\tenum class tag : uint8_t {\n")
	for cls in classes:
		(conditions, class_name) = split_conditions_and_class_name(cls)
		open_conditions(conditions, file)
		file.write(f'\t\t {class_name},\n')
		close_conditions(conditions, file)

	file.write("\t};\n")

def generate_base_class(file: TextIOWrapper, base_class: str, classes: dict[str, str]) -> None:
	file.write(f'struct LOX_EXPORT {base_class} {{\n')
	file.write(f'\tvirtual ~{base_class}() noexcept = default;\n\n')

	generate_tags(file, classes)

	for cls in classes:
		(conditions, class_name) = split_conditions_and_class_name(cls)
		open_conditions(conditions, file)
		file.write(f'\tstruct LOX_EXPORT {class_name};\n')
		close_conditions(conditions, file)

	file.write('\n\tstruct visitor_interface {\n')
	file.write('\t\tvirtual ~visitor_interface() noexcept = default;\n\n')

	for cls in classes:
		(conditions, class_name) = split_conditions_and_class_name(cls)
		open_conditions(conditions, file)
		file.write(f'\t\tvirtual void accept(const {class_name} &) {{ /* no impl required */ }}\n')
		close_conditions(conditions, file)

	file.write('\t};\n\n')
	file.write('\tvirtual void accept(visitor_interface &visitor) = 0;\n')
	file.write('\t[[nodiscard]] virtual auto type() const noexcept -> tag = 0;\n')
	file.write('\n};\n')

def generate_derived_class_definition(file: TextIOWrapper, base_class: str, cls_name: str, fields: str) -> None:
	(conditions, class_name) = split_conditions_and_class_name(cls_name)

	open_conditions(conditions, file)
	file.write(f'struct LOX_EXPORT {base_class}::{class_name} final : public {base_class} {{\n')
	file.write(f'\t{class_name}() = default;\n')

	file.write('\texplicit ' if fields.count(',') == 0 else '\t')
	file.write(f'{class_name}({fields}) noexcept;\n\n')
	file.write(f'\t~{class_name}() override = default;\n\n')

	file.write('\tvoid accept(visitor_interface &visitor) override;\n\n')
	file.write(f'\t[[nodiscard]] auto type() const noexcept -> tag override {{ return tag::{class_name}; }}\n\n')

	for field in fields.split(','):
		file.write('\t')
		file.write(field.strip())
		file.write('{};\n')

	file.write('};\n')
	close_conditions(conditions, file)

def generate_derived_class_implementation(file: TextIOWrapper, base_class: str, cls_name: str, fields: str) -> None:
	(conditions, class_name) = split_conditions_and_class_name(cls_name)

	open_conditions(conditions, file)
	file.write(f'\n{base_class}::{class_name}::{class_name}({fields}) noexcept')

	INIT_LIST_TEMPLATE: str = '\n\t{sep} {name}{{ std::move({name}) }}'
	SEP_LIST: list[str] = [',', ':']
	fields_list: list[str] = fields.split(',')
	first: bool = True
	for field in fields_list:
		name: str = field.split(' ')[-1].strip()
		file.write(INIT_LIST_TEMPLATE.format(
			sep  = SEP_LIST[first],
			name = name
		))
		first = False

	file.write(' {}\n\n')

	file.write(f"void {base_class}::{class_name}::accept(visitor_interface &visitor) {{\n")
	file.write('\tvisitor.accept(*this);\n')
	file.write('}\n\n')
	close_conditions(conditions, file)


def define_ast(out_dir: str, includes: list[str], base_class: str, classes: dict[str, str]) -> str:
	header_file_path: str = os.path.join(out_dir, INCLUDE_DIR, base_class + '.hpp').replace('\\', '/')
	include_path: str = os.path.join(INCLUDE_DIR.removeprefix('include/'), base_class + '.hpp').replace('\\', '/')

	with open(header_file_path, 'w+') as header:
		header.write(DISCLAIMER)
		header.write(HEADER_BEGIN)
		for include in includes:
			header.write(f'\n#include {include}')

		header.write(OPEN_NAMESPACE)
		generate_base_class(header, base_class, classes)
		for (cls, fields) in classes.items():
			header.write('\n')
			generate_derived_class_definition(header, base_class, cls, fields)

		header.write(CLOSE_NAMESPACE)

	source_file_path: str = os.path.join(out_dir, SOURCES_DIR, base_class + '.cpp')
	with open(source_file_path, 'w+') as source:
		source.write(DISCLAIMER)
		source.write(SOURCE_BEGIN.format(
			header_file_path = include_path
		))
		source.write(OPEN_NAMESPACE)

		for (class_name, fields) in classes.items():
			generate_derived_class_implementation(source, base_class, class_name, fields)

		source.write(CLOSE_NAMESPACE)

	return f'"{include_path}"'

def main(args: Namespace) -> int:
	output_path: str = args.output.replace('\\', '/')
	os.makedirs(os.path.join(output_path, INCLUDE_DIR), exist_ok=True)
	os.makedirs(os.path.join(output_path, SOURCES_DIR), exist_ok=True)

	include: str = define_ast(output_path, EXPRESSIONS_INCLUDES, EXPRESSION_CLASS, EXPRESSIONS)
	define_ast(output_path, STATEMENT_INCLUDES + [include], STATEMENT_CLASS, STATEMENTS)

	return 0

if __name__ == '__main__':
	parser: ArgumentParser = ArgumentParser(
		prog='generate_ast',
		description='Utility to generate the abstract syntax tree C++ sources'
	)
	parser.add_argument('--output', type=str,  help='The path to output directory')
	exit(main(parser.parse_args()))