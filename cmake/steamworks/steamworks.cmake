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

add_library(steam_api SHARED IMPORTED)

set_target_properties(steam_api PROPERTIES IMPORTED_LOCATION "${ENGINEBINDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}steam_api${CMAKE_SHARED_LIBRARY_SUFFIX}")

target_include_directories(
	steam_api INTERFACE
	"${THIRDPARTYDIR}/steamworks_sdk_${STEAMWORKS_VER}/public"
)
