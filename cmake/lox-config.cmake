set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(NOT DEFINED CMAKE_SYSTEM_NAME)
	cmake_host_system_information(RESULT CMAKE_SYSTEM_NAME QUERY OS_NAME)
endif()

list(APPEND lox_definitions
	LOX_$<UPPER_CASE:${CMAKE_SYSTEM_NAME}>
	LOX_$<UPPER_CASE:$<CONFIG>>
)

if(LOX_BUILD_MULTITHREADED)
	if(LOX_BUILD_MULTITHREADED_CORES EQUAL 0)
		lox_query_host_info(LOX_AVAILABLE_CORES LOAD 50 QUERY NUMBER_OF_PHYSICAL_CORES)
	else()
		set(LOX_AVAILABLE_CORES ${LOX_BUILD_MULTITHREADED_CORES})
	endif()

	cmake_host_system_information(RESULT cores QUERY NUMBER_OF_PHYSICAL_CORES)
	lox_message(STATUS "[multithreaded] Using ${cores}/${LOX_AVAILABLE_CORES} jobs")
	list(APPEND lox_compile_options
		$<$<CXX_COMPILER_ID:MSVC>:/MP${LOX_AVAILABLE_CORES}>
		$<$<CXX_COMPILER_ID:GCC>:-j ${LOX_AVAILABLE_CORES}>
	)
endif()


if(LOX_ENABLE_CCACHE)
	include(lox-enable-ccache)
endif()

# >-------------------------| CODE GENERATION |-------------------------< #

lox_generate_ast(${lox_code_root})
lox_generate_program_class(${lox_code_root})

include(GenerateExportHeader)
generate_export_header(${lox_target_name}
	BASE_NAME LOX
	EXPORT_FILE_NAME ${lox_export_header_path}
	CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

