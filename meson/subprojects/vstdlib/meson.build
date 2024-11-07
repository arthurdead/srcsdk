project('vstdlib','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
src_engine_dir = get_option('src_engine_dir')
src_dedicated_engine_dir = get_option('src_dedicated_engine_dir')
build_dedicated_server = get_option('build_dedicated_server')

source_base = subproject('source_base')

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	vstdlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','vstdlib')
		),
		link_args: [
			'-lvstdlib'
		]
	)

	vstdlib_ds_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','vstdlib')
		),
		link_args: [
			'-lvstdlib_srv'
		]
	)
elif target_system == 'windows'
	gendef = find_program('gendef', native: true)
	dlltool = find_program('dlltool')

	vstdlib_dll = join_paths(src_engine_dir,'bin','vstdlib.dll')

	vstdlib_def = custom_target('vstdlib.def',
		command: [gendef,'-','@INPUT@'],
		capture: true,
		input: vstdlib_dll,
		output: '@BASENAME@.def'
	)

	vstdlib_lib = custom_target('libvstdlib.a',
		command: [dlltool,'-D','@INPUT0@','-d','@INPUT1@','-l','@OUTPUT@'],
		input: [vstdlib_dll,vstdlib_def],
		output: 'libvstdlib.a',
		install: true,
		install_dir: source_base.get_variable('lib_folder'),
	)

	vstdlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','vstdlib')
		),
		link_with: vstdlib_lib
	)

	if build_dedicated_server
		vstdlib_ds_dll = join_paths(src_dedicated_engine_dir,'bin','vstdlib_srv.dll')

		vstdlib_ds_def = custom_target('vstdlib_ds.def',
			command: [gendef,'-','@INPUT@'],
			capture: true,
			input: vstdlib_ds_dll,
			output: '@BASENAME@.def'
		)

		vstdlib_ds_lib = custom_target('libvstdlib_srv.a',
			command: [dlltool,'-D','@INPUT0@','-d','@INPUT1@','-l','@OUTPUT@'],
			input: [vstdlib_ds_dll,vstdlib_ds_def],
			output: 'libvstdlib_srv.a',
			install: true,
			install_dir: source_base.get_variable('lib_folder'),
		)

		vstdlib_ds_dep = declare_dependency(
			include_directories: include_directories(
				join_paths(src_root_dir,'public','vstdlib')
			),
			link_with: vstdlib_ds_lib
		)
	endif
else
	error('unsupported system')
endif
