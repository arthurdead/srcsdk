project('steamworks')

src_root_dir = get_option('src_root_dir')

steamworks_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','steam')
	),
	link_args: [
		'-lsteam_api'
	]
)
