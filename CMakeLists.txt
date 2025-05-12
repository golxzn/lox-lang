cmake_minimum_required(VERSION 3.25...3.30)

set(root ${CMAKE_CURRENT_SOURCE_DIR})

project(lox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${root}/bin
)

target_include_directories(${PROJECT_NAME} PUBLIC ${root}/code)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${root}/code/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${root}/code/*.hpp)
file(GLOB_RECURSE modules CONFIGURE_DEPENDS ${root}/code/*.ixx)

target_sources(${PROJECT_NAME} PUBLIC ${sources})
target_sources(${PROJECT_NAME} PUBLIC
	FILE_SET cxx_headers TYPE HEADERS
	BASE_DIRS ${root}/code
	FILES ${headers}
)
target_sources(${PROJECT_NAME} PUBLIC
	FILE_SET cxx_modules TYPE CXX_MODULES
	BASE_DIRS ${root}/code
	FILES ${modules}
)
target_precompile_headers(${PROJECT_NAME}
PRIVATE
	"$<$<COMPILE_LANGUAGE:CXX>:<cstdint$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<string$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<string_view$<ANGLE-R>>"
)

add_subdirectory(${root}/libs)
target_link_libraries(${PROJECT_NAME} PUBLIC tb::libs)
