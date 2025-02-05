# vstdlib.cmake

find_library(VSTDLIB_LIB NAMES "vstdlib" PATHS ${ENGINEBINDIR} NO_DEFAULT_PATH REQUIRED)

add_library(vstdlib SHARED IMPORTED)

set_target_properties(vstdlib PROPERTIES IMPORTED_LOCATION ${VSTDLIB_LIB})

target_include_directories(
	vstdlib INTERFACE
	"${SRCDIR}/public/vstdlib"
)
