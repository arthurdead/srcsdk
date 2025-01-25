project('server_dedicated_base','cpp','c', default_options: [
	'cpp_std=gnu++20'
])

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

hackmgr = subproject('hackmgr')
source_base = subproject('source_base')
steamworks = subproject('steamworks')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')

src_root_dir = get_option('src_root_dir')

shared_src_dir = join_paths(src_root_dir,'game','shared')

server_dedicated_base_dep = declare_dependency(
	compile_args: [
		'-DDLLNAME=server_srv',
		'-DMEMOVERRIDE_MODULE=server_srv',
		'-DDEDICATED',
		'-DSWDS'
	],
	sources: files(
		
	),
	dependencies: [
		hackmgr.get_variable('hackmgr_ds_dep'),
		source_base.get_variable('source_ds_dep'),
		steamworks.get_variable('steamworks_ds_dep'),
		tier0.get_variable('tier0_ds_dep'),
		vstdlib.get_variable('vstdlib_ds_dep'),
	]
)
