include(CheckCCompilerFlag)

project(usrsctplib)
cmake_minimum_required(VERSION 3.0)

# Debug build type as default
if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, using DEBUG")
	set(CMAKE_BUILD_TYPE "DEBUG")
endif ()

message(STATUS "Default compiler options: ${CMAKE_C_FLAGS}")

#################################################
# COMPILER SETTINGS
#################################################

# SETTINGS FOR UNIX COMPILER
if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -std=c99 -pedantic -Wall -Wextra -Werror")

	check_c_compiler_flag(-Wfloat-equal has_wfloat_equal)
	if (has_wfloat_equal)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wfloat-equal")
	endif ()

	check_c_compiler_flag(-Wshadow has_wshadow)
	if (has_wshadow)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wshadow")
	endif ()

	check_c_compiler_flag(-Wpointer-arith has_wpointer_aritih)
	if (has_wpointer_aritih)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wpointer-arith")
	endif ()

	check_c_compiler_flag(-Wunreachable-code has_wunreachable_code)
	if (has_wunreachable_code)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wunreachable-code")
	endif ()

	check_c_compiler_flag(-Winit-self has_winit_self)
	if (has_winit_self)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Winit-self")
	endif ()

	check_c_compiler_flag(-Wno-unused-function has_wno_unused_function)
	if (has_wno_unused_function)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wno-unused-function")
	endif ()

	check_c_compiler_flag(-Wno-unused-parameter has_wno_unused_parameter)
	if (has_wno_unused_parameter)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wno-unused-parameter")
	endif ()

	check_c_compiler_flag(-Wno-unreachable-code has_wno_unreachable_code)
	if (has_wno_unreachable_code)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wno-unreachable-code")
	endif ()

	check_c_compiler_flag(-Wstrict-prototypes has_wstrict_prototypes)
	if (has_wstrict_prototypes)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wstrict-prototypes")
	endif ()
endif ()


# SETTINGS FOR VISUAL STUDIO COMPILER
if (CMAKE_C_COMPILER_ID MATCHES "MSVC")
	message(STATUS "MSVC COMPILER!")
	if (CMAKE_C_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W3" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	else ()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
	endif ()

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
endif ()

message(STATUS "Using compiler options: ${CMAKE_C_FLAGS}")

set(check_programs
	tcp-fastopen.c
)

foreach (source_file ${check_programs})
	get_filename_component(source_file_we ${source_file} NAME_WE)
	add_executable(${source_file_we} ${source_file})
endforeach ()
