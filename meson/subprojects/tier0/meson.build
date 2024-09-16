project('tier0','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	lib_arg = 'tier0'
elif target_system == 'windows'
	lib_arg = ':tier0.lib'
else
	error('unsupported system')
endif

tier0_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','tier0')
	),
	link_args: [
		'-l'+lib_arg
	]
)
