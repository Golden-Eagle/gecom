
# require new behaviour of:
# CMP0054 (don't dereference quoted variables in if() args)
cmake_minimum_required(VERSION 3.1)

# Set GECOM default compile options (warnings etc.)
function(gecom_add_default_compile_options)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		# Full normal warnings, multithreaded build
		add_compile_options(/W4 /MP)
		# Disable C4800: forcing X to bool (performance warning)
		add_compile_options(/wd4800)
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		# C++14, full normal warnings
		add_compile_options(-std=c++1y -Wall -Wextra -pedantic)
		# Threading support
		add_compile_options(-pthread)
		# Promote missing return to error
		add_compile_options(-Werror=return-type)
		# enable coloured output if gcc >= 4.9
		# TODO remove this, use the GCC_COLORS env instead?
		execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
		if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
			add_compile_options(-fdiagnostics-color)
		endif()
	elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(Apple)?Clang$")
		# C++14, full normal warnings
		add_compile_options(-std=c++1y -Wall -Wextra -pedantic)
		# Threading support
		add_compile_options(-pthread)
		# Promote missing return to error
		add_compile_options(-Werror=return-type)
	endif()
endfunction()

# Add MSVC-style compile options
function(gecom_target_msvc_compile_options)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		target_compile_options(${ARGV})
	endif()
endfunction()

# Add GCC-style compile options
function(gecom_target_gcc_compile_options)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		target_compile_options(${ARGV})
	elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(Apple)?Clang$")
		target_compile_options(${ARGV})
	endif()
endfunction()














