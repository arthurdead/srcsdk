# tier0.cmake

find_library(TIER0_LIB NAMES "tier0" PATHS ${ENGINEBINDIR} NO_DEFAULT_PATH REQUIRED)

add_library(tier0 SHARED IMPORTED)

set_target_properties(tier0 PROPERTIES IMPORTED_LOCATION ${TIER0_LIB})

target_include_directories(
	tier0 INTERFACE
	"${SRCDIR}/public/tier0"
)
