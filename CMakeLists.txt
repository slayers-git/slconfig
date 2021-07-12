cmake_minimum_required (VERSION 3.12)
project (slconfig)

set (cfiles
	${PROJECT_SOURCE_DIR}/slconfig.c)
set (hfiles
	${PROJECT_SOURCE_DIR}/slconfig.h)

option (SLC_EXAMPLES "build example.c"  OFF) 
option (SLC_ERRORS "enable error map"    ON)
option (SLC_VARIABLES "enable variables" ON)

add_library (slconfig SHARED
	${cfiles}
	${hfiles})
set_target_properties (slconfig PROPERTIES OUTPUT_NAME "slconfig")

if (SLC_EXAMPLES)
	add_executable (example "example.c")
	file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/example.conf DESTINATION
		${CMAKE_CURRENT_BINARY_DIR})
	target_link_libraries (example slconfig)
endif ()

if (SLC_ERRORS)
	list (APPEND CDEFS
		"SLC_ERRORS")
endif ()

if (SLC_VARIABLES)
	list (APPEND CDEFS
		"SLC_VARIABLES")
endif ()

target_compile_definitions (slconfig PRIVATE ${CDEFS})

install (TARGETS slconfig
	 LIBRARY DESTINATION lib
	 ARCHIVE DESTINATION lib)

install (FILES ${hfiles} DESTINATION include)
