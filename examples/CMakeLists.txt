project(lox_examples LANGUAGES CXX)

# find_package(GolxznLox REQUIRED)

set(target ${PROJECT_NAME}_examples)

add_executable(${target} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(${target} PUBLIC golxzn::lox)
set_target_properties(${target} PROPERTIES
	C_STANDARD ${lox_c_standard}
	CXX_STANDARD ${lox_cxx_standard}
	RUNTIME_OUTPUT_DIRECTORY ${root}/bin
)

