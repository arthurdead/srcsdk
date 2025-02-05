# hackmgr.cmake

set(HACKMGR_DIR ${SRCDIR}/hackmgr)
set(
	HACKMGR_SOURCE_FILES

	"${HACKMGR_DIR}/ivmodelinfo.cpp"
	"${HACKMGR_DIR}/IEngineTrace.cpp"
	"${HACKMGR_DIR}/ivdebugoverlay.cpp"
	"${HACKMGR_DIR}/hackmgr.cpp"
	"${HACKMGR_DIR}/isoundemittersystembase.cpp"
	"${HACKMGR_DIR}/createinterface.cpp"
	"${HACKMGR_DIR}/IStaticPropMgr.cpp"
	"${HACKMGR_DIR}/hackmgr_server.cpp"
	"${HACKMGR_DIR}/cvar.cpp"
	"${HACKMGR_DIR}/dlloverride.cpp"
	"${HACKMGR_DIR}/memalloc.cpp"
	"${HACKMGR_DIR}/commandline.cpp"
	"${HACKMGR_DIR}/dbg.cpp"
	"${HACKMGR_DIR}/cpumonitoring.cpp"
	"${HACKMGR_DIR}/filesystem.cpp"
	"${HACKMGR_DIR}/steam.cpp"

	"${SRCDIR}/public/SoundParametersInternal.cpp"
	"${SRCDIR}/public/interval.cpp"

	"$<$<NOT:${DEDICATED}>:${HACKMGR_DIR}/istudiorender.cpp>"
	"$<$<NOT:${DEDICATED}>:${HACKMGR_DIR}/hackmgr_client.cpp>"
	"$<$<NOT:${DEDICATED}>:${HACKMGR_DIR}/ishadowmgr.cpp>"
	"$<$<NOT:${DEDICATED}>:${HACKMGR_DIR}/ivmodelrender.cpp>"
	"$<$<NOT:${DEDICATED}>:${HACKMGR_DIR}/shadersystem.cpp>"
	"$<$<NOT:${DEDICATED}>:${HACKMGR_DIR}/toolframework.cpp>"
	"$<$<NOT:${DEDICATED}>:${HACKMGR_DIR}/vgui.cpp>"
)

add_library(hackmgr SHARED ${HACKMGR_SOURCE_FILES})

target_sources(hackmgr PUBLIC
	"${SRCDIR}/public/tier0/logging.cpp"
)

target_include_directories(
	hackmgr PRIVATE
	"${SRCDIR}/materialsystem"
	$<TARGET_PROPERTY:steam_api,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
	hackmgr PRIVATE
	$<TARGET_PROPERTY:steam_api,INTERFACE_LINK_LIBRARIES>
	$<TARGET_PROPERTY:tier0,INTERFACE_LINK_LIBRARIES>
)

if (${DEDICATED})
	set_target_properties(hackmgr PROPERTIES OUTPUT_NAME hackmgr_ds)
endif ()

target_compile_definitions(
	hackmgr PRIVATE
	BUILDING_HACKMGR

	$<${DEDICATED}:DLLNAME=hackmgr_ds>
	$<${DEDICATED}:MEMOVERRIDE_MODULE=hackmgr_ds>

	$<$<NOT:${DEDICATED}>:DLLNAME=hackmgr>
	$<$<NOT:${DEDICATED}>:MEMOVERRIDE_MODULE=hackmgr>
)
