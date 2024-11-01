project('tier3','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

if get_option('funny')
	subdir('funny')
else
	target_system = target_machine.system()
	if target_system == 'linux' or target_system == 'gnu'
		lib_ext = '.a'
	elif target_system == 'windows'
		lib_ext = '.lib'
	else
		error('unsupported system')
	endif

	tier3_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier3')
		),
		link_args: [
			'-l:tier3_new'+lib_ext
		]
	)
endif
