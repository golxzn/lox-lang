
INCLUDE_DIR: str = 'include/lox/gen/'
SOURCES_DIR: str = 'sources/lox/gen/'

DISCLAIMER: str = '''/* ~~~ DO NOT MODIFY THIS FILE ~~~
 * This file is auto generated using {generator_filename}
 */
'''

HEADER_BEGIN: str = '''#pragma once

#include "lox/export.hpp"'''

OPEN_NAMESPACE: str = '''

namespace lox {

'''

CLOSE_NAMESPACE: str = '''
} // namespace lox

'''

SOURCE_BEGIN: str = '''#include "{header_file_path}"'''

