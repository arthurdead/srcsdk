project('server_listen_base','cpp','c')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

server_base = subproject('server_base')
vgui_controls = subproject('vgui_controls')
vtf = subproject('vtf')
matsys_controls = subproject('matsys_controls')
bitmap = subproject('bitmap')

public_dir = join_paths(src_root_dir,'public')
server_src_dir = join_paths(src_root_dir,'game','server')
shared_src_dir = join_paths(src_root_dir,'game','shared')

server_listen_base_dep = declare_dependency(
	compile_args: [
		'-DDLLNAME=server',
	],
	sources: files(
		join_paths(server_src_dir,'vgui_gamedll_int.cpp'),
		join_paths(server_src_dir,'ndebugoverlay.cpp'),
		join_paths(public_dir,'toolframework','tooldictionary.cpp'),
		join_paths(server_src_dir,'foundryhelpers_server.cpp'),
		join_paths(server_src_dir,'entity_tools_server.cpp'),
		join_paths(server_src_dir,'toolframework_server.cpp'),
		join_paths(server_src_dir,'default','tooldictionary_default.cpp'),
		join_paths(shared_src_dir,'debugoverlay_shared.cpp'),
	),
	dependencies: [
		vgui_controls.get_variable('vgui_controls_dep'),
		matsys_controls.get_variable('matsys_controls_dep'),
		vtf.get_variable('vtf_dep'),
		bitmap.get_variable('bitmap_dep')
	]
)
