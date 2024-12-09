project('tier2','cpp')

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

	tier2_dep = declare_dependency(
		sources: files(
			join_paths(src_root_dir,'public','materialsystem','MaterialSystemUtil.cpp'),
			join_paths(src_root_dir,'tier2','tier2.cpp'),
		),
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier2')
		),
		link_args: [
			'-l:tier2_new'+lib_ext
		]
	)
endif
