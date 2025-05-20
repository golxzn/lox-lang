
macro(lox_message)
	# if (NOT LOX_SILENT)
		message(${ARGN})
	# endif()
endmacro()

function(lox_query_host_info output_variable)
	cmake_parse_arguments(LOXQHI "" "LOAD;QUERY" "" ${ARGN})

	if(NOT DEFINED LOXQHI_QUERY)
		message(SEND_ERROR
			"[lox_query_host_info] QUERY parameter is required! "
			"See https://cmake.org/cmake/help/latest/command/cmake_host_system_information.html"
		)
		return()
	endif()
	if(NOT DEFINED LOXQHI_LOAD)
		set(LOXQHI_LOAD 80)
	endif()

	if(LOXQHI_LOAD LESS_EQUAL 0)
		set(${output_variable} 1 PARENT_SCOPE)
		return()
	endif()

	if(NOT DEFINED LOXQHI_CACHED_${LOXQHI_QUERY} AND NOT LOXQHI_CACHED_${LOXQHI_QUERY}_LOAD EQUAL LOXQHI_LOAD)
		cmake_host_system_information(RESULT cores QUERY ${LOXQHI_QUERY})
		math(EXPR used_cores "(${cores} * ${LOXQHI_LOAD}) / 100" OUTPUT_FORMAT DECIMAL)
		lox_message(STATUS "[lox_query_host_info] Cores     : ${cores}")
		lox_message(STATUS "[lox_query_host_info] Used cores: ${used_cores}")
		set(LOXQHI_CACHED_${LOXQHI_QUERY} ${used_cores} CACHE STRING "" FORCE)
		set(LOXQHI_CACHED_${LOXQHI_QUERY}_LOAD ${LOXQHI_LOAD} CACHE STRING "" FORCE)
		mark_as_advanced(LOXQHI_CACHED_${LOXQHI_QUERY} LOXQHI_CACHED_${LOXQHI_QUERY}_LOAD)
	endif()

	set(${output_variable} ${LOXQHI_CACHED_${LOXQHI_QUERY}} PARENT_SCOPE)

endfunction()

function(lox_ast_generate directory)
	find_package(Python3 REQUIRED)
	set(_script ${root}/tools/codegen/generate_ast.py)
	execute_process(
		COMMAND ${Python3_EXECUTABLE} ${_script} --output ${directory}
	)
	unset(_script)
endfunction()

