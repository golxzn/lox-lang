if(PROJECT_IS_TOP_LEVEL)
	set(CMAKE_INSTALL_INCLUDEDIR include/ CACHE PATH "")
endif()

# find_package(<package>) call for consumers to find this project
set(package ${lox_package_name})
install(
	DIRECTORY
		"${lox_inc_dir}"
		"${PROJECT_BINARY_DIR}/include/"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	COMPONENT ${package}_development
)

# Allow package maintainers to freely override the path for the configs
set(${package}_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/shared/cmake/${package}"
	CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(${package}_INSTALL_CMAKEDIR)

configure_package_config_file(${root}/cmake/lox-install-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/${package}-config.cmake
	INSTALL_DESTINATION ${${package}_INSTALL_CMAKEDIR}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${package}-config.cmake
	DESTINATION ${${package}_INSTALL_CMAKEDIR}
	COMPONENT ${package}_development
)

install(FILES ${PROJECT_BINARY_DIR}/${package}-config-version.cmake
	DESTINATION ${${package}_INSTALL_CMAKEDIR}
	COMPONENT ${package}_development
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${package}-config-version.cmake"
	COMPATIBILITY SameMajorVersion
	ARCH_INDEPENDENT
)

install(TARGETS ${lox_target_name}
	EXPORT ${package}-targets

	RUNTIME COMPONENT ${package}_runtime
	LIBRARY COMPONENT ${package}_runtime
	ARCHIVE COMPONENT ${package}_development

	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}

	PUBLIC_HEADER DESTINATION include COMPONENT ${package}_development
	FILE_SET cxx_headers
)

install(EXPORT ${package}-targets
	FILE ${package}-targets.cmake
	NAMESPACE ${lox_target_name}::
	DESTINATION ${${package}_INSTALL_CMAKEDIR}
)

if(PROJECT_IS_TOP_LEVEL)
	include(CPack)
endif()
unset(package)
