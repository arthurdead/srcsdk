project('source_game','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

server_base = subproject('server_base')
vgui_controls = subproject('vgui_controls')
vtf = subproject('vtf')
matsys_controls = subproject('matsys_controls')
bitmap = subproject('bitmap')

server_src_dir = join_paths(src_root_dir,'game','server')

server_listen_base_dep = declare_dependency(
	compile_args: [
		'-DDLLNAME=server',
	],
	sources: files(
		join_paths(server_src_dir,'vgui_gamedll_int.cpp'),
	),
	dependencies: [
		vgui_controls.get_variable('vgui_controls_dep'),
		matsys_controls.get_variable('matsys_controls_dep'),
		vtf.get_variable('vtf_dep'),
		bitmap.get_variable('bitmap_dep')
	]
)
