cmake_minimum_required(VERSION 3.25...3.30)

set(root ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${root}/cmake)

include(lox-tools)
include(lox-prelude OPTIONAL)

project(lox
	VERSION 0.1.0
	DESCRIPTION "I'm learning how to craft interpreters"
	LANGUAGES C CXX
)

include(GNUInstallDirs)
include(lox-options)

add_library(${lox_target_name})
add_library(golxzn::${lox_target_name} ALIAS ${lox_target_name})

include(lox-config)

target_compile_features(${lox_target_name} PRIVATE
	c_std_${lox_c_standard}
	cxx_std_${lox_cxx_standard}
)

set_target_properties(${lox_target_name} PROPERTIES
	C_STANDARD ${lox_c_standard}
	C_STANDARD_REQUIRED ON
	CXX_STANDARD ${lox_cxx_standard}
	CXX_STANDARD_REQUIRED ON
	CXX_VISIBILITY_PRESET hidden
	CXX_EXTENSIONS OFF
	FOLDER "golxzn/${lox_target_name}"
	VISIBILITY_INLINES_HIDDEN YES
	VERSION "${PROJECT_VERSION}"
	SOVERSION "${PROJECT_VERSION_MAJOR}"
	EXPORT_NAME ${lox_package_name}
	OUTPUT_NAME ${lox_package_name}
)

target_compile_options(${lox_target_name} PRIVATE ${lox_compile_options})
target_compile_definitions(${lox_target_name} PRIVATE ${lox_definitions})
target_include_directories(${lox_target_name} PUBLIC
	$<BUILD_INTERFACE:${lox_inc_dir}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(${lox_target_name} SYSTEM PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${lox_src_dir}/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${lox_inc_dir}/*.hpp)

target_sources(${lox_target_name} PRIVATE ${sources})
target_sources(${lox_target_name} PUBLIC
	FILE_SET cxx_headers TYPE HEADERS
	BASE_DIRS ${root}/code
	FILES ${headers}
)

unset(headers)
unset(sources)

target_precompile_headers(${lox_target_name}
PRIVATE
	"$<$<COMPILE_LANGUAGE:CXX>:<cstdint$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<string$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<string_view$<ANGLE-R>>"
)

add_subdirectory(${root}/libs)

if(NOT CMAKE_SKIP_INSTALL_RULES)
	include(lox-export)
endif()

if(LOX_BUILD_EXAMPLES)
	add_subdirectory(${root}/examples)
endif()

include(lox-postlude OPTIONAL)
