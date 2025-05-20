
set(lox_libraries)
set(BUILD_STATIC_LIBS ON)

# >------------------------------ expected -------------------------------< #

set(EXPECTED_BUILD_TESTS OFF)
add_subdirectory(expected)
list(APPEND lox_libraries tl::expected)

# >------------------------------ fast_float -----------------------------< #

set(FASTFLOAT_CXX_STANDARD ${lox_cxx_standard})
add_subdirectory(fast_float)
list(APPEND lox_libraries FastFloat::fast_float)

# >-----------------------------------------------------------------------< #

target_link_libraries(${lox_target_name} PUBLIC ${lox_libraries})

if(NOT LOX_SILENT)
	message(STATUS "Libraries:")
	foreach(lib IN LISTS _libs)
		message(STATUS "-> ${lib}")
	endforeach()
endif()

unset(lox_libraries)
