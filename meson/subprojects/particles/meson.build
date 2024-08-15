project('particles')

src_root_dir = get_option('src_root_dir')

particles_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','particles')
	),
	link_args: [
		'-l:particles.a'
	]
)
