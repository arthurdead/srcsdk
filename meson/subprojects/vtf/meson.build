project('vtf')

src_root_dir = get_option('src_root_dir')

vtf_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','vtf')
	),
	link_args: [
		'-l:vtf.a'
	]
)
