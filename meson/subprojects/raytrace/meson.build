project('raytrace', 'cpp')

src_root_dir = get_option('src_root_dir')
raytrace_src_dir = join_paths(src_root_dir,'raytrace')

source_base = subproject('source_base')

raytrace_lib = static_library('raytrace',
	sources: files(
		join_paths(raytrace_src_dir,'raytrace.cpp'),
		join_paths(raytrace_src_dir,'trace2.cpp'),
		join_paths(raytrace_src_dir,'trace3.cpp'),
	),
	include_directories: include_directories(
		join_paths(src_root_dir,'public','tier0'),
		join_paths(src_root_dir,'public','tier1')
	),
	dependencies: [
		source_base.get_variable('source_linux_static_lib_dep')
	]
)

raytrace_dep = declare_dependency(
	link_with: raytrace_lib
)
