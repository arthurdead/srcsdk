project('matsys_controls')

src_root_dir = get_option('src_root_dir')

matsys_controls_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','matsys_controls')
	),
	link_args: [
		'-l:matsys_controls.a'
	]
)
