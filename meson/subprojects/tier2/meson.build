project('tier2')

src_root_dir = get_option('src_root_dir')

public_dir = join_paths(src_root_dir,'public')

tier2_dep = declare_dependency(
	sources: files(
		join_paths(public_dir,'materialsystem','MaterialSystemUtil.cpp'),
	),
	include_directories: include_directories(
		join_paths(src_root_dir,'public','tier2')
	),
	link_args: [
		'-l:tier2_no_matsys_util.a'
	]
)
