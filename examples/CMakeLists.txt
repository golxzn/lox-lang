
set(cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB examples RELATIVE ${cur_dir} CONFIGURE_DEPENDS ${cur_dir}/**/CMakeLists.txt)
list(TRANSFORM examples REPLACE "/CMakeLists.txt" "")
list(REMOVE_DUPLICATES examples)

foreach(example IN LISTS examples)
	add_subdirectory(${example})
endforeach()

unset(cur_dir)
