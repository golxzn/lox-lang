
set(_libs)
set(BUILD_STATIC_LIBS ON)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
	message(STATUS "OpenMP found and will be used")
	list(APPEND _libs OpenMP::OpenMP_CXX)
endif()

# >------------------------------ expected -------------------------------< #

set(EXPECTED_BUILD_TESTS OFF)
add_subdirectory(expected)
list(APPEND _libs tl::expected)

# >------------------------------ fast_float -----------------------------< #

set(FASTFLOAT_CXX_STANDARD ${CMAKE_CXX_STANDARD})
set(FASTFLOAT_INSTALL OFF)
add_subdirectory(fast_float)
list(APPEND _libs FastFloat::fast_float)

# >------------------------------- target --------------------------------< #

add_library(tb_libs INTERFACE)
add_library(tb::libs ALIAS tb_libs)
target_link_libraries(tb_libs INTERFACE ${_libs})

message(STATUS "Libraries:")
foreach(lib IN LISTS _libs)
	message(STATUS "-> ${lib}")
endforeach()


unset(_libs)