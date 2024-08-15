project('tier0')

src_root_dir = get_option('src_root_dir')

tier0_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','tier0')
	),
	link_args: [
		'-ltier0'
	]
)
