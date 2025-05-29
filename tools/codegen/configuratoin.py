import json

class configuration:
	__slots__ = [
		'config',
		'expression_class', 'expressions', 'expression_includes',
		'statement_class', 'statements', 'statement_includes'
	]
	def __init__(self, filename: str) -> None:
		with open(filename, 'r') as file:
			self.config: dict[str, str | dict[str, str] | list[str]] = json.load(file)

		self.expression_class: str = self.config['expression_class']
		self.expressions: dict[str, str] = self.config['expressions']
		self.expression_includes: list[str] = self.config['expression_includes']

		self.statement_class: str = self.config['statement_class']
		self.statements: dict[str, str] = self.config['statements']
		self.statement_includes: list[str] = self.config['statement_includes']

