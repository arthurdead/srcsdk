project('appframework','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

error('valve never shipped the sdk appframework lib for linux')

src_root_dir = get_option('src_root_dir')

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	lib_ext = '.a'
elif target_system == 'windows'
	lib_ext = '.lib'
else
	error('unsupported system')
endif

appframework_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','appframework')
	),
	link_args: [
		'-l:appframework'+lib_ext
	]
)
