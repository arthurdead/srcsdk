# base.cmake

# NOTE: We use 0 or 1 so we can use these more easily in generator expressions
# Initialize them with default values that we then set later

set(IS_WINDOWS 0)
set(IS_LINUX 0)
set(IS_OSX 0)
set(IS_POSIX 0)

set(IS_XCODE 0)
set(IS_SOURCESDK 1)

if (WIN32)
	set(IS_WINDOWS 1)
endif()

if (UNIX AND NOT APPLE)
	set(IS_LINUX 1)
endif()

if (APPLE)
	set(IS_OSX 1)
	if (${CMAKE_GENERATOR} STREQUAL "Xcode")
		set(IS_XCODE 1)
	endif()
endif()

if (UNIX)
	set(IS_POSIX 1)
endif()

if (${IS_WINDOWS})
	set(_DLL_EXT ".dll")
	set(STATIC_LIB_EXT ".lib")
	set(IMPLIB_EXT ".lib")
elseif (${IS_OSX})
	set(_DLL_EXT ".dylib")
	set(STATIC_LIB_EXT ".a")
	set(IMPLIB_EXT ".dylib")
elseif(${IS_LINUX})
	set(_DLL_EXT ".so")
	set(STATIC_LIB_EXT ".a")
	set(IMPLIB_EXT ".so")
endif()

set(BUILD_REPLAY 0)
set(DEDICATED 0)

add_compile_definitions(
	$<$<CONFIG:Debug>:DEBUG>
	$<$<CONFIG:Debug>:_DEBUG>
	$<$<CONFIG:Debug>:STAGING_ONLY>
	$<$<CONFIG:Debug>:FRAME_POINTER_OMISSION_DISABLED>
	$<$<CONFIG:Debug>:DEV_BUILD>
	$<$<CONFIG:Debug>:VPROF_ENABLED>

	$<$<CONFIG:Release>:NDEBUG>
	$<$<CONFIG:Release>:_RETAIL>

	$<${DEDICATED}:DEDICATED>
	$<${DEDICATED}:SWDS>

	MOVE_CONSTRUCTOR_SUPPORT
	VALVE_RVALUE_REFS
	VALVE_INITIALIZER_LIST_SUPPORT

	RAD_TELEMETRY_DISABLED
	_DLL_EXT=${_DLL_EXT}
	HACKMGR_ENGINE_TARGET=HACKMGR_ENGINE_TARGET_${HACKMGR_ENGINE_TARGET}
)