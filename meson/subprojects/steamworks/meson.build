project('steamworks','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
src_engine_dir = get_option('src_engine_dir')
src_dedicated_engine_dir = get_option('src_dedicated_engine_dir')
build_dedicated_server = get_option('build_dedicated_server')

source_base = subproject('source_base')

steamworks_sdk_dir = join_paths(src_root_dir,'thirdparty','steamworks_sdk_160')

steamworks_sdk_inc = include_directories(
	join_paths(steamworks_sdk_dir,'public','steam'),
	join_paths(steamworks_sdk_dir,'public')
)

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	steamworks_dep = declare_dependency(
		compile_args: [
			
		],
		include_directories: steamworks_sdk_inc,
		link_args: [
			join_paths(src_engine_dir,'bin','libsteam_api.so')
		]
	)

	steamworks_ds_dep = declare_dependency(
		compile_args: [
			
		],
		include_directories: steamworks_sdk_inc,
		link_args: [
			join_paths(src_dedicated_engine_dir,'bin','libsteam_api.so')
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
			
		],
		include_directories: steamworks_sdk_inc,
		link_with: steam_api_lib
	)

	if build_dedicated_server
		steam_api_ds_dll = join_paths(src_dedicated_engine_dir,'bin','steam_api.dll')

		steam_api_ds_def = custom_target('steam_api_ds.def',
			command: [gendef,'-','@INPUT@'],
			capture: true,
			input: steam_api_ds_dll,
			output: '@BASENAME@.def'
		)

		steam_api_ds_lib = custom_target('libsteam_api_ds.a',
			command: [dlltool,'-D','@INPUT0@','-d','@INPUT1@','-l','@OUTPUT@'],
			input: [steam_api_ds_dll,steam_api_ds_def],
			output: 'libsteam_api_ds.a',
			install: true,
			install_dir: source_base.get_variable('lib_folder'),
		)

		steamworks_ds_dep = declare_dependency(
			compile_args: [
				
			],
			include_directories: steamworks_sdk_inc,
			link_with: steam_api_ds_lib
		)
	endif
else
	error('unsupported system')
endif
