project('tier2')

src_root_dir = get_option('src_root_dir')

tier2_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','tier2')
	),
	link_args: [
		'-l:tier2.a'
	]
)
