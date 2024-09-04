project('choreoobjects', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
shared_src_dir = join_paths(src_root_dir,'game','shared')

choreoobjects_lib = static_library('choreoobjects',
	cpp_args: [
		'-DLIBNAME=choreoobjects'
	],
	sources: files(
		join_paths(shared_src_dir,'choreoactor.cpp'),
		join_paths(shared_src_dir,'choreochannel.cpp'),
		join_paths(shared_src_dir,'choreoevent.cpp'),
		join_paths(shared_src_dir,'choreoscene.cpp'),
		join_paths(shared_src_dir,'sceneimage.cpp')
	),
	include_directories: include_directories(
		join_paths(public_dir,'tier0'),
		join_paths(public_dir,'tier1')
	),
	dependencies: [
		source_base.get_variable('source_os_static_lib_dep')
	]
)

choreoobjects_dep = declare_dependency(
	link_with: choreoobjects_lib
)
