import os
import constants
from io import TextIOWrapper

def get_filename(filename: str = __file__) -> str:
	return os.path.basename(os.path.realpath(filename))

def make_include_path(class_name: str) -> str:
	return '"' + os.path.join(constants.INCLUDE_DIR.removeprefix('include/'), class_name + '.hpp"').replace('\\', '/')

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

