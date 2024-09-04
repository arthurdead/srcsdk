project('vstdlib','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

vstdlib_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','vstdlib')
	),
	link_args: [
		'-lvstdlib'
	]
)
