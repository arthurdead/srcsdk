project('tier0','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
src_engine_dir = get_option('src_engine_dir')

source_base = subproject('source_base')

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	tier0_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier0')
		),
		link_args: ['-ltier0']
	)
elif target_system == 'windows'
	gendef = find_program('gendef', native: true)
	dlltool = find_program('dlltool')

	tier0_dll = join_paths(src_engine_dir,'bin','tier0.dll')

	tier0_def = custom_target('tier0.def',
		command: [gendef,'-','@INPUT@'],
		capture: true,
		input: tier0_dll,
		output: '@BASENAME@.def'
	)

	tier0_lib = custom_target('libtier0.a',
		command: [dlltool,'-D','@INPUT0@','-d','@INPUT1@','-l','@OUTPUT@'],
		input: [tier0_dll,tier0_def],
		output: 'libtier0.a',
		install: true,
		install_dir: source_base.get_variable('lib_folder'),
	)

	tier0_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier0')
		),
		link_with: tier0_lib
	)
else
	error('unsupported system')
endif
