project('server_dedicated_base','cpp','c')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

hackmgr = subproject('hackmgr')

src_root_dir = get_option('src_root_dir')

server_src_dir = join_paths(src_root_dir,'game','server')

server_dedicated_base_dep = declare_dependency(
	compile_args: [
		'-DDLLNAME=server_srv',
		'-DDEDICATED',
		'-DSWDS'
	],
	sources: files(
		join_paths(server_src_dir,'debugoverlay_null.cpp'),
		hackmgr.get_variable('hackmgr_ds_dep'),
	),
)
