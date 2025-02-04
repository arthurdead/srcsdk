# platform_dirs.cmake

if (UNIX)
	if (APPLE)
		set(PLATSUBDIR_OS "/osx32")
	else()
		set(PLATSUBDIR_OS "/linux32")
	endif()
endif()

if (WIN32)
	set(PLATSUBDIR_OS "/")
endif()

if (${HACKMGR_ENGINE_TARGET} STREQUAL "SDK2013MP")
	set(PLATSUBDIR "${PLATSUBDIR_OS}/sdk2013mp")
elseif (${HACKMGR_ENGINE_TARGET} STREQUAL "SDK2013SP")
	set(PLATSUBDIR "${PLATSUBDIR_OS}/sdk2013sp")
else ()
	message( FATAL_ERROR "Invalid engine target" )
endif ()