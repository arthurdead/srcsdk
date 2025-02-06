# tier0.cmake

add_library(tier0 SHARED IMPORTED)

set_target_properties(tier0 PROPERTIES IMPORTED_LOCATION "${ENGINEBINDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}tier0${CMAKE_SHARED_LIBRARY_SUFFIX}")

target_include_directories(
	tier0 INTERFACE
	"${SRCDIR}/public/tier0"
)
