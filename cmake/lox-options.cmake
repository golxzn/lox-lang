
option(LOX_SILENT              "Hide lox messages"          ${PROJECT_IS_TOP_LEVEL})
option(LOX_BUILD_EXAMPLES      "Build examples"             ${PROJECT_IS_TOP_LEVEL})
option(LOX_BUILD_INTERPRETER   "Build lox interpreter"      ${PROJECT_IS_TOP_LEVEL})
option(LOX_ENABLE_CCACHE       "Use ccache"                 ON)
option(LOX_BUILD_MULTITHREADED "Enable multithreaded build" ON)
set(LOX_BUILD_MULTITHREADED_CORES 0 CACHE STRING "Explicitly set the number of cores for multithreaded build (0 for auto)")

set(CMAKE_EXPORT_COMPILE_COMMANDS ${PROJECT_IS_TOP_LEVEL})

set(lox_package_name golxzn-lox CACHE STRING "Package export name")
mark_as_advanced(lox_package_name)

set(lox_target_name lox)
set(lox_code_root ${root}/code           CACHE PATH "The root sources directory")
set(lox_inc_dir ${lox_code_root}/include CACHE PATH "The include files directory")
set(lox_src_dir ${lox_code_root}/sources CACHE PATH "The source files directory")

set(lox_export_header_path include/lox/export.hpp)

set(lox_c_standard   17 CACHE STRING "The C standard")
set(lox_cxx_standard 20 CACHE STRING "The C++ standard")


set(lox_definitions)
set(lox_compile_options)
