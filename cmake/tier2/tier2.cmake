# tier2.cmake

set(TIER2_DIR ${SRCDIR}/tier2)

if (${FUNNY})
	include("tier2/funny/tier2.cmake")
else()
	add_library(tier2 STATIC IMPORTED)

	set_target_properties(tier2 PROPERTIES IMPORTED_LOCATION "${LIBPUBLIC}/tier2_new${CMAKE_STATIC_LIBRARY_SUFFIX}")

	target_include_directories(
		tier2 INTERFACE
		"${SRCDIR}/public/tier2"
	)
endif()

target_sources(tier2 INTERFACE
	"${SRCDIR}/public/materialsystem/MaterialSystemUtil.cpp"
	"${TIER2_DIR}/tier2.cpp"
)
