project('server_listen_base','cpp','c')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

source_base = subproject('source_base')
server_base = subproject('server_base')
vgui_controls = subproject('vgui_controls')
vtf = subproject('vtf')
matsys_controls = subproject('matsys_controls')
bitmap = subproject('bitmap')
hackmgr = subproject('hackmgr')
steamworks = subproject('steamworks')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')

public_dir = join_paths(src_root_dir,'public')
server_src_dir = join_paths(src_root_dir,'game','server')
shared_src_dir = join_paths(src_root_dir,'game','shared')

server_listen_base_dep = declare_dependency(
	compile_args: [
		'-DDLLNAME=server',
		'-DMEMOVERRIDE_MODULE=server',
		'-DTOOL_DLL',
	],
	sources: files(
		join_paths(server_src_dir,'vgui_gamedll_int.cpp'),
		join_paths(server_src_dir,'ndebugoverlay.cpp'),
		join_paths(server_src_dir,'foundryhelpers_server.cpp'),
		join_paths(public_dir,'editor_sendcommand.cpp'),
		join_paths(shared_src_dir,'wcedit.cpp'),
		join_paths(server_src_dir,'entity_tools_server.cpp'),
		join_paths(server_src_dir,'toolframework_server.cpp'),
		join_paths(shared_src_dir,'toolframework_shared.cpp'),
		join_paths(shared_src_dir,'debugoverlay_shared.cpp'),
		join_paths(shared_src_dir,'entityinspector.cpp'),
	),
	dependencies: [
		vgui_controls.get_variable('vgui_controls_dep'),
		matsys_controls.get_variable('matsys_controls_dep'),
		vtf.get_variable('vtf_dep'),
		bitmap.get_variable('bitmap_dep'),
		hackmgr.get_variable('hackmgr_dep'),
		source_base.get_variable('source_cl_dep'),
		steamworks.get_variable('steamworks_dep'),
		tier0.get_variable('tier0_dep'),
		vstdlib.get_variable('vstdlib_dep'),
	]
)
