# steamworks.cmake

set(STEAMWORKS_VER 160)

if (UNIX)
	if (APPLE)
		set(PLATSUBDIR "/osx32")
	else()
		set(PLATSUBDIR "/linux32")
	endif()
endif()

if (WIN32)
	set(PLATSUBDIR "/.")
endif()

set(STEAMWORKSLIBDIR "${THIRDPARTYDIR}/steamworks_sdk_${STEAMWORKS_VER}/redistributable_bin/${PLATSUBDIR}")

find_library(STEAM_API_LIB NAMES "steam_api" PATHS ${STEAMWORKSLIBDIR} NO_DEFAULT_PATH REQUIRED)

add_library(steamworks UNKNOWN IMPORTED)
set_target_properties(steamworks PROPERTIES IMPORTED_LOCATION ${STEAM_API_LIB})
target_include_directories(
	steamworks INTERFACE
	"${THIRDPARTYDIR}/steamworks_sdk_${STEAMWORKS_VER}/public"
)