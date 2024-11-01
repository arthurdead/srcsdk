project('steamworks','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
src_engine_dir = get_option('src_engine_dir')

source_base = subproject('source_base')

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	steamworks_dep = declare_dependency(
		compile_args: [
			'-DVERSION_SAFE_STEAM_API_INTERFACES',
		],
		include_directories: include_directories(
			join_paths(src_root_dir,'public','steam')
		),
		link_args: [
			'-lsteam_api'
		]
	)
elif target_system == 'windows'
	gendef = find_program('gendef', native: true)
	dlltool = find_program('dlltool')

	steam_api_dll = join_paths(src_engine_dir,'bin','steam_api.dll')

	steam_api_def = custom_target('steam_api.def',
		command: [gendef,'-','@INPUT@'],
		capture: true,
		input: steam_api_dll,
		output: '@BASENAME@.def'
	)

	steam_api_lib = custom_target('libsteam_api.a',
		command: [dlltool,'-D','@INPUT0@','-d','@INPUT1@','-l','@OUTPUT@'],
		input: [steam_api_dll,steam_api_def],
		output: 'libsteam_api.a',
		install: true,
		install_dir: source_base.get_variable('lib_folder'),
	)

	steamworks_dep = declare_dependency(
		compile_args: [
			'-DVERSION_SAFE_STEAM_API_INTERFACES',
		],
		include_directories: include_directories(
			join_paths(src_root_dir,'public','steam')
		),
		link_with: steam_api_lib
	)
else
	error('unsupported system')
endif
