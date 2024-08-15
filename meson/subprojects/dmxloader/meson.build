project('dmxloader')

src_root_dir = get_option('src_root_dir')

dmxloader_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','dmxloader')
	),
	link_args: [
		'-l:dmxloader.a'
	]
)
