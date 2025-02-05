# raytrace.cmake

set(RAYTRACE_DIR ${SRCDIR}/raytrace)
set(
	RAYTRACE_SOURCE_FILES

	"${RAYTRACE_DIR}/raytrace.cpp"
	"${RAYTRACE_DIR}/trace2.cpp"
	"${RAYTRACE_DIR}/trace3.cpp"
)

add_library(raytrace STATIC ${RAYTRACE_SOURCE_FILES})

set_target_properties(raytrace PROPERTIES PREFIX "")

target_include_directories(
	raytrace PRIVATE
	"${SRCDIR}/utils/common"
)

target_compile_definitions(
	raytrace PRIVATE
	LIBNAME=raytrace
)
