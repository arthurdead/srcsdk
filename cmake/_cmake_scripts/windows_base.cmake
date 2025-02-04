# windows_base.cmake

add_compile_definitions(
	WIN32
	_WIN32
	_WINDOWS
	IS_WINDOWS_PC
	PLATFORM_WINDOWS_PC
)

if (MSVC)
	include("${CMAKE_CURRENT_LIST_DIR}/msvc_base.cmake")
endif()

list(
	APPEND ADDITIONAL_SOURCES_EXE
	"$<$<NOT:${IS_SOURCESDK}>:${SRCDIR}/public/windows_default.manifest>"
)

list(
	APPEND ADDITIONAL_LINK_LIBRARIES_EXE
	tier0
	tier1
	vstdlib
)

list(
	APPEND ADDITIONAL_LINK_LIBRARIES_DLL
	tier0
	tier1
	vstdlib
)