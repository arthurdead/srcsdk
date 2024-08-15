project('tier3')

src_root_dir = get_option('src_root_dir')

tier3_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','tier3')
	),
	link_args: [
		'-l:tier3.a'
	]
)
